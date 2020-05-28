package utils

import (
	"fmt"
	"math"
	"os"
	"strings"

	"google.golang.org/grpc"

	"github.com/pensando/sw/venice/globals"
	"github.com/pensando/sw/venice/utils/log"
)

const (
	pegasusGRPCDefaultBaseURL = globals.Localhost
)

// CreateNewGRPCClient creates a grpc connection to HAL
func CreateNewGRPCClient(port string) (*grpc.ClientConn, error) {
	pdsPort := os.Getenv("PDS_MS_GRPC_PORT")
	if pdsPort == "" {
		pdsPort = port
	}
	srvURL := pegasusGRPCDefaultBaseURL + ":" + pdsPort
	var grpcOpts []grpc.DialOption
	grpcOpts = append(grpcOpts, grpc.WithMaxMsgSize(math.MaxInt32-1))
	grpcOpts = append(grpcOpts, grpc.WithInsecure())
	rpcClient, err := grpc.Dial(srvURL, grpcOpts...)
	if err != nil {
		log.Errorf("Creating gRPC Client failed. Server URL: %s", srvURL)
		return nil, err
	}

	return rpcClient, err
}

// PrintHeader prints a CLI header given the format and fields names
func PrintHeader(format string, fields string) {
	var hdrs []interface{}
	strs := strings.Split(fields, ",")
	for _, s := range strs {
		hdrs = append(hdrs, s)
	}
	headerStr := fmt.Sprintf(format, hdrs...)
	fmt.Println(strings.Repeat("-", len(headerStr)))
	fmt.Printf("%s\n", headerStr)
	fmt.Println(strings.Repeat("-", len(headerStr)))
}

// TimeStr converts time in seconds to human readable string
func TimeStr(sec uint32) (res string) {
	wks, sec := sec/604800, sec%604800
	ds, sec := sec/86400, sec%86400
	hrs, sec := sec/3600, sec%3600
	mins, sec := sec/60, sec%60
	CommaRequired := false
	if wks != 0 {
		res += fmt.Sprintf("%d wk", wks)
		CommaRequired = true
	}
	if ds != 0 {
		if CommaRequired {
			res += ", "
		}
		res += fmt.Sprintf("%d d", ds)
		CommaRequired = true
	}
	if hrs != 0 {
		if CommaRequired {
			res += ", "
		}
		res += fmt.Sprintf("%d hr", hrs)
		CommaRequired = true
	}
	if mins != 0 {
		if CommaRequired {
			res += ", "
		}
		res += fmt.Sprintf("%d min", mins)
		CommaRequired = true
	}
	if sec != 0 {
		if CommaRequired {
			res += ", "
		}
		res += fmt.Sprintf("%d sec", sec)
	}
	return
}
