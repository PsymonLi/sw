//-----------------------------------------------------------------------------
// {C} Copyright 2017 Pensando Systems Inc. All rights reserved
//-----------------------------------------------------------------------------

package cmd

import (
	"context"
	"fmt"
	"os"
	"strings"

	"github.com/spf13/cobra"

	"github.com/pensando/sw/nic/agent/cmd/halctl/utils"
	"github.com/pensando/sw/nic/agent/netagent/datapath/halproto"
)

//IpsecCbID and ClearOnRead for stats
var (
	IpsecCbID   uint64
	ClearOnRead uint32
)

//IpsecEncryptShowCmd show command for encrypt
var IpsecEncryptShowCmd = &cobra.Command{
	Use:         "ipsec-encrypt",
	Short:       "show ipsec-encrypt information",
	Long:        "show ipsec-encrypt information",
	Run:         ipsecEncryptShowCmdHandler,
	Annotations: map[string]string{"techsupport": "false"},
}

//IpsecDecryptShowCmd Decrypt show command
var IpsecDecryptShowCmd = &cobra.Command{
	Use:         "ipsec-decrypt",
	Short:       "show ipsec-decrypt information",
	Long:        "show ipsec-decrypt information",
	Run:         ipsecDecryptShowCmdHandler,
	Annotations: map[string]string{"techsupport": "false"},
}

//IpsecGlobalStatisticsShowCmd Global Stats show command
var IpsecGlobalStatisticsShowCmd = &cobra.Command{
	Use:         "ipsec-global-stats",
	Short:       "show ipsec-global-stats",
	Long:        "show ipsec-global-stats",
	Run:         ipsecGlobalStatisticsShowCmdHandler,
	Annotations: map[string]string{"techsupport": "false"},
}

func init() {
	showCmd.AddCommand(IpsecEncryptShowCmd)
	showCmd.AddCommand(IpsecDecryptShowCmd)
	showCmd.AddCommand(IpsecGlobalStatisticsShowCmd)
	//IpsecEncryptShowCmd.AddCommand(IpsecDecryptShowCmd)
	//IpsecEncryptShowCmd.AddCommand(IpsecGlobalStatisticsShowCmd)
	IpsecEncryptShowCmd.Flags().Uint64Var(&IpsecCbID, "qid", 1, "Specify qid")
	IpsecDecryptShowCmd.Flags().Uint64Var(&IpsecCbID, "qid", 1, "Specify qid")
	IpsecGlobalStatisticsShowCmd.Flags().Uint32Var(&ClearOnRead, "clear-on-read", 1, "Specify clear on read")
}

func ipsecEncryptShowCmdHandler(cmd *cobra.Command, args []string) {
	if !cmd.Flags().Changed("qid") {
		fmt.Printf("Please specify a queue id(qid)...\n")
		return
	}

	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}

	client := halproto.NewIpsecClient(c)

	defer c.Close()

	req := &halproto.IpsecSAEncryptGetRequest{
		KeyOrHandle: &halproto.IpsecSAEncryptKeyHandle{
			KeyOrHandle: &halproto.IpsecSAEncryptKeyHandle_CbId{
				CbId: IpsecCbID,
			},
		},
	}

	IpsecEncryptGetReqMsg := &halproto.IpsecSAEncryptGetRequestMsg{
		Request: []*halproto.IpsecSAEncryptGetRequest{req},
	}
	respMsg, err := client.IpsecSAEncryptGet(context.Background(), IpsecEncryptGetReqMsg)
	if err != nil {
		fmt.Printf("IpsecSAEncryptGet failed. %v\n", err)
		return
	}
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("HAL Returned error status. %v\n", resp.ApiStatus)
			continue
		}

		fmt.Printf("%s\n", strings.Repeat("-", 80))
		fmt.Printf("\nIpsec Encrypt SA \n")
		fmt.Printf("%s\n", strings.Repeat("-", 80))
		showIpsecEncryptSA(resp)
		fmt.Printf("%s\n\n", strings.Repeat("-", 80))
	}

}

