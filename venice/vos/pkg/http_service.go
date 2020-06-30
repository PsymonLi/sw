package vospkg

import (
	"context"
	"crypto/tls"
	"encoding/json"
	"expvar"
	"fmt"
	"io"
	"io/ioutil"
	"net"
	"net/http"
	"time"

	"github.com/pensando/sw/venice/utils/k8s"
	"github.com/pensando/sw/venice/utils/ratelimit"

	"github.com/go-martini/martini"
	"github.com/minio/minio-go/v6"
	jose "gopkg.in/square/go-jose.v2"
	"gopkg.in/square/go-jose.v2/jwt"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/objstore"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
	minioclient "github.com/pensando/sw/venice/utils/objstore/minio/client"
	"github.com/pensando/sw/venice/vos"
)

const (
	apiPrefix              = "/apis/v1"
	uploadImagesPath       = "/uploads/images/"
	uploadSnapshotsPath    = "/uploads/snapshots/"
	downloadPath           = "/downloads/images/**"
	bucketDiskThresholdKey = "bucketDiskThresholds"
)

type httpHandler struct {
	client      vos.BackendClient
	adminClient minioclient.AdminClient
	handler     *martini.ClassicMartini
	server      http.Server
	instance    *instance
}

func newHTTPHandler(instance *instance,
	client vos.BackendClient, adminClient minioclient.AdminClient) (*httpHandler, error) {
	log.InfoLog("msgs", "creating new HTTP backend", "port", globals.VosHTTPPort)
	mux := martini.Classic()
	return &httpHandler{client: client, adminClient: adminClient, handler: mux, instance: instance}, nil
}

func (h *httpHandler) start(ctx context.Context, port, minioKey, minioSecret string, config *tls.Config) {
	log.InfoLog("msg", "starting HTTP listener")
	h.handler.Get(apiPrefix+downloadPath, h.downloadHandler)
	log.InfoLog("msg", "adding path", "path", apiPrefix+uploadImagesPath)
	h.handler.Post(apiPrefix+uploadImagesPath, h.uploadImagesHandler)
	log.InfoLog("msg", "adding path", "path", apiPrefix+uploadSnapshotsPath)
	h.handler.Post(apiPrefix+uploadSnapshotsPath, h.uploadSnapshotsHandler)
	log.InfoLog("msg", "adding path", "path", "/debug/vars")
	h.handler.Get("/debug/vars", expvar.Handler())
	log.InfoLog("msg", "adding path", "path", "/debug/minio/metrics")
	h.handler.Get("/debug/minio/metrics", h.minioMetricsHandler(minioKey, minioSecret))
	log.InfoLog("msg", "adding path", "path", "/debug/minio/credentials")
	h.handler.Get("/debug/minio/credentials", h.minioCredentialsHandler(minioKey, minioSecret))
	log.InfoLog("msg", "adding path", "path", "/debug/config")
	h.handler.Get("/debug/config", h.debugConfigHandler())
	h.handler.Post("/debug/config", h.debugConfigHandler())
	h.handler.Get("/debug/minio/clusterinfo", h.minioClusterInfoHandler())
	h.handler.Get("/debug/minio/datausageinfo", h.minioDataUsageInfoHandler())
	h.handler.Get("/debug/minio/restartservices", h.minioRestartServicesHandler())
	h.handler.Get("/debug/minio/recoverconfigmigration", h.minioRecoverFromConfigMigration())

	done := make(chan error)
	var ln net.Listener
	var err error
	if config != nil {
		ln, err = tls.Listen("tcp", ":"+port, config)
		if err != nil {
			panic("failed to start VOS HTTP server")
		}
	} else {
		ln, err = net.Listen("tcp", ":"+port)
		if err != nil {
			panic("failed to start VOS HTTP server")
		}
	}

	h.server = http.Server{
		TLSConfig: config,
		Handler:   h.handler,
	}
	go func() {
		close(done)
		h.server.Serve(ln)
	}()
	<-done
	go func() {
		<-ctx.Done()
		h.server.Close()
	}()
}

// ClusterCreateHandler handles the REST call for cluster creation.
func (h *httpHandler) uploadImagesHandler(w http.ResponseWriter, req *http.Request) {
	h.uploadHandler(w, req, objstore.Buckets_images.String())
}

// ClusterCreateHandler handles the REST call for cluster creation.
func (h *httpHandler) uploadSnapshotsHandler(w http.ResponseWriter, req *http.Request) {
	h.uploadHandler(w, req, objstore.Buckets_snapshots.String())
}

