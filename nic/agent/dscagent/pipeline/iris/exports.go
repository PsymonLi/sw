// +build iris

package iris

import (
	"context"
	"fmt"
	"net"
	"strconv"
	"syscall"
	"time"

	"github.com/pkg/errors"
	"golang.org/x/sys/unix"

	"github.com/pensando/sw/api"
	"github.com/pensando/sw/nic/agent/dscagent/pipeline/iris/utils"
	commonUtils "github.com/pensando/sw/nic/agent/dscagent/pipeline/utils"
	"github.com/pensando/sw/nic/agent/dscagent/types"
	halapi "github.com/pensando/sw/nic/agent/dscagent/types/irisproto"
	"github.com/pensando/sw/nic/agent/protos/netproto"
	"github.com/pensando/sw/venice/utils/ipfix"
	"github.com/pensando/sw/venice/utils/log"
)

// Export represents a single export wrt HAL
type Export struct {
	CollectorID      uint64
	CompositeKey     string
	VrfName          string
	Destination      string
	Transport        *netproto.ProtoPort
	Gateway          string
	Interval         string
	TemplateInterval string
}

var templateContextMap = map[string]context.CancelFunc{}

// HandleExport handles crud operations on Export objects
func HandleExport(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, oper types.Operation, export *Export, vrfID uint64) error {
	switch oper {
	case types.Create:
		return createExportHandler(infraAPI, telemetryClient, intfClient, epClient, export, vrfID)
	case types.Update:
		return updateExportHandler(infraAPI, telemetryClient, intfClient, epClient, export, vrfID)
	case types.Delete:
		return deleteExportHandler(infraAPI, telemetryClient, intfClient, epClient, export, vrfID)
	default:
		return errors.Wrapf(types.ErrUnsupportedOp, "Op: %s", oper)
	}
}

func createExportHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, export *Export, vrfID uint64) error {
	var destPort int
	dstIP := export.Destination
	mgmtIP := commonUtils.GetMgmtIP(MgmtLink)
	if err := CreateLateralNetAgentObjects(infraAPI, intfClient, epClient, vrfID, export.CompositeKey, mgmtIP, dstIP, export.Gateway, false); err != nil {
		log.Error(errors.Wrapf(types.ErrNetflowCreateLateralObjects, "ExportConfig: %s | Err: %v", export.CompositeKey, err))
		return errors.Wrapf(types.ErrNetflowCreateLateralObjects, "ExportConfig: %s | Err: %v", export.CompositeKey, err)
	}

	// Create HAL Collector
	l2SegID := getL2SegByCollectorIP(infraAPI, dstIP)
	collectorReqMsg := convertCollector(infraAPI, export, vrfID, l2SegID)
	resp, err := telemetryClient.CollectorCreate(context.Background(), collectorReqMsg)
	if resp != nil {
		if err := utils.HandleErr(types.Create, resp.GetResponse()[0].GetApiStatus(), err, fmt.Sprintf("Collector Create Failed for ExportConfig: %s", export.CompositeKey)); err != nil {
			return err
		}
	}

	templateCtx, cancel := context.WithCancel(context.Background())
	destIP := net.ParseIP(export.Destination)
	if export.Transport == nil {
		destPort = types.DefaultNetflowExportPort
	} else {
		destPort, _ = strconv.Atoi(export.Transport.Port)
	}
	go sendTemplate(templateCtx, infraAPI, destIP, destPort, export)
	templateContextMap[export.CompositeKey] = cancel
	return nil
}

func updateExportHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, export *Export, vrfID uint64) error {
	// Update HAL Collector
	dstIP := export.Destination
	l2SegID := getL2SegByCollectorIP(infraAPI, dstIP)
	collectorUpdateMsg := convertCollector(infraAPI, export, vrfID, l2SegID)
	resp, err := telemetryClient.CollectorUpdate(context.Background(), collectorUpdateMsg)
	if resp != nil {
		if err := utils.HandleErr(types.Update, resp.GetResponse()[0].GetApiStatus(), err, fmt.Sprintf("Collector Update Failed for ExportConfig: %s", export.CompositeKey)); err != nil {
			return err
		}
	}
	return nil
}

func deleteExportHandler(infraAPI types.InfraAPI, telemetryClient halapi.TelemetryClient, intfClient halapi.InterfaceClient, epClient halapi.EndpointClient, export *Export, vrfID uint64) error {
	cKey := convertCollectorKeyHandle(export.CollectorID)
	cancel := templateContextMap[export.CompositeKey]
	if cancel != nil {
		cancel()
	}
	delete(templateContextMap, export.CompositeKey)
	collectorDeleteReq := &halapi.CollectorDeleteRequestMsg{
		Request: []*halapi.CollectorDeleteRequest{
			{
				KeyOrHandle: cKey,
			},
		},
	}
	cResp, err := telemetryClient.CollectorDelete(context.Background(), collectorDeleteReq)
	if cResp != nil {
		if err := utils.HandleErr(types.Delete, cResp.GetResponse()[0].GetApiStatus(), err, fmt.Sprintf("FlowMonitorRule Delete Failed for collector %s | %s", export.CompositeKey, cKey)); err != nil {
			return err
		}
	}
	if err := DeleteLateralNetAgentObjects(infraAPI, intfClient, epClient, vrfID, export.CompositeKey, export.Destination, export.Gateway, false); err != nil {
		log.Error(errors.Wrapf(types.ErrNetflowDeleteLateralObjects, "ExportConfig: %s | Err: %v", export.CompositeKey, err))
	}
	return nil
}