func ipsecDecryptShowCmdHandler(cmd *cobra.Command, args []string) {
	if !cmd.Flags().Changed("qid") {
		fmt.Printf("Please specify a queue id(qid)...\n")
		return
	}

	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}

	client := halproto.NewIpsecClient(c)

	defer c.Close()

	req := &halproto.IpsecSADecryptGetRequest{
		KeyOrHandle: &halproto.IpsecSADecryptKeyHandle{
			KeyOrHandle: &halproto.IpsecSADecryptKeyHandle_CbId{
				CbId: IpsecCbID,
			},
		},
	}

	IpsecDecryptGetReqMsg := &halproto.IpsecSADecryptGetRequestMsg{
		Request: []*halproto.IpsecSADecryptGetRequest{req},
	}
	respMsg, err := client.IpsecSADecryptGet(context.Background(), IpsecDecryptGetReqMsg)
	if err != nil {
		fmt.Printf("IpsecSADecryptGet failed. %v\n", err)
		return
	}
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("HAL Returned error status. %v\n", resp.ApiStatus)
			continue
		}

		fmt.Printf("%s\n", strings.Repeat("-", 80))
		fmt.Printf("\nIpsec Decrypt SA \n")
		fmt.Printf("%s\n", strings.Repeat("-", 80))
		showIpsecDecryptSA(resp)
		fmt.Printf("%s\n\n", strings.Repeat("-", 80))
	}

}

func showIpsecEncryptSA(resp *halproto.IpsecSAEncryptGetResponse) {
	spec := resp.GetSpec()

	switch spec.AuthenticationAlgorithm {
	case 1:
		fmt.Printf("%-30s : %-6s\n", "Authentication Algorithm", "AUTHENTICATION_AES_GCM")
	case 2:
		fmt.Printf("%-30s : %-6s\n", "Authentication Algorithm", "AUTHENTICATION_AES_CCM")
	case 3:
		fmt.Printf("%-30s : %-6s\n", "Authentication Algorithm", "AUTHENTICATION_HMAC")
	case 4:
		fmt.Printf("%-30s : %-6s\n", "Authentication Algorithm", "AUTHENTICATION_AES_CBC_SHA")
	}
	fmt.Printf("%-30s : %-6s\n", "Authentication Key", spec.GetAuthenticationKey().GetKey())
	switch spec.EncryptionAlgorithm {
	case 1:
		fmt.Printf("%-30s : %-6s\n", "Encryption Algorithm", "AUTHENTICATION_AES_GCM")
	case 2:
		fmt.Printf("%-30s : %-6s\n", "Encryption Algorithm", "AUTHENTICATION_AES_CCM")
	case 3:
		fmt.Printf("%-30s : %-6s\n", "Encryption Algorithm", "AUTHENTICATION_HMAC")
	case 4:
		fmt.Printf("%-30s : %-6s\n", "Encryption Algorithm", "AUTHENTICATION_AES_CBC_SHA")
	}
	fmt.Printf("%-30s : %-6s\n", "Encryption Key", spec.GetEncryptionKey().GetKey())
	fmt.Printf("%-30s : %-6s\n", "Local GatewayIP", utils.IPAddrToStr(spec.LocalGatewayIp))
	fmt.Printf("%-30s : %-6s\n", "Remote GatewayIP", utils.IPAddrToStr(spec.RemoteGatewayIp))
	fmt.Printf("%-30s : %-6d\n", "SPI", spec.Spi)
	fmt.Printf("%-30s : %-6d\n", "Tunnel Vrf", spec.GetTepVrf().GetVrfId())
	fmt.Printf("%-30s : %-6d\n", "Salt", spec.Salt)
	fmt.Printf("%-30s : %-6d\n", "IV", spec.Iv)
	fmt.Printf("%-30s : %-6d\n", "Key Index", spec.KeyIndex)
	fmt.Printf("%-30s : %-6d\n", "SeqNo", spec.SeqNo)
	//fmt.Printf("%-30s : %-6d\n", "total_rx_pkts", spec.TotalRxPkts)
	//fmt.Printf("%-30s : %-6d\n", "total_rx_drops", spec.TotalRxDrops)
	fmt.Printf("%-30s : %-6d\n", "Total Pkts", spec.TotalPkts)
	fmt.Printf("%-30s : %-6d\n", "Total Bytes", spec.TotalBytes)
	//fmt.Printf("%-30s : %-6d\n", "total_drops", spec.TotalDrops)
}

