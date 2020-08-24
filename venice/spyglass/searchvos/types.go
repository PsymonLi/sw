package searchvos

import (
	"context"
	"fmt"
	"strings"
	"sync"
	"time"

	"github.com/pensando/sw/venice/utils/log"
	minio "github.com/pensando/sw/venice/utils/objstore/minio"
	"github.com/pensando/sw/venice/utils/resolver"
	"github.com/pensando/sw/venice/utils/workfarm"
)

const (

	// A search context will get released within 5 minutes by default.
	// Once its released user would have to restart the search from the beginning.
	defaultQueryContextPurgeDuration = time.Minute * 5
)

// vos clients
var clients sync.Map

// Option provides optional parameters to the constructor
type Option func(s *SearchFwLogsOverVos)

// MetaIndexerOption provides optional parameters to the constructor
type MetaIndexerOption func(s *MetaIndexer)

// SearchFwLogsOverVos is used for searching flow logs over Vos
type SearchFwLogsOverVos struct {
	// context
	ctx context.Context

	// resolver
	res resolver.Interface

	// metaindexer, used for searching the index/logs present in memory
	mi *MetaIndexer

	// logger
	logger log.Logger

	// objstore credentials manager
	credentialsManager minio.CredentialsManager

	// Cache
	scache *searchCache

	// search workers
	fetchDataWorkers workfarm.PinnedWorker
}

type flowsSetOutput struct {
	id    int
	flows [][]string
}

type fileInfo struct {
	name string
	s    time.Time
	e    time.Time
}

func newFileInfo(fileName string) (fileInfo, error) {
	slashTokens := strings.Split(fileName, "/")
	relativeFileName := slashTokens[6]
	tokens := strings.Split(relativeFileName, "_")
	st, err := time.Parse(timeFormatWithoutColon, tokens[0])
	if err != nil {
		return fileInfo{}, fmt.Errorf(fmt.Sprintf("error is parsing file starts, fileName %s, err %+v", fileName, err))
	}
	et, err := time.Parse(timeFormatWithoutColon, strings.Split(tokens[1], ".")[0])
	if err != nil {
		return fileInfo{}, fmt.Errorf(fmt.Sprintf("error is parsing file endts, fileName %s, err %+v", fileName, err))
	}

	return fileInfo{
		name: fileName,
		s:    st,
		e:    et,
	}, nil
}

type ascendingFiles []fileInfo

func (s ascendingFiles) Len() int {
	return len(s)
}

func (s ascendingFiles) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s ascendingFiles) Less(i, j int) bool {
	return s[i].s.Before(s[j].s)
}

func (s ascendingFiles) getFileSets() [][]string {
	// maxFiles represent the maximum number of files that can be searched concurrently and sorted
	// The maxFiles is not respected for the files which have overlapping start & end time becuase
	// such files must be sorted together.
	files := [][]string{}
	var prevStart, prevEnd time.Time

	temp := []string{}
	for i := 0; i < len(s); i++ {
		st := s[i].s
		et := s[i].e
		if prevStart.IsZero() && prevEnd.IsZero() {
			prevStart = st
			prevEnd = et
			temp = append(temp, s[i].name)
			continue
		}

		if st.After(prevEnd) {
			files = append(files, temp)
			temp = []string{}
			temp = append(temp, s[i].name)
			prevStart = st
			prevEnd = et
			continue
		}

		temp = append(temp, s[i].name)
	}

	files = append(files, temp)
	return files
}

type ascendingLogs [][]string

func (s ascendingLogs) Len() int {
	return len(s)
}

func (s ascendingLogs) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s ascendingLogs) Less(i, j int) bool {
	// t1, err := time.Parse(time.RFC3339, s[i][4])
	// if err != nil {
	// 	panic(err)
	// }

	// t2, err := time.Parse(time.RFC3339, s[j][4])
	// if err != nil {
	// 	panic(err)
	// }
	// return t1.Before(t2)

	return s[i][4] < s[j][4]
}

type descendingLogs [][]string

func (s descendingLogs) Len() int {
	return len(s)
}

func (s descendingLogs) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s descendingLogs) Less(i, j int) bool {
	// t1, err := time.Parse(time.RFC3339, s[i][4])
	// if err != nil {
	// 	panic(err)
	// }

	// t2, err := time.Parse(time.RFC3339, s[j][4])
	// if err != nil {
	// 	panic(err)
	// }
	// return t2.Before(t1)

	return s[i][4] > s[j][4]
}

type indexRecreationStatus int32

const (
	// indexRecreationFinished tells if index recreation has finished after Spyglass restart
	indexRecreationFinished indexRecreationStatus = iota

	// indexRecreationRunning tells if index recreation is running after Spyglass restart
	indexRecreationRunning

	// indexRecreationFailed tells if index recreation has failed after Spyglass restart
	indexRecreationFailed

	// indexRecreationFinishedWithError tells if index recreation has finished with some error
	// after Spyglass restart
	indexRecreationFinishedWithError
)
