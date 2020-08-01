//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// implementation of flow logs generator
///
//----------------------------------------------------------------------------

#ifndef __FLOWLOG_GEN_HPP__
#define __FLOWLOG_GEN_HPP__

#include "nic/sdk/lib/operd/region.hpp"

using namespace std;

#define FLOW_LOG_PRODUCER_NAME  "vpp"

namespace test {
namespace flow_logs {

class generator {
public:

    /// \brief factory method to allocate & initialize generator instance
    /// \return new instance of generator or NULL, in case of error
    static generator *factory(uint32_t rate, uint32_t total);

    /// \brief free memory allocated to generator instance
    static void destroy(generator *inst);

    /// \brief displays the configuration
    void show(void);

    /// \brief generates flow_logs
    void generate(void);

private:
    /// \brief parameterized constructor
    generator(uint32_t rate, uint32_t total) {
        rate_ = rate;
        total_ = total;
        recorder_ = sdk::operd::region::create(FLOW_LOG_PRODUCER_NAME);
    }

    /// \brief destructor
    ~generator() {}

    /// \brief records single flow log based on config
    void record_flow_log_(void);

private:
    uint32_t rate_;                    ///< generation rate per second
    uint32_t total_;                   ///< number of flow logs to be generated
    sdk::operd::region_ptr recorder_;  ///< flow log recorder
};

}    // namespace flow_logs
}    // namespace test
#endif    // __FLOWLOG_GEN_HPP__