func showIpsecDecryptSA(resp *halproto.IpsecSADecryptGetResponse) {
	spec := resp.GetSpec()

	switch spec.AuthenticationAlgorithm {
	case 1:
		fmt.Printf("%-30s : %-6s\n", "Authentication Algorithm", "AUTHENTICATION_AES_GCM")
	case 2:
		fmt.Printf("%-30s : %-6s\n", "Authentication Algorithm", "AUTHENTICATION_AES_CCM")
	case 3:
		fmt.Printf("%-30s : %-6s\n", "Authentication Algorithm", "AUTHENTICATION_HMAC")
	case 4:
		fmt.Printf("%-30s : %-6s\n", "Authentication Algorithm", "AUTHENTICATION_AES_CBC_SHA")
	}
	fmt.Printf("%-30s : %-6s\n", "Authentication Key", spec.GetAuthenticationKey().GetKey())
	switch spec.DecryptionAlgorithm {
	case 1:
		fmt.Printf("%-30s : %-6s\n", "DecryptionAlgorithm", "AUTHENTICATION_AES_GCM")
	case 2:
		fmt.Printf("%-30s : %-6s\n", "DecryptionAlgorithm", "AUTHENTICATION_AES_CCM")
	case 3:
		fmt.Printf("%-30s : %-6s\n", "DecryptionAlgorithm", "AUTHENTICATION_HMAC")
	case 4:
		fmt.Printf("%-30s : %-6s\n", "DecryptionAlgorithm", "AUTHENTICATION_AES_CBC_SHA")
	}
	fmt.Printf("%-30s : %-6s\n", "Decryption Key", spec.GetDecryptionKey().GetKey())
	//fmt.Printf("%-30s : %-6d\n", "rekey_dec_algorithm", spec.RekeyDecAlgorithm)
	//fmt.Printf("%-30s : %-6s\n", "Rekey Decryption Key", spec.GetRekeyDecryptionKey().GetKey())
	//fmt.Printf("%-30s : %-6s\n", "Rekey Authentication Key", spec.GetRekeyAuthenticationKey().GetKey())
	fmt.Printf("%-30s : %-6d\n", "SPI", spec.Spi)
	fmt.Printf("%-30s : %-6d\n", "Key Index", spec.KeyIndex)
	if spec.RekeyActive == 1 {
		fmt.Printf("%-30s : %-6s\n", "Rekey Active", "True")
		fmt.Printf("%-30s : %-6d\n", "Rekey SPI", spec.RekeySpi)
		fmt.Printf("%-30s : %-6d\n", "New Key Index", spec.NewKeyIndex)
	} else {
		fmt.Printf("%-30s : %-6s\n", "rekey_active", "False")
	}
	fmt.Printf("%-30s : %-6d\n", "Tunnel Vrf", spec.GetTepVrf().GetVrfId())
	fmt.Printf("%-30s : %-6d\n", "Salt", spec.Salt)
	fmt.Printf("%-30s : %-6d\n", "Expected Seq No", spec.SeqNo)
	//fmt.Printf("%-30s : %-6d\n", "seq_no_bmp", spec.SeqNoBmp)
	//fmt.Printf("%-30s : %-6d\n", "total_rx_pkts", spec.TotalRxPkts)
	//fmt.Printf("%-30s : %-6d\n", "total_rx_drops", spec.TotalRxDrops)
	fmt.Printf("%-30s : %-6d\n", "Total Pkts", spec.TotalPkts)
	fmt.Printf("%-30s : %-6d\n", "Total Bytes", spec.TotalBytes)
	//fmt.Printf("%-30s : %-6d\n", "total_drops", spec.TotalDrops)
}

