// {C} Copyright 2017 Pensando Systems Inc. All rights reserved.

package tstore

import (
	"fmt"
	"io"
	"path/filepath"
	"strings"
	"time"

	"github.com/influxdata/influxdb/coordinator"
	"github.com/influxdata/influxdb/models"
	"github.com/influxdata/influxdb/query"
	"github.com/influxdata/influxdb/services/meta"
	"github.com/influxdata/influxdb/tsdb"
	"github.com/influxdata/influxql"

	"github.com/pensando/sw/venice/citadel/tproto"
	"github.com/pensando/sw/venice/utils/log"
)

// Tstore is a timeseries store instance
type Tstore struct {
	dbPath        string                         // directory where files are stored
	tsdb          *tsdb.Store                    // tsdb store instance
	stmtExecutor  *coordinator.StatementExecutor // statement executor
	queryExecutor *query.QueryExecutor           // query executor
	pointsWriter  *coordinator.PointsWriter      // points writer
	metaClient    *meta.Client                   // metadata client
}

// number of times to retru backup operation if it fails transiently..
const numBackupRetries = 5

// NewTstoreWithConfig returns a new tstore instance with the custom engine config
func NewTstoreWithConfig(dbPath string, cfg tsdb.Config) (*Tstore, error) {
	// create a new tsdb store
	ts := tsdb.NewStore(dbPath)

	// set the custom config, use tsdb.NewConfig() for deaults
	ts.EngineOptions.Config = cfg

	// open the tsdb store
	err := ts.Open()
	if err != nil {
		log.Fatalf("Error opening the tsdb. Err: %v", err)
	}

	return newTstore(dbPath, ts)
}

// NewTstore returns a new tstore instance
func NewTstore(dbPath string) (*Tstore, error) {
	// create a new tsdb store
	ts := tsdb.NewStore(dbPath)

	// set the WAL directory
	ts.EngineOptions.Config.WALDir = filepath.Join(dbPath, "wal")
	// todo: switch to tsi1, ts.EngineOptions.Config.Index = "tsi1"

	// update tag/series limits, TODO: check mem. utilization
	ts.EngineOptions.Config.MaxSeriesPerDatabase = 5 * tsdb.DefaultMaxSeriesPerDatabase
	ts.EngineOptions.Config.MaxValuesPerTag = 0

	// open the tsdb store
	err := ts.Open()
	if err != nil {
		log.Fatalf("Error opening the tsdb. Err: %v", err)
	}

	return newTstore(dbPath, ts)
}

func newTstore(dbPath string, ts *tsdb.Store) (*Tstore, error) {

	// local meta
	cfg := meta.NewConfig()
	cfg.Dir = dbPath
	localMeta := meta.NewClient(cfg)

	// open the metadata if it already exists
	err := localMeta.Open()
	if err != nil {
		// try again after a small delay
		time.Sleep(time.Millisecond * 10)
		err = localMeta.Open()
		if err != nil {
			log.Fatalf("Error opening the local metadata. Err: %v", err)
		}
	}

	// create a points writer
	pwr := coordinator.NewPointsWriter()
	pwr.TSDBStore = ts
	pwr.MetaClient = localMeta

	// create a statemete executor
	stEx := coordinator.StatementExecutor{
		MetaClient: localMeta,
		TSDBStore:  ts,
		ShardMapper: &coordinator.LocalShardMapper{
			MetaClient: localMeta,
			TSDBStore:  ts,
		},
		PointsWriter: pwr,
	}

	// create a query executor
	qEx := query.NewQueryExecutor()
	qEx.StatementExecutor = &stEx

	// create a tstore instance
	s := Tstore{
		dbPath:        dbPath,
		tsdb:          ts,
		stmtExecutor:  &stEx,
		queryExecutor: qEx,
		pointsWriter:  pwr,
		metaClient:    localMeta,
	}

	return &s, nil
}

// CreateDatabase creates a database
func (ts *Tstore) CreateDatabase(database string, spec *meta.RetentionPolicySpec) error {
	// if retention spec was nil, return error
	if spec == nil {
		return fmt.Errorf("invalid retention policy")
	}

	// create the database with retention policy
	_, err := ts.metaClient.CreateDatabaseWithRetentionPolicy(database, spec)
	if err != nil {
		return err
	}

	return nil
}

// ReadDatabases reads all databases
func (ts *Tstore) ReadDatabases() []meta.DatabaseInfo {
	// read databases
	return ts.metaClient.Databases()
}

// DeleteDatabase deletes the database
func (ts *Tstore) DeleteDatabase(database string) error {
	// delete it from tsdb
	err := ts.tsdb.DeleteDatabase(database)
	if err != nil {
		log.Errorf("Error deleting the database %s from tsdb. Err: %v", database, err)
	}

	// delete it from local metadata
	return ts.metaClient.DropDatabase(database)
}