// uploadHandler handles all uploads proxy requests.
func (h *httpHandler) uploadHandler(w http.ResponseWriter, req *http.Request, nspace string) {
	log.Infof("got Upload call")
	if req.Method == "POST" {
		req.ParseMultipartForm(32 << 20)
		file, header, err := req.FormFile("file")
		if err != nil {
			log.Errorf("could not get the file from form")
			h.writeError(w, http.StatusBadRequest, "could not get the file from form")
			return
		}
		contentType := req.FormValue("content-type")
		if contentType == "" {
			contentType = "application/octet-stream"
		}
		meta := make(map[string]string)
		for k, v := range req.Form {
			if k != metaFileName && k != metaCreationTime && k != metaContentType {
				for i := range v {
					if _, ok := meta[k]; ok {
						meta[fmt.Sprintf("%s-%d", k, i)] = v[i]
					} else {
						meta[k] = v[i]
					}
				}
				meta[k] = v[0]
			}
		}

		// Request should be authorized again at the backend because this is a proxied call.
		// XXX-TBD(sanjayt): add once the ueer permission are added to the HTTP request by the API Gateway

		log.Infof("got meta in form  as [%+v]", meta)
		// Check if we have the object
		bucket := "default." + nspace
		stat, err := h.client.StatObject(bucket, header.Filename, minio.StatObjectOptions{})
		if err != nil {
			meta[metaCreationTime] = fmt.Sprintf("%s", time.Now().Format(time.RFC3339Nano))
		} else {
			meta[metaCreationTime] = stat.Metadata.Get(metaPrefix + metaCreationTime)
		}
		errs := h.instance.RunPlugins(context.TODO(), nspace, vos.PreOp, vos.Upload, nil, h.client)
		if errs != nil {
			h.writeError(w, http.StatusPreconditionFailed, errs)
			return
		}
		lreader := ratelimit.NewReader(file, 20*1024*1024, 100*time.Millisecond)

		sz, err := h.client.PutObject(bucket, header.Filename, lreader, -1, minio.PutObjectOptions{UserMetadata: meta, ContentType: contentType, NumThreads: 1})
		if err != nil {
			log.Errorf("failed to write object (%s)", err)
			h.writeError(w, http.StatusInternalServerError, fmt.Sprintf("error writing object (%s)", err))
			return
		}
		in := objstore.Object{
			TypeMeta:   api.TypeMeta{Kind: "Object"},
			ObjectMeta: api.ObjectMeta{Name: header.Filename, Namespace: nspace},
			Spec:       objstore.ObjectSpec{ContentType: contentType},
			Status: objstore.ObjectStatus{
				Size_:  sz,
				Digest: stat.ETag,
			},
		}
		updateObjectMeta(&stat, &in.ObjectMeta)
		stat, err = h.client.StatObject(bucket, header.Filename, minio.StatObjectOptions{})
		if err != nil {
			h.writeError(w, http.StatusInternalServerError, fmt.Sprintf("error verifying image write (%s)", err))
			return
		}
		errs = h.instance.RunPlugins(context.TODO(), nspace, vos.PostOp, vos.Upload, &in, h.client)
		if errs != nil {
			h.writeError(w, http.StatusInternalServerError, errs)
			return
		}
		log.Infof("Wrote object [%v] of size [%v]", header.Filename, sz)

		ret := objstore.Object{
			TypeMeta:   api.TypeMeta{Kind: "Object"},
			ObjectMeta: api.ObjectMeta{Name: header.Filename, Namespace: nspace},
			Spec:       objstore.ObjectSpec{ContentType: contentType},
			Status: objstore.ObjectStatus{
				Size_:  sz,
				Digest: stat.ETag,
			},
		}
		if stat.Size != sz {
			h.writeError(w, http.StatusInternalServerError, fmt.Sprintf("error in object size after write written[%d] readback[%d]", sz, stat.Size))
			return
		}
		updateObjectMeta(&stat, &ret.ObjectMeta)
		b, err := json.Marshal(&ret)
		if err != nil {
			h.writeError(w, http.StatusInternalServerError, fmt.Sprintf("error marshalling return value (%s)", err))
			return
		}
		w.WriteHeader(http.StatusOK)
		w.Write(b)
	} else {
		h.writeError(w, http.StatusMethodNotAllowed, fmt.Sprintf("upsupported method [%v]", req.Method))
	}
}