func ipsecGlobalStatisticsShowCmdHandler(cmd *cobra.Command, args []string) {
	c, err := utils.CreateNewGRPCClient()
	if err != nil {
		fmt.Printf("Could not connect to the HAL. Is HAL Running?\n")
		os.Exit(1)
	}

	client := halproto.NewIpsecClient(c)

	defer c.Close()

	req := &halproto.IpsecGlobalStatisticsGetRequest{
		ClearOnRead: ClearOnRead,
	}
	IpsecGlobalStatisticsGetRequestMsg := &halproto.IpsecGlobalStatisticsGetRequestMsg{
		Request: []*halproto.IpsecGlobalStatisticsGetRequest{req},
	}
	respMsg, err := client.IpsecGlobalStatisticsGet(context.Background(), IpsecGlobalStatisticsGetRequestMsg)
	if err != nil {
		fmt.Printf("IpsecGlobalStatisticsGetRequest failed. %v\n", err)
		return
	}
	for _, resp := range respMsg.Response {
		if resp.ApiStatus != halproto.ApiStatus_API_STATUS_OK {
			fmt.Printf("HAL Returned error status. %v\n", resp.ApiStatus)
			continue
		}

		fmt.Printf("%s\n", strings.Repeat("-", 80))
		fmt.Printf("\nIPSec Global Statistics \n")
		fmt.Printf("%s\n", strings.Repeat("-", 80))
		showIpsecGlobalStatistics(resp)
		fmt.Printf("%s\n\n", strings.Repeat("-", 80))
	}
}

