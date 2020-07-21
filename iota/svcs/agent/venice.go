package agent

import (
	"context"
	"fmt"
	"os"
	"strings"
	"time"

	"github.com/onsi/ginkgo"
	"github.com/pkg/errors"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/api/generated/apiclient"
	"github.com/pensando/sw/api/generated/auth"
	"github.com/pensando/sw/api/generated/cluster"
	loginctx "github.com/pensando/sw/api/login/context"
	iota "github.com/pensando/sw/iota/protos/gogen"
	utils "github.com/pensando/sw/iota/svcs/agent/utils"
	Common "github.com/pensando/sw/iota/svcs/common"
	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/authn/testutils"
	authntestutils "github.com/pensando/sw/venice/utils/authn/testutils"
	log "github.com/pensando/sw/venice/utils/log"
)

const (
	veniceStartScript   = "INSTALL.sh"
	veniceInstallLog    = "install.log"
	maxClusterInitRetry = 60
)

type veniceNode struct {
	commandNode
	authToken string
	peers     []string
	licenses  []string
}

type veniceBMNode struct {
	commandNode
}

type venicePeerNode struct {
	hostname string
	ip       string
}

func (venice *veniceNode) bringUpVenice(image string, hostname string,
	ctrlIntf string, ctrlIP string, reload bool, peers []venicePeerNode) error {

	if ctrlIntf != "" {
		utils.DisableDhcpOnInterface(ctrlIntf)
		venice.logger.Println("Configuring intf : " + ctrlIntf + " with " + ctrlIP)
		ifConfigCmd := []string{"ifconfig", ctrlIntf, ctrlIP, "up"}
		if _, stdout, err := utils.Run(ifConfigCmd, 0, false, true, nil); err != nil {
			errors.New("Setting control interface IP to venice node failed.." + stdout)
		}
	}

	// if this is a reload, we are done
	if reload {
		return nil
	}

	curDir, _ := os.Getwd()
	defer os.Chdir(curDir)
	os.Chdir(Common.ImageArtificatsDirectory)
	venice.logger.Println("Untar image : " + image)
	untar := []string{"tar", "-xvzf", image}
	if _, stdout, err := utils.Run(untar, 0, false, false, nil); err != nil {
		return errors.Wrap(err, stdout)
	}

	for i := 0; true; i++ {
		setHostname := []string{"hostnamectl", "set-hostname", "--static", hostname}
		if stderr, stdout, err := utils.Run(setHostname, 0, false, false, nil); err != nil {
			venice.logger.Printf("Setting hostname failed %v %v %v", stderr, stdout, err.Error())
			if i == 5 {
				return errors.Wrap(err, stdout)
			}
			time.Sleep(1 * time.Second)
			continue
		}
		break
	}

	venice.logger.Println("Running Install Script : " + veniceStartScript)
	install := []string{"./" + veniceStartScript, "--clean", "|", "tee", veniceInstallLog}
	if _, stdout, err := utils.Run(install, 0, false, true, nil); err != nil {
		venice.logger.Println("Running Install Script failed : " + veniceStartScript)
		return errors.Wrap(err, stdout)
	}

	cmd := []string{"touch", "/etc/hosts"}
	utils.Run(cmd, 0, false, true, nil)

	for _, peer := range peers {
		if peer.hostname != "" && peer.ip != "" {
			cmd := []string{"echo", strings.Split(peer.ip, "/")[0], peer.hostname, " | sudo tee -a /etc/hosts"}
			if _, stdout, err := utils.Run(cmd, 0, false, true, nil); err != nil {
				venice.logger.Println("Setting venice peer hostnames failed")
				return errors.Wrap(err, stdout)
			}
		}
	}
	return nil
}

// SetupLicenses setsup licesses
func (venice *veniceNode) setupLicenses(licenses []string) error {
	// set bootstrap flag

	venice.logger.Infof("Setting up licenses %v", len(licenses))
	if len(licenses) == 0 {
		return nil
	}

	apicl, err := apiclient.NewRestAPIClient(venice.peers[0])
	if err != nil {
		return fmt.Errorf("cannot create rest client, err: %v", err)
	}

	features := []cluster.Feature{}
	for _, license := range licenses {
		venice.logger.Infof("Adding license %v", license)
		features = append(features, cluster.Feature{FeatureKey: license})
	}

	_, err = testutils.CreateLicense(apicl, features)
	if err != nil {
		// 401 when auth is already bootstrapped. we are ok with that
		if !strings.HasPrefix(err.Error(), "Status:(401)") {
			ginkgo.Fail(fmt.Sprintf("SetAuthBootstrapFlag failed with err: %v", err))
		}
	}

	return nil
}