// GetShardInfo will fill in the shard info
func (ts *Tstore) GetShardInfo(sinfo *tproto.SyncShardInfoMsg) error {
	// get the current snapshot of current metadata
	lmd := ts.metaClient.Data()
	data, err := lmd.MarshalBinary()
	if err != nil {
		return err
	}
	sinfo.ChunkMeta = data

	// read all the shards
	shards := ts.tsdb.Shards(ts.tsdb.ShardIDs())
	for _, shard := range shards {
		chunkInfo := tproto.ChunkInfo{
			ChunkID:  shard.ID(),
			Database: shard.Database(),
		}
		sinfo.Chunks = append(sinfo.Chunks, &chunkInfo)
	}

	return nil
}

// RestoreShardInfo restores local meta and chunk state
func (ts *Tstore) RestoreShardInfo(sinfo *tproto.SyncShardInfoMsg) error {
	var lmd meta.Data

	// read meta data from binary
	err := lmd.UnmarshalBinary(sinfo.ChunkMeta)
	if err != nil {
		return err
	}

	// restore local metadata
	err = ts.metaClient.SetData(&lmd)
	if err != nil {
		log.Errorf("Error restoring local metadata. Err: %v", err)
		return err
	}

	// recreate all the shards
	for _, chunk := range sinfo.Chunks {
		err = ts.tsdb.CreateShard(chunk.Database, "default", chunk.ChunkID, true)
		if err != nil {
			log.Errorf("Error creating shard %d for db %s. Err: %v", chunk.ChunkID, chunk.Database, err)
			return err
		}
	}

	return nil
}

// BackupChunk backs up a specific chunk
func (ts *Tstore) BackupChunk(chunkID uint64, w io.Writer) error {
	var err error
	i := 0
	// retry backup few times if compaction is in progress
	for i < numBackupRetries {
		err = ts.tsdb.BackupShard(chunkID, time.Unix(0, 0), w)
		if err == nil {
			return nil
		}

		// retry after a small delay
		time.Sleep(time.Millisecond * 10)

		// retry on "snapshots disabled" errror
		if strings.Contains(err.Error(), "snapshots") {
			log.Infof("retrying backup, err: %s", err)
			continue
		}
		i++
	}

	return err
}

// RestoreChunk restores a specific chunk
func (ts *Tstore) RestoreChunk(chunkID uint64, r io.Reader) error {
	return ts.tsdb.RestoreShard(chunkID, r)
}

// WritePoints writes points to a shard
func (ts *Tstore) WritePoints(database string, points []models.Point) error {
	return ts.pointsWriter.WritePointsPrivileged(database, "default", models.ConsistencyLevelOne, points)
}

// WritePointsInto is a wrapper to write points using BufferedPointsWriter
func (ts *Tstore) WritePointsInto(wr *coordinator.IntoWriteRequest) error {
	return ts.stmtExecutor.PointsWriter.WritePointsInto(wr)
}

// ExecuteQuery executes a query
func (ts *Tstore) ExecuteQuery(q, database string) (<-chan *query.Result, error) {
	// parse the query
	pq, err := influxql.ParseQuery(q)
	if err != nil {
		return nil, err
	}

	// execute the query
	ret := ts.queryExecutor.ExecuteQuery(pq, query.ExecutionOptions{
		Database:   database,
		ChunkSize:  0,
		Authorizer: &tstoreAuth{},
	}, make(chan struct{}))

	return ret, nil
}

// tstoreAuth is the Authorizer in tstore to avoid failure in imem.go:987 where iterator expects valid Authorizer
type tstoreAuth struct {
}

// AuthorizeDatabase indicates whether the given Privilege is authorized on the database with the given name.
func (ta *tstoreAuth) AuthorizeDatabase(p influxql.Privilege, name string) bool {
	return true
}

// AuthorizeQuery returns an error if the query cannot be executed
func (ta *tstoreAuth) AuthorizeQuery(database string, query *influxql.Query) error {
	return nil
}

// AuthorizeSeriesRead determines if a series is authorized for reading
func (ta *tstoreAuth) AuthorizeSeriesRead(database string, measurement []byte, tags models.Tags) bool {
	return true
}

// AuthorizeSeriesWrite determines if a series is authorized for writing
func (ta *tstoreAuth) AuthorizeSeriesWrite(database string, measurement []byte, tags models.Tags) bool {
	return true
}

// Close closes the tstore
func (ts *Tstore) Close() error {
	ts.pointsWriter.Close()
	ts.queryExecutor.Close()
	ts.metaClient.Close()
	ts.tsdb.Close()
	return nil
}