func showIpsecGlobalStatistics(resp *halproto.IpsecGlobalStatisticsGetResponse) {
	spec := resp.GetSpec()

	fmt.Printf("\nEncrypt Global Counters:\n")
	fmt.Printf("-----------------------\n")
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_enter_counters", spec.EncryptRxdmaEnterCounters)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma1_enter_counters", spec.EncryptTxdma1EnterCounters)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma2_enter_counters", spec.EncryptTxdma2EnterCounters)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma2_sem_free_counters", spec.EncryptTxdma2SemFreeErrors)
	fmt.Printf("\nEncrypt Global Errors:\n")
	fmt.Printf("-----------------------\n")
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_input_desc_errors", spec.EncryptInputDescErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_output_desc_errors", spec.EncryptOutputDescErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_cb_ring_base_errors", spec.EncryptCbRingBaseErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_input_page_errors", spec.EncryptInputPageErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_pad_addr_errors", spec.EncryptPadAddrErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_tail_bytes_errors", spec.EncryptTailBytesErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_table3_inpage_errors", spec.EncryptTable3InpageErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_table2_inpage_errors", spec.EncryptTable2InpageErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_table0_inpage_errors", spec.EncryptTable0InpageErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_enqueue_cb_ring_errors", spec.EncryptCbRingDmaErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_desc_exhaust_errors", spec.EncryptDescExhaustErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_dummy_desc_req_errors", spec.EncryptRxdmaDummyDescErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rxdma_cb_ring_full_errors", spec.EncryptRxdmaCbRingFullErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma1_barco_addr_errors", spec.EncryptBadBarcoAddrErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma1_barco_req_addr_errors", spec.EncryptBarcoReqAddrErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma1_barco_cb_base_errors", spec.EncryptBarcoCbBaseErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma1_barco_pindex_full_errors", spec.EncryptBarcoFullErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma1_dummy_errors", spec.EncryptTxdma1DummyErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma1_ipsec_int_indesc_errors", spec.EncryptBarcoBadIndescErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma1_ipsec_int_outdesc_errors", spec.EncryptBarcoBadOutdescErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma1_bad_indesc_free_errors", spec.EncryptTxdma1BadIndescFreeErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma1_bad_outdesc_free_errors", spec.EncryptTxdma1BadOutdescFreeErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma1_sem_free_counters", spec.EncryptTxdma1SemFreeErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma1_barco_ring_full_errors", spec.EncryptTxdma1BarcoRingFullErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma2_output_page_errors", spec.EncryptOutputPageErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma2_stage4_inpage_errors", spec.EncryptStage4InpageErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma2_bad_indesc_free_errors", spec.EncryptTxdma2BadIndescFreeErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma2_bad_outdesc_free_errors", spec.EncryptTxdma2BadOutdescFreeErrors)
	fmt.Printf("%-40s : %-6d\n", "encrypt_txdma2_barco_req_errors", spec.EncryptTxdma2BarcoReqErrors)

	fmt.Printf("\nDecrypt Global Counters:\n")
	fmt.Printf("-----------------------\n")
	fmt.Printf("%-40s : %-6d\n", "decrypt_rxdma_enter_counters", spec.DecryptRxdmaEnterCounters)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma1_enter_counters", spec.DecryptTxdma1EnterCounters)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma2_enter_counters", spec.DecryptTxdma2EnterCounters)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma2_sem_free_counters", spec.DecryptTxdma2SemFreeErrors)
	fmt.Printf("\nDecrypt Global Errors:\n")
	fmt.Printf("-----------------------\n")
	fmt.Printf("%-40s : %-6d\n", "decrypt_rxdma_input_desc_errors", spec.DecryptInputDescErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_rxdma_output_desc_errors", spec.DecryptOutputDescErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_rxdma_cb_ring_base_errors", spec.DecryptCbRingBaseErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_rxdma_input_page_errors", spec.DecryptInputPageErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_rxdma_desc_exhaust_errors", spec.DecryptDescExhaustErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_rxdma_dummy_desc_errors", spec.DecryptRxdmaDummyDescErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_rxdma_cb_ring_full_errors", spec.DecryptRxdmaCbRingFullErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma1_barco_req_addr_errors", spec.DecryptBarcoReqAddrErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma1_barco_cb_addr_errors", spec.DecryptBarcoCbAddrErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma1_drop_counters", spec.DecryptTxdma1DropCounters)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma1_dummy_errors", spec.DecryptTxdma1DummyErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma1_sem_free_counters", spec.DecryptTxdma1SemFreeErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma1_bad_indesc_free_errors", spec.DecryptTxdma1BadIndescFreeErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma1_bad_outdesc_free_errors", spec.DecryptTxdma1BadOutdescFreeErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma1_barco_ring_full_errors", spec.DecryptTxdma1BarcoRingFullErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma1_barco_full_errors", spec.DecryptTxdma1BarcoFullErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma2_stage4_inpage_errors", spec.DecryptStage4InpageErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma2_output_page_errors", spec.DecryptOutputPageErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma2_load_ipsec_int_errors", spec.DecryptLoadIpsecIntErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma2_dummy_free", spec.DecryptTxdma2DummyFree)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma2_barco_bad_indesc_errors", spec.DecryptTxdma2BarcoBadIndescErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma2_barco_bad_outdesc_errors", spec.DecryptTxdma2BarcoBadOutdescErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma2_bad_indesc_free_errors", spec.DecryptTxdma2BadIndescFreeErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma2_bad_outdesc_free_errors", spec.DecryptTxdma2BadOutdescFreeErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_txdma2_invalid_barco_req_errors", spec.DecryptTxdma2InvalidBarcoReqErrors)

	fmt.Printf("\n%-40s : %-6d\n", "encrypt_rnmdpr_sem_pindex", spec.EncRnmdprSemPindex)
	fmt.Printf("%-40s : %-6d\n", "encrypt_rnmdpr_sem_cindex", spec.EncRnmdprSemCindex)
	fmt.Printf("%-40s : %-6d\n", "decrypt_rnmdpr_sem_pindex", spec.DecRnmdprSemPindex)
	fmt.Printf("%-40s : %-6d\n", "decrypt_rnmdpr_sem_cindex", spec.DecRnmdprSemCindex)
	fmt.Printf("%-40s : %-6d\n", "encrypt_barco_full_errors", spec.Gcm0BarcoFullErrors)
	fmt.Printf("%-40s : %-6d\n", "decrypt_barco_full_errors", spec.Gcm1BarcoFullErrors)
	fmt.Printf("%-40s : %-6d\n", "enc_global_barco_pi", spec.EncGlobalBarcoPi)
	fmt.Printf("%-40s : %-6d\n", "enc_global_barco_ci", spec.EncGlobalBarcoCi)
	fmt.Printf("%-40s : %-6d\n", "dec_global_barco_pi", spec.DecGlobalBarcoPi)
	fmt.Printf("%-40s : %-6d\n", "dec_global_barco_ci", spec.DecGlobalBarcoCi)
}