// SetupAuth bootstraps default tenant, authentication policy, local user and super admin role
func (venice *veniceNode) SetupAuth(userID, password string) error {

	apicl, err := apiclient.NewRestAPIClient(venice.peers[0])
	if err != nil {
		return fmt.Errorf("cannot create rest client, err: %v", err)
	}

	// create tenant. default roles (admin role) are created automatically when a tenant is created
	_, err = testutils.CreateTenant(apicl, globals.DefaultTenant)
	if err != nil {
		// 412 is returned when tenant and default roles already exist. 401 when auth is already bootstrapped. we are ok with that
		// already exists
		if !strings.HasPrefix(err.Error(), "Status:(412)") && !strings.HasPrefix(err.Error(), "Status:(401)") &&
			!strings.HasPrefix(err.Error(), "already exists") {
			return fmt.Errorf("CreateTenant failed with err: %v", err)
		}
	}

	// create authentication policy with local auth enabled
	_, err = testutils.CreateAuthenticationPolicyWithOrder(apicl, &auth.Local{}, nil, nil, []string{auth.Authenticators_LOCAL.String()}, testutils.ExpiryDuration)
	if err != nil {
		// 409 is returned when authpolicy already exists. 401 when auth is already bootstrapped. we are ok with that
		if !strings.HasPrefix(err.Error(), "Status:(409)") && !strings.HasPrefix(err.Error(), "Status:(401)") &&
			!strings.HasPrefix(err.Error(), "already exists") {
			return fmt.Errorf("CreateAuthenticationPolicy failed with err: %v", err)
		}
	}

	// create user is only allowed after auth policy is created and local auth is enabled
	_, err = testutils.CreateTestUser(context.TODO(), apicl, userID, password, globals.DefaultTenant)
	if err != nil {
		// 409 is returned when user already exists. 401 when auth is already bootstrapped. we are ok with that
		if !strings.HasPrefix(err.Error(), "Status:(409)") && !strings.HasPrefix(err.Error(), "Status:(401)") &&
			!strings.HasPrefix(err.Error(), "already exists") {
			return fmt.Errorf("CreateTestUser failed with err: %v", err)
		}
	}

	// update admin role binding
	_, err = testutils.UpdateRoleBinding(context.TODO(), apicl, globals.AdminRoleBinding, globals.DefaultTenant, globals.AdminRole, []string{userID}, nil)
	if err != nil {
		// 401 when auth is already bootstrapped. we are ok with that
		if !strings.HasPrefix(err.Error(), "Status:(401)") {
			return fmt.Errorf("UpdateRoleBinding failed with err: %v", err)
		}
	}

	//Setup any lines
	err = venice.setupLicenses(venice.licenses)
	if err != nil {
		return err
	}

	// set bootstrap flag
	_, err = testutils.SetAuthBootstrapFlag(apicl)
	if err != nil {
		// 401 when auth is already bootstrapped. we are ok with that
		if !strings.HasPrefix(err.Error(), "Status:(401)") {
			ginkgo.Fail(fmt.Sprintf("SetAuthBootstrapFlag failed with err: %v", err))
		}
	}

	return nil
}

// GetVeniceURL returns venice URL for the sysmodel
func (venice *veniceNode) GetVeniceURL() []string {
	var veniceURL []string

	// walk all venice nodes
	for _, node := range venice.peers {
		veniceURL = append(veniceURL, fmt.Sprintf("%s:%s", node, globals.APIGwRESTPort))
	}

	return veniceURL
}

