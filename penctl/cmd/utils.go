//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strings"
)

func isUserSureToProceedWithAction(s string) bool {
	reader := bufio.NewReader(os.Stdin)
	for {
		fmt.Printf("%s [y/n]: ", s)
		response, err := reader.ReadString('\n')
		if err != nil {
			log.Fatal(err)
		}
		response = strings.ToLower(strings.TrimSpace(response))
		if response == "y" || response == "yes" {
			return true
		} else if response == "n" || response == "no" {
			return false
		}
	}
}

func naplesExecCmdNoPrint(v interface{}) error {
	_, err := restGetWithBody(v, "cmd/v1/naples/")
	if err != nil {
		if verbose {
			fmt.Println(err)
		}
		return err
	}
	return nil
}

func naplesExecCmd(v interface{}) error {
	resp, err := restGetWithBody(v, "cmd/v1/naples/")
	if err != nil {
		if verbose {
			fmt.Println(err)
		}
		return err
	}
	if len(resp) > 3 {
		fmt.Println(strings.Replace(string(resp), `\n`, "\n", -1))
	}
	return nil
}

func stripURLScheme(url string) string {
	if strings.HasPrefix(url, "https://") {
		return strings.TrimPrefix(url, "https://")
	} else if strings.HasPrefix(url, "http://") {
		return strings.TrimPrefix(url, "http://")
	}
	return url
}
