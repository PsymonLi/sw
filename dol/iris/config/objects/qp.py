#! /usr/bin/python3
import pdb

import scapy.all                as scapy
import model_sim.src.model_wrap as model_wrap

import infra.common.defs        as defs
import infra.common.objects     as objects
import infra.config.base        as base

import iris.config.resmgr            as resmgr
from iris.config.store               import Store
from infra.common.logging       import logger

import iris.config.hal.api           as halapi
import iris.config.hal.defs          as haldefs
import rdma_pb2                 as rdma_pb2

import iris.config.objects.slab      as slab
import iris.config.objects.mr        as mr

from factory.objects.rdma.descriptor import RdmaSqDescriptorBase
from factory.objects.rdma.descriptor import RdmaSqDescriptorSend
from factory.objects.rdma.descriptor import RdmaSqDescriptorUDSend
from factory.objects.rdma.descriptor import RdmaRqDescriptorBase
from factory.objects.rdma.descriptor import RdmaRrqDescriptorBase
from factory.objects.rdma.descriptor import RdmaRrqDescriptorRead
from factory.objects.rdma.descriptor import RdmaRrqDescriptorAtomic
from factory.objects.rdma.descriptor import RdmaRsqDescriptorBase
from factory.objects.rdma.descriptor import RdmaRsqDescriptorRead
from factory.objects.rdma.descriptor import RdmaRsqDescriptorAtomic
from factory.objects.rdma.descriptor import RdmaSge

from infra.common.glopts import GlobalOptions