func (h *httpHandler) downloadHandler(params martini.Params, w http.ResponseWriter, req *http.Request) {
	log.Infof("got Upload call")
	path := params["_1"]
	if path == "" {
		h.writeError(w, http.StatusBadRequest, "empty path")
		return
	}
	// Only supports default.images for now.
	bucket := "default.images"
	buf := make([]byte, 1024*1024)
	ctx := context.Background()
	fr, err := h.client.GetStoreObject(ctx, bucket, path, minio.GetObjectOptions{})
	if err != nil {
		log.Errorf("failed to get object [%v]", path)
		h.writeError(w, http.StatusNotFound, "unknown object")
		return
	}
	errs := h.instance.RunPlugins(ctx, "images", vos.PreOp, vos.Download, nil, h.client)
	if errs != nil {
		h.writeError(w, http.StatusPreconditionFailed, errs)
		return
	}
	w.Header().Set("Content-Disposition", "attachment")
	w.Header().Set("Content-Type", "application/octet-stream; charset=utf-8")
	totsize := 0
	for {
		n, err := fr.Read(buf)
		if err != nil && err != io.EOF {
			log.Errorf("error while reading object (%s)", err)
			h.writeError(w, http.StatusInternalServerError, fmt.Sprintf("error reading object: (%s)", err))
			return
		}
		if n == 0 {
			break
		}
		totsize += n
		w.Write(buf[:n])
	}
	errs = h.instance.RunPlugins(ctx, "images", vos.PostOp, vos.Download, nil, h.client)
	if errs != nil {
		h.writeError(w, http.StatusInternalServerError, errs)
		return
	}
	w.WriteHeader(http.StatusOK)
}

func (h *httpHandler) writeError(w http.ResponseWriter, code int, msg interface{}) {
	w.WriteHeader(code)
	w.Write([]byte(fmt.Sprintf("%v", msg)))
}

func (h *httpHandler) minioMetricsHandler(minioKey, minioSecret string) func(w http.ResponseWriter, req *http.Request) {
	return func(w http.ResponseWriter, req *http.Request) {
		token, err := generateTokenForMetrics(minioKey, minioSecret)
		if err != nil {
			h.writeError(w, http.StatusInternalServerError, fmt.Sprintf("error in generating token (%s)", err))
			return
		}

		metricsReq, err := http.NewRequest("GET",
			fmt.Sprintf("https://%s:%s/minio/prometheus/metrics", k8s.GetPodIP(), globals.VosMinioPort), nil)
		if err != nil {
			h.writeError(w, http.StatusInternalServerError, fmt.Sprintf("error in creating metrics request (%s)", err))
			return
		}

		metricsReq.Header.Set("Authorization", "Bearer "+token)
		client := &http.Client{Timeout: time.Second * 10}
		resp, err := client.Do(metricsReq)
		if err != nil {
			h.writeError(w, http.StatusInternalServerError, fmt.Sprintf("error in collecting metrics request (%s)", err))
			return
		}

		defer resp.Body.Close()

		body, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			h.writeError(w, http.StatusInternalServerError, fmt.Sprintf("error in reading metrics (%s)", err))
			return
		}

		metrics := map[string]interface{}{"miniometrics": string(body[:])}
		b, err := json.Marshal(metrics)
		if err != nil {
			h.writeError(w, http.StatusInternalServerError, fmt.Sprintf("error in marshalling metrics (%s)", err))
			return
		}

		w.WriteHeader(http.StatusOK)
		w.Write(b)
	}
}

func (h *httpHandler) minioCredentialsHandler(minioKey, minioSecret string) func(w http.ResponseWriter, req *http.Request) {
	return func(w http.ResponseWriter, req *http.Request) {

		minioCredentials := map[string]interface{}{globals.MinioAccessKeyName: minioKey, globals.MinioSecretKeyName: minioSecret}
		b, err := json.Marshal(minioCredentials)
		if err != nil {
			h.writeError(w, http.StatusInternalServerError, fmt.Sprintf("error in marshalling credentials (%s)", err))
			return
		}

		w.WriteHeader(http.StatusOK)
		w.Write(b)
	}
}

func generateTokenForMetrics(accessKey, secretKey string) (string, error) {
	expiry := 60 * time.Second
	claims := jwt.Claims{
		Subject:  accessKey,
		Issuer:   "prometheus",
		Expiry:   jwt.NewNumericDate(time.Now().UTC().Add(expiry)),
		IssuedAt: jwt.NewNumericDate(time.Now()),
	}
	signer, err := jose.NewSigner(jose.SigningKey{Algorithm: jose.HS512, Key: []byte(secretKey)}, nil)
	if err != nil {
		return "", err
	}
	return jwt.Signed(signer).Claims(claims).CompactSerialize()
}