// VeniceNodeLoggedInCtx logs in to a specified node and returns loggedin context
func (venice *veniceNode) veniceNodeLoggedInCtxWithURL(nodeURL string) (context.Context, error) {
	// local user credentials

	userCred := auth.PasswordCredential{
		Username: "admin",
		Password: Common.UserPassword,
		Tenant:   "default",
	}

	// try to login
	ctx, err := authntestutils.NewLoggedInContext(context.Background(), nodeURL, &userCred)
	if err != nil {
		log.Errorf("Error logging into Venice URL %v. Err: %v", nodeURL, err)
		return nil, err
	}
	authToken, ok := loginctx.AuthzHeaderFromContext(ctx)
	if ok {
		venice.authToken = authToken
	} else {
		return nil, fmt.Errorf("auth token not available in logged-in context")
	}

	return ctx, nil
}

// VeniceLoggedInCtx returns loggedin context for venice taking a context
func (venice *veniceNode) VeniceLoggedInCtx(ctx context.Context) (context.Context, error) {
	var err error

	for _, url := range venice.GetVeniceURL() {
		_, err = venice.veniceNodeLoggedInCtxWithURL(url)
		if err == nil {
			break
		}
	}
	if err != nil {
		return nil, err
	}
	return loginctx.NewContextWithAuthzHeader(ctx, venice.authToken), nil

}

// VeniceRestClient returns the REST client for venice
func (venice *veniceNode) VeniceRestClient() ([]apiclient.Services, error) {
	var restcls []apiclient.Services
	for _, url := range venice.GetVeniceURL() {
		// connect to Venice
		restcl, err := apiclient.NewRestAPIClient(url)
		if err != nil {
			log.Errorf("Error connecting to Venice %v. Err: %v", url, err)
			return nil, err
		}

		restcls = append(restcls, restcl)
	}

	return restcls, nil
}

func (venice *veniceNode) WaitForVeniceClusterUp(ctx context.Context) error {
	// wait for cluster to come up
	for i := 0; i < maxVeniceUpWait; i++ {
		restcls, err := venice.VeniceRestClient()
		if err == nil {
			ctx2, err := venice.VeniceLoggedInCtx(ctx)
			if err == nil {
				for _, restcl := range restcls {
					_, err = restcl.ClusterV1().Cluster().Get(ctx2, &api.ObjectMeta{Name: "iota-cluster"})
					if err == nil {
						return nil
					}
				}
			}
		}

		time.Sleep(time.Second)
		if e := ctx.Err(); e != nil {
			return e
		}
	}

	// if we reached here, it means we werent able to connect to Venice API GW
	return fmt.Errorf("Failed to connect to Venice")
}

const maxVeniceUpWait = 300

// InitVeniceConfig initializes base configuration for venice
func (venice *veniceNode) initVeniceConfig(ctx context.Context) error {

	venice.logger.Infof("Setting up Auth on Venice cluster...")

	var err error
	for i := 0; i < maxVeniceUpWait; i++ {
		err = venice.SetupAuth("admin", Common.UserPassword)
		if err == nil {
			break
		}
		time.Sleep(time.Second)
		if ctx.Err() != nil {
			return ctx.Err()
		}
	}
	if err != nil {
		log.Errorf("Setting up Auth failed. Err: %v", err)
		return err
	}

	venice.logger.Infof("Auth setup complete...")

	// wait for venice cluster to come up
	return venice.WaitForVeniceClusterUp(ctx)
}

// MakeVeniceCluster inits the venice cluster
func (venice *veniceNode) makeVeniceCluster(ctx context.Context) error {
	// get CMD URL URL
	var veniceCmdURL []string
	var err error
	var response string
	for _, node := range venice.peers {
		veniceCmdURL = append(veniceCmdURL, fmt.Sprintf("%s:9001", node))
	}

	// cluster message to init cluster
	clusterCfg := cluster.Cluster{
		TypeMeta: api.TypeMeta{Kind: "Cluster"},
		ObjectMeta: api.ObjectMeta{
			Name: "iota-cluster",
		},
		Spec: cluster.ClusterSpec{
			AutoAdmitDSCs: true,
			QuorumNodes:   venice.peers,
			NTPServers:    []string{"pool.ntp.org"},
		},
	}

	// make cluster message to be sent to API server

	venice.logger.Infof("Making Venice cluster..")
	ep := veniceCmdURL[0] + "/api/v1/cluster"
	// retry cluster init few times
	for i := 0; i < maxClusterInitRetry; i++ {
		_, response, err = Common.HTTPPost(ep, "", &clusterCfg)
		venice.logger.Infof("CFG SVC | DEBUG | MakeCluster. Received REST Response Msg: %v, err: %v", response, err)
		if err == nil {
			break
		}

		// sleep for a second and retry again
		time.Sleep(time.Second)
	}

	return err
}