class QpObject(base.ConfigObjectBase):
    def __init__(self, pd, qp_id, spec, sges):
        super().__init__()
        self.Clone(Store.templates.Get('QPAIR_RDMA'))
        self.pd = pd
        self.remote = pd.remote
        self.id = qp_id
        self.GID("QP%04d" % self.id)
        self.spec = spec

        self.pd = pd
        self.svc = spec.service

        self.pmtu = spec.pmtu
        self.hostmem_pg_size = spec.hostmem_pg_size
        self.atomic_enabled = spec.atomic_enabled
        self.sq_in_nic = spec.sq_in_nic
        self.rq_in_nic = spec.rq_in_nic

        # we can use this tiny attribute to filter testcases
        self.tiny = False
        if sges <= 2:
            self.tiny = True

        self.num_sq_sges = sges
        self.num_sq_wqes = self.__roundup_to_pow_2(spec.num_sq_wqes)
        self.num_rrq_wqes = self.__roundup_to_pow_2(spec.num_rrq_wqes)

        self.num_rq_sges = sges
        self.num_rq_wqes = self.__roundup_to_pow_2(spec.num_rq_wqes)
        self.num_rsq_wqes = self.__roundup_to_pow_2(spec.num_rsq_wqes)

        self.sqwqe_size = self.__get_sqwqe_size()
        self.rqwqe_size = self.__get_rqwqe_size()
        self.rrqwqe_size = self.__get_rrqwqe_size()
        self.rsqwqe_size = self.__get_rsqwqe_size()

        self.sq_size = self.num_sq_wqes * self.sqwqe_size
        self.rq_size = self.num_rq_wqes * self.rqwqe_size
        self.rrq_size = self.num_rrq_wqes * self.rrqwqe_size
        self.rsq_size = self.num_rsq_wqes * self.rsqwqe_size

        if not self.remote:
            self.sq = pd.ep.intf.lif.GetQ('RDMA_SQ', self.id)
            self.rq = pd.ep.intf.lif.GetQ('RDMA_RQ', self.id)
            
            if (self.sq is None or self.rq is None):
                assert(0)
        
            #logger.info('QP: %s PD: %s Remote: %s intf: %s lif: %s' %(self.GID(), self.pd.GID(), self.remote, pd.ep.intf.GID(), pd.ep.intf.lif.GID()))

            self.tx = pd.ep.intf.lif.GetQt('TX')
            self.rx = pd.ep.intf.lif.GetQt('RX')
            
            if (self.tx is None or self.rx is None):
                assert(0)

            # for now map both sq/rq cq to be same
            #TODO: Until Yogesh's fix comes in for Unaligned write back, just allocate CQIDs as even number
            #revert '* 2' 
            self.sq_cq = pd.ep.intf.lif.GetQ('RDMA_CQ', self.id * 2)
            self.rq_cq = pd.ep.intf.lif.GetQ('RDMA_CQ', self.id * 2)
            if (self.sq_cq is None or self.rq_cq is None):
                assert(0)
    
            # allocating one EQ for one PD
            self.eq = pd.ep.intf.lif.GetQ('RDMA_EQ', pd.id)  # PD id is the EQ number
            if (self.eq is None):
                assert(0)
    
            # create sq/rq slabs
            self.sq_slab = slab.SlabObject(self.pd.ep, self.sq_size)
            self.rq_slab = slab.SlabObject(self.pd.ep, self.rq_size)
            self.pd.ep.AddSlab(self.sq_slab)
            self.pd.ep.AddSlab(self.rq_slab)

            # create sq/rq mrs
            #self.sq_mr = mr.MrObject(self.pd, self.sq_slab)
            #self.rq_mr = mr.MrObject(self.pd, self.rq_slab)
            #self.pd.AddMr(self.sq_mr)
            #self.pd.AddMr(self.rq_mr)
    
        self.Show()

        return

    def __roundup_to_pow_2(self, x):
        power = 1
        while power < x : 
            power *= 2
        return power
 
    def __get_sqwqe_size(self):
        return  self.__roundup_to_pow_2(
                len(RdmaSqDescriptorBase()) + 
                len(RdmaSqDescriptorSend()) +
                (self.num_sq_sges * len(RdmaSge()))) 

    def __get_rqwqe_size(self):
        return self.__roundup_to_pow_2(
                len(RdmaRqDescriptorBase()) + 
                (self.num_rq_sges * len(RdmaSge()))) 

    def __get_rrqwqe_size(self):
        return self.__roundup_to_pow_2(
                len(RdmaRrqDescriptorBase()) + 
                len(RdmaRrqDescriptorRead()))

    def __get_rsqwqe_size(self):
        return self.__roundup_to_pow_2(
                len(RdmaRsqDescriptorBase()) + 
                len(RdmaRsqDescriptorRead()))

    def Show(self):
        logger.info('QP: %s PD: %s Remote: %s' %(self.GID(), self.pd.GID(), self.remote))
        logger.info('SQ num_sges: %d num_wqes: %d wqe_size: %d' %(self.num_sq_sges, self.num_sq_wqes, self.sqwqe_size)) 
        logger.info('RQ num_sges: %d num_wqes: %d wqe_size: %d' %(self.num_rq_sges, self.num_rq_wqes, self.rqwqe_size)) 
        logger.info('RRQ num_wqes: %d wqe_size: %d' %(self.num_rrq_wqes, self.rrqwqe_size)) 
        logger.info('RSQ num_wqes: %d wqe_size: %d' %(self.num_rsq_wqes, self.rsqwqe_size)) 
        if not self.remote:
            logger.info('SQ_CQ: %s RQ_CQ: %s' %(self.sq_cq.GID(), self.rq_cq.GID()))

    def set_dst_qp(self, value):
        self.modify_qp_oper = rdma_pb2.RDMA_UPDATE_QP_OPER_SET_DEST_QPN
        self.dst_qp = value
        halapi.ModifyQps([self])

    def set_q_key(self, value):
        self.modify_qp_oper = rdma_pb2.RDMA_UPDATE_QP_OPER_SET_QKEY
        self.q_key = value
        halapi.ModifyQps([self])

    def set_q_state(self, value):
        self.modify_qp_oper = rdma_pb2.RDMA_UPDATE_QP_OPER_SET_STATE
        self.qstate = value
        halapi.ModifyQps([self])

    def PrepareHALRequestSpec(self, req_spec):
        logger.info("QP: %s PD: %s Remote: %s HW_LIF: %d EP->Intf: %s" %\
                        (self.GID(), self.pd.GID(), self.remote, self.pd.ep.intf.lif.hw_lif_id,
                         self.pd.ep.intf.GID()))

        if (GlobalOptions.dryrun): return

        if req_spec.__class__.__name__ == "RdmaQpSpec":  
    
            req_spec.qp_num = self.id
            req_spec.hw_lif_id = self.pd.ep.intf.lif.hw_lif_id
            req_spec.if_handle = self.pd.ep.intf.hal_handle
            req_spec.sq_wqe_size = self.sqwqe_size
            req_spec.rq_wqe_size = self.rqwqe_size
            req_spec.num_sq_wqes = self.num_sq_wqes
            req_spec.num_rq_wqes = self.num_rq_wqes
            req_spec.num_rsq_wqes = self.num_rsq_wqes
            req_spec.num_rrq_wqes = self.num_rrq_wqes
            req_spec.pd = self.pd.id
            req_spec.pmtu = self.pmtu
            req_spec.hostmem_pg_size = self.hostmem_pg_size
            req_spec.svc = self.svc
            req_spec.atomic_enabled = self.atomic_enabled

            #req_spec.sq_lkey = self.sq_mr.lkey
            #req_spec.rq_lkey = self.rq_mr.lkey
            
            req_spec.va_pages_phy_addr[:] = self.sq_slab.phy_address + self.rq_slab.phy_address
            req_spec.num_sq_pages = len(self.sq_slab.phy_address)

            req_spec.sq_cq_num = self.sq_cq.id
            req_spec.rq_cq_num = self.rq_cq.id
            req_spec.sq_in_nic_memory = self.sq_in_nic
            req_spec.rq_in_nic_memory = self.rq_in_nic
    
        elif req_spec.__class__.__name__ == "RdmaQpUpdateSpec":

            req_spec.qp_num = self.id
            req_spec.hw_lif_id = self.pd.ep.intf.lif.hw_lif_id
            req_spec.oper = self.modify_qp_oper
            if req_spec.oper == rdma_pb2.RDMA_UPDATE_QP_OPER_SET_DEST_QPN:
               req_spec.dst_qp_num = self.dst_qp
            elif req_spec.oper == rdma_pb2.RDMA_UPDATE_QP_OPER_SET_QKEY:
               req_spec.q_key = self.q_key
            elif req_spec.oper == rdma_pb2.RDMA_UPDATE_QP_OPER_SET_AV:
               req_spec.header_template = bytes(self.HdrTemplate)
               req_spec.ahid = self.ah_handle
            elif req_spec.oper == rdma_pb2.RDMA_UPDATE_QP_OPER_SET_STATE:
               req_spec.qstate = self.qstate

    def ProcessHALResponse(self, req_spec, resp_spec):

        if req_spec.__class__.__name__ == "RdmaQpSpec":  
    
            self.rsq_base_addr = resp_spec.rsq_base_addr
            self.rrq_base_addr = resp_spec.rrq_base_addr
            self.nic_sq_base_addr = resp_spec.nic_sq_base_addr
            self.nic_rq_base_addr = resp_spec.nic_rq_base_addr
            self.rdma_atomic_res_addr = resp_spec.rdma_atomic_res_addr
            if self.sq_in_nic:
                self.sq.SetRingParams('SQ', 0, True, True,
                                  None,
                                  self.nic_sq_base_addr,
                                  self.num_sq_wqes, 
                                  self.sqwqe_size)
            else:
                self.sq.SetRingParams('SQ', 0, True, False,
                                  self.sq_slab.mem_handle,
                                  self.sq_slab.address, 
                                  self.num_sq_wqes, 
                                  self.sqwqe_size)
            if self.rq_in_nic:
                self.rq.SetRingParams('RQ', 0, True, True,
                                  None,
                                  self.nic_rq_base_addr,
                                  self.num_rq_wqes, 
                                  self.rqwqe_size)
            else:
                self.rq.SetRingParams('RQ', 0, True, False,
                                  self.rq_slab.mem_handle,
                                  self.rq_slab.address,
                                  self.num_rq_wqes, 
                                  self.rqwqe_size)
            logger.info("QP: %s PD: %s Remote: %s"
                           "sq_base_addr: 0x%x rq_base_addr: 0x%x "
                           "rsq_base_addr: 0x%x rrq_base_addr: 0x%x " %\
                            (self.GID(), self.pd.GID(), self.remote,
                             self.sq_slab.address, self.rq_slab.address,
                             self.rsq_base_addr, self.rrq_base_addr))
    
        elif req_spec.__class__.__name__ == "RdmaQpUpdateSpec":

            logger.info("Resp for RdmaQpUpdateSpec, "
                           "QP: %s PD: %s oper: %d" %\
                            (self.GID(), self.pd.GID(), req_spec.oper))


        #logger.ShowScapyObject(self.sq_cq.qstate.data)

    def IsFilterMatch(self, spec):
        logger.debug("Matching QID %d svc %d" %(self.id, self.svc))
        match = super().IsFilterMatch(spec.filters)

        return match

    def ShowTestcaseConfig(self, obj):
        logger.info("Config Objects for %s" % (self.GID()))
        return

    def ConfigureHeaderTemplate(self, rdma_session, initiator, responder, flow, isipv6, forward, ah_handle):
        logger.info("rdma_session: %s" % rdma_session.GID())
        logger.info("session: %s" % rdma_session.session.GID())
        logger.info("flow_ep1: %s ep1: %s" \
            %(initiator.GID(), initiator.ep.GID()))
        logger.info("flow_ep2: %s ep2: %s" \
             %(responder.GID(), responder.ep.GID()))
        logger.info("src_ip: %s" % initiator.addr.get())
        logger.info("dst_ip: %s" % responder.addr.get())
        logger.info("src_mac: %s" % initiator.ep.macaddr.get())
        logger.info("dst_mac: %s" % responder.ep.macaddr.get())
        logger.info("proto: %s" % flow.proto)
        logger.info("sport: %s" % flow.sport)
        logger.info("dport: %s" % flow.dport)
        logger.info("ah_handle: %d" % ah_handle)
        if forward:
            logger.info("src_qp: %d pd: %s" % (rdma_session.lqp.id, rdma_session.lqp.pd.GID()))
            logger.info("dst_qp: %d pd: %s" % (rdma_session.rqp.id, rdma_session.rqp.pd.GID()))
        else:
            logger.info("src_qp: %d pd: %s" % (rdma_session.rqp.id, rdma_session.rqp.pd.GID()))
            logger.info("dst_qp: %d pd: %s" % (rdma_session.lqp.id, rdma_session.lqp.pd.GID()))
        logger.info("isipv6: %d" % (isipv6))
        logger.info("isVXLAN: %d" % (rdma_session.IsVXLAN))

        EthHdr = scapy.Ether(src=initiator.ep.macaddr.get(),
                             dst=responder.ep.macaddr.get())
        Dot1qHdr = scapy.Dot1Q(vlan=initiator.ep.intf.encap_vlan_id,
                               prio=flow.txqos.cos)
        if isipv6:
            IpHdr = scapy.IPv6(src=initiator.addr.get(),
                             dst=responder.addr.get(),
                             tc=flow.txqos.dscp,
                             plen = 0)
        else:
            IpHdr = scapy.IP(src=initiator.addr.get(),
                             dst=responder.addr.get(),
                             tos=flow.txqos.dscp,
                             len = 0, chksum = 0)
        if forward:
            UdpHdr = scapy.UDP(sport=flow.sport,
                               dport=flow.dport,
                               len = 0, chksum = 0)
        else:
            UdpHdr = scapy.UDP(sport=flow.dport,
                               dport=flow.sport,
                               len = 0, chksum = 0)
        self.HdrTemplate = EthHdr/Dot1qHdr/IpHdr/UdpHdr
        self.ah_handle = ah_handle

        # header_template size is 66. The address is maintained only to calculate
        # dcqcn_cb address.
        # 136 = [ 66 (header_template_t) + 1 (ah_size) + 64 (dcqcncb_t) ] 8 byte aligned
        self.header_temp_addr = self.pd.ep.intf.lif.rdma_at_base_addr + (ah_handle * 136)

        self.modify_qp_oper = rdma_pb2.RDMA_UPDATE_QP_OPER_SET_AV

        if (GlobalOptions.dryrun): return

        halapi.ModifyQps([self])

        return

    # Routines to read and write to dcqcn_cb    
    def WriteDcqcnCb(self):
        if (GlobalOptions.dryrun): return
        # dcqcn_cb is located after header_template. header_template is 66 bytes len and 1 byte ah_size.
        logger.info("Writing DCQCN Qstate @0x%x  size: %d" % (self.header_temp_addr + 67, 64))
        model_wrap.write_mem_pcie(self.header_temp_addr + 67, bytes(self.dcqcn_data), 64)
        self.ReadDcqcnCb()
        return

    def ReadDcqcnCb(self):
        if (GlobalOptions.dryrun):
            dcqcn_data = bytes(64)
            self.dcqcn_data = RdmaDCQCNstate(dcqcn_data)
            return
        self.dcqcn_data = RdmaDCQCNstate(model_wrap.read_mem(self.header_temp_addr + 67, 64))
        logger.ShowScapyObject(self.dcqcn_data)
        logger.info("Read DCQCN Qstate @0x%x size: %d" % (self.header_temp_addr + 67, 64))
        return

    # Routines to read and write to atomic_res_addr
    def WriteAtomicResData(self):
        if (GlobalOptions.dryrun): return
        logger.info("Writing Atomic resource address @0x%x  size: %d" % (self.rdma_atomic_res_addr, 64))
        model_wrap.write_mem_pcie(self.rdma_atomic_res_addr, bytes(self.atomic_res_data), 64)
        self.ReadAtomicResData()
        return

    def ReadAtomicResData(self):
        if (GlobalOptions.dryrun):
            atomic_res_data = bytes(64)
            self.atomic_res_data = RdmaAtomicResState(atomic_res_data)
            return
        self.atomic_res_data = RdmaAtomicResState(model_wrap.read_mem(self.rdma_atomic_res_addr, 64))
        logger.ShowScapyObject(self.atomic_res_data)
        return

