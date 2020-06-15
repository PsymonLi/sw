package cmd

import (
	"bytes"
	"context"
	"crypto/tls"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"os"

	"github.com/pkg/errors"
	"github.com/spf13/cobra"
	"golang.org/x/crypto/ssh/terminal"

	"github.com/pensando/sw/api/generated/auth"
	"github.com/pensando/sw/api/generated/tokenauth"
	"github.com/pensando/sw/api/login"
	loginctx "github.com/pensando/sw/api/login/context"
	"github.com/pensando/sw/venice/globals"
)

const authTokenPrefix = "Bearer "

var (
	psmIP         string
	psmPort       string
	tenant        string
	audience      []string
	tokenFilePath string
)

var nodeTokenCmdDescription = `Login to PSM and get node token to access DSC(s)

Sample invocations:
1. Get node-token with wildcard audience ("*")
   > psm/build/bin/psmctl.linux get node-token --audience="*" --psm-ip="10.7.100.79" --psm-port="10001" --token-ouput "/tmp/token2.pem"
2. Get node-token with multiple audience MACs
   > psm/build/bin/psmctl.linux get node-token --audience="mac.addr.one,mac.addr.two" --psm-ip="10.7.100.79" --psm-port="10001" --token-ouput "/tmp/token2.pem"
   > psm/build/bin/psmctl.linux get node-token --audience="mac.addr.one" --audience="mac.addr.two" --psm-ip="10.7.100.79" --psm-port="10001" --token-ouput "/tmp/token2.pem"
`

var getCmd = &cobra.Command{
	Use:   "get",
	Short: "Get PSM resources",
	Long:  "\n-------------------\n Get PSM resources \n-------------------\n",
	Args:  cobra.NoArgs,
}

var getNodeTokenCmd = &cobra.Command{
	Use:   "node-token",
	Short: "Get node token to access DSC(s)",
	Long:  nodeTokenCmdDescription,
	Args:  cobra.NoArgs,
	Run:   getNodeTokenCmdHandler,
}

func init() {
	rootCmd.AddCommand(getCmd)
	getCmd.AddCommand(getNodeTokenCmd)
	getNodeTokenCmd.Flags().StringVar(&psmIP, "psm-ip", "", "IP address to access PSM")
	getNodeTokenCmd.Flags().StringVar(&psmPort, "psm-port", globals.APIGwRESTPort, "Port to access PSM")
	getNodeTokenCmd.Flags().StringVar(&tenant, "tenant", globals.DefaultTenant, "Tenant to which the user belongs")
	getNodeTokenCmd.Flags().StringVar(&tokenFilePath, "token-output", "", "File path to which node authentication token needs to be stored")
	getNodeTokenCmd.Flags().StringSliceVarP(&audience, "audience", "a", []string{"*"}, "Target audience (DSC node IPs) for which node authentication token is needed")
	getNodeTokenCmd.MarkFlagRequired("psm-ip")
	getNodeTokenCmd.MarkFlagRequired("audience")
}

func getNodeTokenCmdHandler(cmd *cobra.Command, args []string) {
	userName, password, err := readUserNameAndPassword()
	if err != nil {
		errorPrintf("Unable to read username/password, err: %v", err)
		return
	}

	loginURL := fmt.Sprintf("https://%s:%s/v1/login", psmIP, psmPort)
	credentials := &auth.PasswordCredential{
		Username: userName,
		Password: password,
		Tenant:   tenant,
	}
	verbosePrintf("Sending login request to %s", loginURL)
	_, psmAuthToken, err := login.UserLogin(context.Background(), loginURL, credentials)
	if err != nil {
		errorPrintf("Unable to login as tenant/user: %s/%s, err: %v", tenant, userName, err)
		return
	}
	verbosePrintf("Successfully logged-in as user %s", userName)
	nodeTokenURL := fmt.Sprintf("https://%s:%s/tokenauth/v1/node", psmIP, psmPort)
	nodeAuthToken, err := fetchNodeAuthToken(nodeTokenURL, psmAuthToken, audience)
	if err != nil {
		errorPrintf("Failed to fetch node token for audience: %v,\n\terr: %v", audience, err)
		return
	}

	if tokenFilePath != "" {
		err = ioutil.WriteFile(tokenFilePath, []byte(nodeAuthToken), 0644)
		if err == nil {
			fmt.Println("Successfully stored node token to file: ", tokenFilePath)
			return
		}
		fmt.Printf("Failed to write token contents to file: %s, err: %v \n", tokenFilePath, err)
	}
	// print token to console when it isn't saved to a file
	fmt.Println("Note: Node token is not being persisted to any file. Use --token-ouput flag to redirect token contents to a file.")
	fmt.Println("Please copy the following token contents to a secure file location:")
	fmt.Printf(nodeAuthToken)
	return
}

