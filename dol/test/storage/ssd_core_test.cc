#include "dol/test/storage/ssd_core.hpp"
#include "gtest/gtest.h"

class ssdtest : public storage_test::NvmeSsdCore {
 public:
  ssdtest() { Ctor(); }
  ~ssdtest() { Dtor(); }
  virtual void *DMAMemAlloc(uint32_t size) { return malloc(size); }
  virtual void DMAMemFree(void *ptr) { free(ptr); }
  virtual uint64_t DMAMemV2P(void *ptr) { return (uint64_t)ptr; }
  virtual void *DMAMemP2V(uint64_t addr) { return (void *)addr; }
  virtual void RaiseInterrupt() { intr_counter_++; }

  unsigned intr_counter_ = 0;
};

class SsdCoreTest : public testing::Test {
 public:
  virtual void SetUp() {
    ssd_.reset(new ssdtest());
    buf_.reset(new uint8_t[4096]);
    ssd_->GetWorkingParams(&params_);
    expected_phase_ = 1;
    compq_ci_ = subq_pi_ = 0;
  }

  void PushCmd(const NvmeCmd &cmd) {
    bcopy(&cmd, &params_.subq_va[subq_pi_], sizeof(cmd));
    subq_pi_++;
    if (subq_pi_ == params_.subq_nentries)
      subq_pi_ = 0;
    *params_.subq_pi_va = subq_pi_;
  }

  bool PopCompletion(NvmeStatus *comp) {
    if (params_.compq_va[compq_ci_].dw3.phase != expected_phase_)
      return false;
    bcopy(&params_.compq_va[compq_ci_], comp, sizeof (*comp));
    compq_ci_++;
    if (compq_ci_ == params_.compq_nentries) {
      compq_ci_ = 0;
      expected_phase_ ^= 1;
    }
    *params_.compq_ci_va = compq_ci_;
    return true;
  }

  uint16_t subq_pi_, compq_ci_;
  uint8_t expected_phase_;
  std::unique_ptr<ssdtest> ssd_;
  std::unique_ptr<uint8_t[]> buf_;
  storage_test::SsdWorkingParams params_;
};

TEST_F(SsdCoreTest, BasicTest) {
  NvmeCmd cmd;
  NvmeStatus comp;

  // Prepare Command
  bzero(&cmd, sizeof(cmd));
  bzero(&comp, sizeof(comp));
  cmd.dw0.opc = NVME_READ_CMD_OPCODE;
  cmd.dw0.cid = 0x1234;
  cmd.nsid = params_.namespace_id;
  cmd.prp.prp1 = ssd_->DMAMemV2P((void *)buf_.get());
  cmd.prp.prp2 = (cmd.prp.prp1 + 4096ul) & ~0xFFFul;
  cmd.slba = 1;

  uint32_t old_intr_counter = ssd_->intr_counter_;
  // Push the command and ring doorbell.
  PushCmd(cmd);
  usleep(10000);

  // Wait for completion.
  ASSERT_TRUE(ssd_->intr_counter_ != old_intr_counter);
  ASSERT_TRUE(PopCompletion(&comp));
  ASSERT_EQ(0x1234, comp.dw3.cid);
  ASSERT_EQ(1, comp.dw3.phase);
  ASSERT_EQ(0, comp.dw3.status);
  ASSERT_EQ(1, comp.dw2.sq_head);
}
