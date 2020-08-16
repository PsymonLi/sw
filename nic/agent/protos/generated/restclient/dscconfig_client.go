// Code generated by protoc-gen-grpc-pensando DO NOT EDIT.

/*
Package netproto is a auto generated package.
Input file: dscconfig.proto
*/
package restclient

import (
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/netutils"
)

// DSCConfigList lists all DSCConfig objects
func (cl *AgentClient) DSCConfigList() ([]netproto.DSCConfig, error) {
	var dscconfigList []netproto.DSCConfig

	err := netutils.HTTPGet("http://"+cl.agentURL+"/api/dscconfig/", &dscconfigList)

	return dscconfigList, err
}

// DSCConfigPost creates DSCConfig object
func (cl *AgentClient) DSCConfigCreate(postData netproto.DSCConfig) error {
	var resp Response

	err := netutils.HTTPPost("http://"+cl.agentURL+"/api/dscconfig/", &postData, &resp)

	return err

}

// DSCConfigDelete deletes DSCConfig object
func (cl *AgentClient) DSCConfigDelete(deleteData netproto.DSCConfig) error {
	var resp Response

	err := netutils.HTTPDelete("http://"+cl.agentURL+"/api/dscconfig/default/default/testDeleteDSCConfig", &deleteData, &resp)

	return err
}

// DSCConfigPut updates DSCConfig object
func (cl *AgentClient) DSCConfigUpdate(putData netproto.DSCConfig) error {
	var resp Response

	err := netutils.HTTPPut("http://"+cl.agentURL+"/api/dscconfig/default/default/preCreatedDSCConfig", &putData, &resp)

	return err
}