func (h *httpHandler) debugConfigHandler() func(w http.ResponseWriter, req *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		switch r.Method {
		case "GET":
			config := map[string]interface{}{}
			ths := map[string]DiskMonitorConfig{}
			h.instance.bucketDiskThresholds.Range(func(k interface{}, v interface{}) bool {
				ths[k.(string)] = v.(DiskMonitorConfig)
				return true
			})
			config[bucketDiskThresholdKey] = ths

			out, err := json.Marshal(config)
			if err != nil {
				log.Errorf("Error in marshling output: %v", err)
				http.Error(w, "error in marshling output", http.StatusInternalServerError)
			}
			w.WriteHeader(http.StatusOK)
			w.Header().Set("Content-Type", "application/json; charset=utf-8")
			w.Write(out)
		case "POST":
			body, err := ioutil.ReadAll(r.Body)
			if err != nil {
				log.Errorf("Error reading body: %v", err)
				http.Error(w, "can't read body", http.StatusBadRequest)
				return
			}

			config := map[string]interface{}{}
			err = json.Unmarshal(body, &config)
			if err != nil {
				log.Errorf("Error in unmarshaling body: %v", err)
				http.Error(w, "error in unmarshaling body", http.StatusBadRequest)
				return
			}

			h.updateBucketThresholdPaths(w, config)
		default:
			http.Error(w, "only GET and POST are supported", http.StatusBadRequest)
		}
	}
}

func (h *httpHandler) minioClusterInfoHandler() func(w http.ResponseWriter, req *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		out, err := h.adminClient.ClusterInfo(context.Background())
		if err != nil {
			log.Errorf("Error in marshling output: %v", err)
			http.Error(w, "error in marshling output", http.StatusInternalServerError)
		}

		w.WriteHeader(http.StatusOK)
		w.Header().Set("Content-Type", "application/json; charset=utf-8")
		w.Write([]byte(out))
	}
}

func (h *httpHandler) minioDataUsageInfoHandler() func(w http.ResponseWriter, req *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		out, err := h.adminClient.DataUsageInfo(context.Background())
		if err != nil {
			log.Errorf("Error in marshling output: %v", err)
			http.Error(w, "error in marshling output", http.StatusInternalServerError)
		}
		w.WriteHeader(http.StatusOK)
		w.Header().Set("Content-Type", "application/json; charset=utf-8")
		w.Write([]byte(out))
	}
}

func (h *httpHandler) minioRestartServicesHandler() func(w http.ResponseWriter, req *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		err := h.adminClient.ServiceRestart(context.Background())
		if err != nil {
			log.Errorf("Error in marshling output: %v", err)
			http.Error(w, "error in marshling output", http.StatusInternalServerError)
		}
		w.WriteHeader(http.StatusOK)
	}
}

func (h *httpHandler) minioRecoverFromConfigMigration() func(w http.ResponseWriter, req *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		err := recoverMinioHelper(context.Background(), h.adminClient)
		if err != nil {
			log.Errorf("Error in marshling output: %v", err)
			http.Error(w, "error in marshling output", http.StatusInternalServerError)
		}
		w.WriteHeader(http.StatusOK)
	}
}

func (h *httpHandler) updateBucketThresholdPaths(w http.ResponseWriter, config map[string]interface{}) {
	if v, ok := config[bucketDiskThresholdKey]; ok {
		data, ok := v.(map[string]interface{})
		if !ok {
			http.Error(w, "bucket disk threshold config should be in form map[string]float64", http.StatusBadRequest)
			return
		}
		for p, v := range data {
			t, ok := v.(map[string]interface{})
			if !ok {
				http.Error(w, "bucket disk threshold config should be in form map[string]DiskMonitorConfig", http.StatusBadRequest)
				return
			}

			c := DiskMonitorConfig{}
			for k, v := range t {
				switch k {
				case "buckets":
					for _, b := range v.([]interface{}) {
						c.CombinedBuckets = append(c.CombinedBuckets, b.(string))
					}
				case "tenant":
					c.TenantName = v.(string)
				case "threshold":
					c.CombinedThresholdPercent = v.(float64)
				}
			}
			h.instance.bucketDiskThresholds.Store(p, c)
		}
	}
}
