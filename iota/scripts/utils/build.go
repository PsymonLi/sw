package main

import (
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"time"

	"github.com/pkg/errors"

	"github.com/pensando/sw/venice/utils/log"

	"github.com/spf13/cobra"
)

var (
	targetBranch, version, srcFile, dstFile string
)

type buildMeta struct {
	Repo            string
	Branch          string
	Prefix          string
	Label           string
	NextBuildNumber int
}

func getLatestBuildVersionInfo(targetBranch string) (string, int, error) {
	jsonURL := []string{"-s", "http://jobd.pensando.io:3456/tags"}
	out, err := exec.Command("curl", jsonURL...).CombinedOutput()
	outStr := strings.TrimSpace(string(out))
	fmt.Println(fmt.Sprintf("curl output: %s, err: %v\n", outStr, err))
	if err != nil {
		fmt.Println(fmt.Sprintf("curl Error: %s, err: %v\n", outStr, err))
		fmt.Println()
	}
	jsonResponseBytes := []byte(outStr)
	var buildMetas = []*buildMeta{}
	err = json.Unmarshal(jsonResponseBytes, &buildMetas)
	if err != nil {
		return "", 0, fmt.Errorf("Unable to unmarshal response from jobd [%v]", err)
	}
	for _, bm := range buildMetas {
		if bm.Branch == targetBranch {
			return bm.Prefix, bm.NextBuildNumber, nil
		}
	}
	return "", 0, fmt.Errorf("No build metadata found for targetBranch: %s", targetBranch)
}

func downloadBuildArtifcat(version, targetBranch, srcFile, dstFile string) error {

	localFilePath := dstFile
	tryDownloadVersion := func(version string) error {

		log.Errorf("ts:%s Trying to download src artifcat %v , version: %s", time.Now().String(), srcFile, version)
		url := fmt.Sprintf("http://pxe.pensando.io/builds/hourly/%s/%s", version, srcFile)
		jsonURL := []string{url, "--output", localFilePath, "-f"}

		fmt.Println(fmt.Sprintf("curl string: %v\n", jsonURL))
		out, err := exec.Command("curl", jsonURL...).CombinedOutput()
		outStr := strings.TrimSpace(string(out))
		fmt.Println(fmt.Sprintf("curl output: %s, err: %v\n", outStr, err))
		if strings.Contains(outStr, "404 Not Found") || strings.Contains(outStr, " error: ") {
			log.Errorf("ts:%s Error when trying to download Bundle with version: %s, response: %s", time.Now().String(), version, outStr)
			fmt.Println(fmt.Sprintf("curl Error: %s, err: %v\n", outStr, err))
			return errors.New("Error downloading")
		}
		fmt.Println(fmt.Sprintf("curl success: %s\n", outStr))
		fmt.Println()
		return nil
	}

	if version == "" {

		versionPrefix, buildNumber, err := getLatestBuildVersionInfo(targetBranch)
		if err != nil {
			log.Errorf("ts:%s Unable to fetch latest build version information: %s %s", time.Now().String(), targetBranch, err)
			return errors.New("Unable to fetch latest build version information")
		}
		for b := buildNumber; b > 0; b-- {
			version = versionPrefix + "-" + strconv.Itoa(b)
			if err := tryDownloadVersion(version); err == nil {
				return nil
			}
		}
		return errors.New("Download build artifact failed")
	}

	return tryDownloadVersion(version)
}

// RootCmd represents the base command when called without any subcommands
var RootCmd = &cobra.Command{
	Use:   "build-util",
	Short: "Build utility",
	Long:  `Build utility`,
	RunE: func(cmd *cobra.Command, args []string) error {

		if srcFile == "" || dstFile == "" {
			cmd.Usage()
			return errors.New("Invalid command usage")
		}

		if (targetBranch == "" && version == "") || (targetBranch != "" && version != "") {
			cmd.Usage()
			return errors.New("Either Target Branch or version should be specified")
		}

		if err := downloadBuildArtifcat(version, targetBranch, srcFile, dstFile); err != nil {
			log.Error("Failed to clean up esx node")
			return err
		}
		return nil
	},
}

func main() {

	if err := RootCmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}

}

func init() {
	RootCmd.Flags().StringVarP(&targetBranch, "target-branch", "", "", "Target Branch")
	RootCmd.Flags().StringVarP(&version, "version", "", "", "Target Branch")
	RootCmd.Flags().StringVarP(&srcFile, "src-file", "", "", "Source File")
	RootCmd.Flags().StringVarP(&dstFile, "dst-file", "", "", "DestFile")
}