//Init initalize node type
func (venice *veniceNode) Init(in *iota.Node) (*iota.Node, error) {
	venice.commandNode.Init(in)
	venice.iotaNode.name = in.GetName()
	venice.iotaNode.nodeMsg = in
	venice.logger.Printf("Bring up request received for : %v. Req: %+v", in.GetName(), in)

	veniceNodes := []venicePeerNode{}

	for _, node := range in.GetVeniceConfig().GetVenicePeers() {
		veniceNodes = append(veniceNodes, venicePeerNode{hostname: node.GetHostName(),
			ip: node.GetIpAddress()})
		venice.peers = append(venice.peers, node.GetIpAddress())
	}

	if err := venice.bringUpVenice(in.GetImage(), in.GetName(),
		in.GetVeniceConfig().GetControlIntf(), in.GetVeniceConfig().GetControlIp(),
		in.GetReload(), veniceNodes); err != nil {
		venice.logger.Println("Venice bring up failed.")
		msg := fmt.Sprintf("Venice bring up failed. %v", err.Error())
		venice.logger.Println(msg)
		return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR,
			ErrorMsg: msg,
		}}, err

	}

	if !in.Reload && in.GetVeniceConfig().GetClusterInfo() != nil {
		venice.licenses = in.GetVeniceConfig().GetClusterInfo().GetLicenses()
		ctx, cancel := context.WithTimeout(context.TODO(), 30*time.Minute)
		defer cancel()
		if err := venice.makeVeniceCluster(ctx); err != nil {
			venice.logger.Println("Venice make cluster failed")
			msg := fmt.Sprintf("Venice make cluster failed. %v", err.Error())
			venice.logger.Println(msg)
			return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR,
				ErrorMsg: msg,
			}}, err

		}

		if err := venice.initVeniceConfig(ctx); err != nil {
			venice.logger.Println("init venice config failed")
			msg := fmt.Sprintf("init venice config failed %v", err.Error())
			venice.logger.Println(msg)
			return &iota.Node{NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_SERVER_ERROR,
				ErrorMsg: msg,
			}}, err
		}
	}

	venice.logger.Println("Venice bring script up successful.")

	dir := Common.DstIotaEntitiesDir + "/" + in.GetName()
	os.Mkdir(dir, 0765)
	os.Chmod(dir, 0777)

	return &iota.Node{Name: in.Name, IpAddress: in.IpAddress, Type: in.GetType(),
		NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_STATUS_OK}}, nil
}

// AddWorkload brings up a workload type on a given node
func (venice *veniceNode) AddWorkloads(*iota.WorkloadMsg) (*iota.WorkloadMsg, error) {
	venice.logger.Println("Add workload on venice not supported.")
	return &iota.WorkloadMsg{ApiResponse: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST}}, nil
}

// DeleteWorkloads deletes a given workloads
func (venice *veniceNode) DeleteWorkloads(*iota.WorkloadMsg) (*iota.WorkloadMsg, error) {
	venice.logger.Println("Delete workload on venice not supported.")
	return &iota.WorkloadMsg{ApiResponse: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_BAD_REQUEST}}, nil
}

func (venice *veniceBMNode) Init(in *iota.Node) (*iota.Node, error) {
	venice.commandNode.Init(in)
	venice.iotaNode.name = in.GetName()
	venice.iotaNode.nodeMsg = in
	venice.logger.Printf("Bring up request received for BM Venice node : %v. Req: %+v", in.GetName(), in)

	dir := Common.DstIotaEntitiesDir + "/" + in.GetName()
	os.Mkdir(dir, 0765)
	os.Chmod(dir, 0777)

	return &iota.Node{Name: in.Name, IpAddress: in.IpAddress, Type: in.GetType(),
		NodeStatus: &iota.IotaAPIResponse{ApiStatus: iota.APIResponseType_API_STATUS_OK}}, nil
}
