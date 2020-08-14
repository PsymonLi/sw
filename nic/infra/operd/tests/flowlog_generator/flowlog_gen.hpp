//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// implementation of flow_logs_generator
///
//----------------------------------------------------------------------------

#ifndef __FLOWLOG_GEN_HPP__
#define __FLOWLOG_GEN_HPP__

#include "nic/sdk/lib/operd/region.hpp"

using namespace std;

#define FLOW_LOG_PRODUCER_NAME  "vpp"

namespace test {

class flow_logs_generator {
public:

    /// \brief    factory method to allocate and initialize
    ///           flow_logs_generator instance
    /// \return   new instance of flow_logs_generator or NULL, in case of error
    static flow_logs_generator *factory(uint32_t rate, uint32_t total,
                                        uint32_t lookup_id, string type);

    /// \brief    free memory allocated to flow_logs_generator instance
    static void destroy(flow_logs_generator *inst);

    /// \brief    displays the configuration
    void show(void);

    /// \brief    generates flow_logs
    void generate(void);

private:
    /// \brief    parameterized constructor
    flow_logs_generator(uint32_t rate, uint32_t total, uint32_t lookup_id,
                        string type);

    /// \brief    destructor
    ~flow_logs_generator() {}

    /// \brief    records single flow log based on config
    void record_flow_log_(void);

private:
    uint32_t rate_;                    ///< generation rate per second
    uint32_t total_;                   ///< number of flow logs to be generated
    uint32_t lookup_id_;               ///< lookup_id to be used in flow logs
    uint32_t type_;                    ///< type of flow logs to be generated
    sdk::operd::region_ptr recorder_;  ///< flow log recorder
};

}    // namespace test
#endif    // __FLOWLOG_GEN_HPP__