class RdmaDCQCNstate(scapy.Packet):
    name = "RdmaDCQCNstate"
    fields_desc = [
        scapy.BitField("last_cnp_timestamp", 0, 48),

        scapy.IntField("byte_counter_thr",0),
        scapy.BitField("timer_exp_thr", 0, 16),
        scapy.BitField("g_val", 0, 16),

        scapy.IntField("rate_enforced", 0),
        scapy.IntField("target_rate",0),
        scapy.BitField("alpha_value", 0, 16),

        scapy.IntField("cur_byte_counter",0),
        scapy.BitField("byte_counter_exp_cnt",0, 16),
        scapy.BitField("timer_exp_cnt", 0, 16),
        scapy.BitField("num_alpha_exp_cnt", 0, 16), 
        scapy.ByteField("num_cnp_rcvd", 0),
        scapy.ByteField("num_cnp_processed", 0),
        scapy.BitField("max_rate_reached", 0, 1),
        scapy.BitField("log_sq_size", 0, 5),
        scapy.BitField("rsvd0", 0, 2),
        

        scapy.BitField("last_sched_timestamp", 0, 48),
        scapy.BitField("delta_tokens_last_sched", 0, 16),
        scapy.BitField("cur_avail_tokens", 0, 48),
        scapy.BitField("token_bucket_size", 0, 48),
        scapy.BitField("sq_cindex", 0, 16),

        scapy.ByteField("num_sched_drop", 0),
        scapy.BitField("cur_timestamp", 0, 32),
    ]