func getL2SegByCollectorIP(i types.InfraAPI, destIP string) (l2SegID uint64) {
	eDat, err := i.List("Endpoint")
	if err != nil {
		log.Error(errors.Wrapf(types.ErrBadRequest, "Err: %v", types.ErrObjNotFound))
		l2SegID = types.UntaggedCollVLAN
		return
	}

	pip := net.ParseIP(destIP).String()
	for _, o := range eDat {
		var endpoint netproto.Endpoint
		err := endpoint.Unmarshal(o)
		if err != nil {
			log.Error(errors.Wrapf(types.ErrUnmarshal, "Endpoint: %s | Err: %v", endpoint.GetKey(), err))
			continue
		}
		for _, address := range endpoint.Spec.IPv4Addresses {
			epIP, _, _ := net.ParseCIDR(address)
			if epIP.String() == pip {
				var linkedNetwork netproto.Network
				obj := netproto.Network{
					TypeMeta: api.TypeMeta{
						Kind: "Network",
					},
					ObjectMeta: api.ObjectMeta{
						Name:      endpoint.Spec.NetworkName,
						Tenant:    "default",
						Namespace: "default",
					},
				}
				dat, err := i.Read(obj.Kind, obj.GetKey())
				if err != nil {
					log.Error(errors.Wrapf(types.ErrBadRequest, "Err: %v", types.ErrObjNotFound))
					l2SegID = types.UntaggedCollVLAN
					return
				}
				err = linkedNetwork.Unmarshal(dat)
				l2SegID = linkedNetwork.Status.NetworkID
				return
			}
		}
	}
	l2SegID = types.UntaggedCollVLAN
	return
}

func convertCollector(infraAPI types.InfraAPI, export *Export, vrfID uint64, l2SegID uint64) *halapi.CollectorRequestMsg {
	var port uint64
	var protocol halapi.IPProtocol
	mgmtIP := commonUtils.GetMgmtIP(MgmtLink)
	srcIP := utils.ConvertIPAddresses(mgmtIP)[0]
	dstIP := utils.ConvertIPAddresses(export.Destination)[0]
	if export.Transport != nil {
		port, _ = strconv.ParseUint(export.Transport.GetPort(), 10, 64)
		protocol = halapi.IPProtocol(convertProtocol(export.Transport.Protocol))
	}
	interval, _ := time.ParseDuration(export.Interval)

	// TODO fix export destination to use the same protocol ports

	return &halapi.CollectorRequestMsg{
		Request: []*halapi.CollectorSpec{
			{
				KeyOrHandle:  convertCollectorKeyHandle(export.CollectorID),
				VrfKeyHandle: convertVrfKeyHandle(vrfID),
				Encap: &halapi.EncapInfo{
					EncapType:  halapi.EncapType_ENCAP_TYPE_DOT1Q,
					EncapValue: types.UntaggedCollVLAN,
				},
				L2SegKeyHandle: convertL2SegKeyHandle(l2SegID),
				SrcIp:          srcIP,
				DestIp:         dstIP,
				Protocol:       protocol,
				DestPort:       uint32(port),
				ExportInterval: uint32(interval.Seconds()),
				Format:         halapi.ExportFormat_IPFIX,
			},
		},
	}
}

// TODO Remove this once the HAL side telemetry code is cleaned up. Agents must not be sending raw packets on sockets
func sendTemplate(ctx context.Context, infraAPI types.InfraAPI, destIP net.IP, destPort int, export *Export) {
	log.Infof("FlowExportPolicy | %s Collector: %s", types.InfoTemplateSendStart, destIP.String())
	var templateTicker *time.Ticker
	if len(export.TemplateInterval) > 0 {
		duration, _ := time.ParseDuration(export.TemplateInterval)
		templateTicker = time.NewTicker(duration)
	} else {
		templateTicker = time.NewTicker(types.DefaultTemplateDuration)
	}

	template, err := ipfix.CreateTemplateMsg()
	if err != nil {
		log.Error(errors.Wrapf(types.ErrIPFIXTemplateCreate, "ExportConfig: %s  | Err: %v", export.CompositeKey, err))
		return
	}

	nc := net.ListenConfig{
		Control: func(network, address string, c syscall.RawConn) error {
			var sockErr error
			if err := c.Control(func(fd uintptr) {
				sockErr = unix.SetsockoptInt(int(fd), unix.SOL_SOCKET, unix.SO_REUSEPORT, 1)
			}); err != nil {
				log.Error(errors.Wrapf(types.ErrIPFIXTemplateSockSend, "ExportConfig: %s  | Err: %v", export.CompositeKey, err))
			}
			return sockErr
		},
	}

	mgmtIP := commonUtils.GetMgmtIP(MgmtLink)
	conn, err := nc.ListenPacket(ctx, "udp", fmt.Sprintf("%v:%v", mgmtIP, types.IPFIXSrcPort))
	if err != nil {
		log.Error(errors.Wrapf(types.ErrIPFIXPacketListen, "ExportConfig: %s  | Err: %v", export.CompositeKey, err))
		return
	}
	defer conn.Close()

	for {
		select {
		case <-ctx.Done():
			log.Infof("ExportConfig | %s Collector: %s", types.InfoTemplateSendStop, destIP.String())
			return
		case <-templateTicker.C:
			if _, err := conn.WriteTo(template, &net.UDPAddr{IP: destIP, Port: destPort}); err != nil {
				log.Error(errors.Wrapf(types.ErrIPFIXPacketSend, "ExportConfig: %s  | Err: %v", export.CompositeKey, err))
			}
		}
	}
}

func convertCollectorKeyHandle(collectorID uint64) *halapi.CollectorKeyHandle {
	return &halapi.CollectorKeyHandle{
		KeyOrHandle: &halapi.CollectorKeyHandle_CollectorId{
			CollectorId: collectorID,
		},
	}
}