func fetchNodeAuthToken(nodeTokenURL, psmAuthToken string, audience []string) (string, error) {
	verbosePrintf("Getting node token for audience: %v", audience)
	tokenReq := &tokenauth.NodeTokenRequest{
		Audience: audience,
	}

	payloadBytes, err := json.Marshal(tokenReq)
	if err != nil {
		return "", errors.Wrap(err, "failed to unmarshal token request")
	}
	if verbose {
		var prettyJSON bytes.Buffer
		err := json.Indent(&prettyJSON, payloadBytes, "", "\t")
		if err != nil {
			fmt.Printf("failed to pretty print token request, err: %v", err)
		} else {
			fmt.Println("Node token request:", string(prettyJSON.Bytes()))
		}
	}
	body := bytes.NewReader(payloadBytes)
	verbosePrintf("Node token request endpoint: %s", nodeTokenURL)
	nodeAuthTokenReq, err := http.NewRequest("GET", nodeTokenURL, body)
	if err != nil {
		return "", errors.Wrap(err, "failed to create http request")
	}
	nodeAuthTokenReq.Header.Add("Authorization", authTokenPrefix+psmAuthToken)
	ctx := loginctx.NewContextWithAuthzHeader(context.Background(), authTokenPrefix+psmAuthToken)
	nodeAuthTokenReq.WithContext(ctx)
	nodeAuthTokenReq.Header.Set("Content-Type", "application/json")
	psmHTTPClient := &http.Client{
		Transport: &http.Transport{
			TLSClientConfig: &tls.Config{
				InsecureSkipVerify: true,
			},
		},
	}
	response, err := psmHTTPClient.Do(nodeAuthTokenReq)
	if err != nil {
		return "", errors.Wrap(err, "failed to fetch token from PSM")
	}
	defer response.Body.Close()
	responseBodyBytes, _ := ioutil.ReadAll(response.Body)
	if response.StatusCode != 200 && (len(responseBodyBytes) > 1) {
		return "", fmt.Errorf("%d %s %s", response.StatusCode, response.Status, string(responseBodyBytes))
	}

	verbosePrintf("Successfully received node token response")
	verbosePrintf("Status: %s", response.Status)
	verbosePrintf("StatusCode: %d", response.StatusCode)
	verbosePrintf("Response body: %s", string(responseBodyBytes))

	var nodeTokenResponse tokenauth.NodeTokenResponse
	err = json.Unmarshal(responseBodyBytes, &nodeTokenResponse)
	if err != nil {
		return "", errors.Wrap(err, "failed to unmarshal node token response")
	}
	return nodeTokenResponse.Token, nil
}

func readUserNameAndPassword() (userName string, password string, e error) {
	// terminal needs to be in raw mode to be able to read username and password
	oldState, err := terminal.MakeRaw(0)
	if err != nil {
		verbosePrintf("unable to switch terminal to raw mode, err: %v", err)
		return "", "", errors.Wrap(err, "unable to switch terminal to raw mode")
	}
	defer terminal.Restore(0, oldState)
	screen := struct {
		io.Reader
		io.Writer
	}{os.Stdin, os.Stdout}
	t := terminal.NewTerminal(screen, "")

	for {
		t.SetPrompt("User name:")
		userName, err = t.ReadLine()
		if err != nil {
			verbosePrintf("unable to read user name, err: %v", err)
			return "", "", errors.Wrap(err, "unable to read user name")
		}
		if len(userName) > 0 {
			break
		}
	}

	for {
		password, err = t.ReadPassword("Password:")
		if err != nil {
			verbosePrintf("unable to read user name, err: %v", err)
			return "", "", errors.Wrap(err, "unable to read user name")
		}
		if len(password) > 0 {
			break
		}
	}

	return userName, password, nil
}
