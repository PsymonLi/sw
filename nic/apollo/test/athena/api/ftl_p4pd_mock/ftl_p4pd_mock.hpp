//------------------------------------------------------------------------------
// {C} Copyright 2019 Pensando Systems Inc. All rights reserved
// This file contains Key and Data structures used to test the mem hash library.
// The variations mostly are because of the difference in key and data sizes which
// changes the number of hints.
//------------------------------------------------------------------------------
#ifndef __FTL_P4PD_MOCK_HPP__
#define __FTL_P4PD_MOCK_HPP__

void dnat_mock_init(void);
void dnat_mock_cleanup(void);
void flow_mock_init(void);
void flow_mock_cleanup(void);
void l2_flow_mock_init(void);
void l2_flow_mock_cleanup(void);
uint32_t ftl_mock_get_valid_count(uint32_t table_id);

#endif
