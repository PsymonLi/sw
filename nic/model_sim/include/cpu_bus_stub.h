#ifndef CPU_BUS_STUB_H
#define CPU_BUS_STUB_H

#include "cpu_bus_base.h"

using namespace std;

class cpu_bus_stub_if : public cpu_bus_base {

public:

  cpu_bus_stub_if(string cpu_if_name, string cpu_if_hier_path) : cpu_bus_base(cpu_if_name, cpu_if_hier_path) {
  }

  virtual ~cpu_bus_stub_if() {
  }

  virtual uint32_t read(uint32_t chip, uint64_t addr, cpu_access_type_e do_backdoor=front_door_e, uint32_t flags=secure_acc_e) {return 0;} ; 
  virtual void write(uint32_t chip, uint64_t addr, uint32_t data, cpu_access_type_e do_backdoor=front_door_e, uint32_t flags=secure_acc_e) {};
  void block_write(uint32_t chip, uint64_t addr, int size, vector<uint32_t> data, cpu_access_type_e do_backdoor, uint32_t flags=secure_acc_e) {}
  vector<uint32_t> block_read(uint32_t chip, uint64_t addr, int size, cpu_access_type_e do_backdoor, uint32_t flags=secure_acc_e) {
      vector<uint32_t> ret_val;
      for(int ii = 0; ii < size; ii++) { ret_val.push_back(0); }
      return ret_val;
  }

};

#endif // CPU_BUS_STUB_H