class RdmaAtomicResState(scapy.Packet):
    name = "RdmaAtomicResState"
    fields_desc = [
        scapy.LongField("data0", 0),
        scapy.LongField("data1", 0),
        scapy.LongField("data2", 0),
        scapy.LongField("data3", 0),

        scapy.LongField("pad0", 0),
        scapy.LongField("pad1", 0),
        scapy.LongField("pad2", 0),
        scapy.LongField("pad3", 0),
    ]

class QpObjectHelper:
    def __init__(self):
        self.qps = []
        self.perf_qps = []
        self.udqps = []

    def Generate(self, pd, spec):
        self.pd = pd
        j = 0

        #RC QPs
        rc_spec = spec.rc
        count = rc_spec.count
        logger.info("Creating %d %s Qps. for PD:%s" %\
                       (count, rc_spec.svc_name, pd.GID()))
        for i in range(count):
            qp_id = j if pd.remote else pd.ep.intf.lif.GetQpid()
            sges = 2 << i
            logger.info("RC: qp_id: %d sges: %d" %(qp_id, sges))
            qp = QpObject(pd, qp_id, rc_spec, sges)
            self.qps.append(qp)
            j += 1

        #PERF RC QPs
        rc_spec = spec.perf_rc
        count = rc_spec.count
        logger.info("Creating %d %s Perf Qps. for PD:%s" %\
                       (count, rc_spec.svc_name, pd.GID()))
        for i in range(count):
            qp_id = j if pd.remote else pd.ep.intf.lif.GetQpid()
            sges = 2 << i
            logger.info("PerfRC: qp_id: %d sges: %d" %(qp_id, sges))
            qp = QpObject(pd, qp_id, rc_spec, sges)
            self.perf_qps.append(qp)
            j += 1


        #UD QPs
        ud_spec = spec.ud
        count = ud_spec.count
        logger.info("Creating %d %s Qps. for PD:%s" %\
                       (count, ud_spec.svc_name, pd.GID()))
        for i in range(count):
            qp_id = j if pd.remote else pd.ep.intf.lif.GetQpid()
            sges = 2 << i
            logger.info("UD: qp_id: %d sges: %d" %(qp_id, sges))
            qp = QpObject(pd, qp_id, ud_spec, sges)
            self.qps.append(qp)
            self.udqps.append(qp)
            j += 1


    def Configure(self):
        if self.pd.remote:
            logger.info("skipping QP configuration for remote PD: %s" %(self.pd.GID()))
            return
        logger.info("Configuring %d QPs %d Perf QPs." % (len(self.qps), len(self.perf_qps)))
        halapi.ConfigureQps(self.qps)
        halapi.ConfigureQps(self.perf_qps)
