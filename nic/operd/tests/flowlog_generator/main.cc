//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// test app to generate flow logs
///
//----------------------------------------------------------------------------

#include <getopt.h>
#include <string>
#include <iostream>

#include "flowlog_gen.hpp"

using namespace std;

// max flow logs per second supported
static constexpr uint32_t g_max_rate = 10000;

static inline
void print_usage(char* name)
{
    cerr << "Usage: " << name << endl
         << "\t -r rate per second [default: 100]\n"
         << "\t -n number of flow_logs [default: 500]\n"
         //  << "\t -t type of flow [default: IPV4 (random type)]\n"
         << "\t -h help \n"
         << endl;
}

int main(int argc, char** argv)
{
    int opt;
    uint32_t rate = 100;
    uint32_t total = 1000;
    test::flow_logs::generator *flow_log_generator;

    while ((opt = getopt(argc, argv, "r:n:h")) != -1) {
        switch (opt) {
        case 'r':
            rate = atoi(optarg);
            if (rate > g_max_rate) {
                cerr << "option -"
                     << optopt
                     << " requires value in between 1 and "
                     << g_max_rate
                     << endl;
                exit(EXIT_FAILURE);
            }
            break;
        case 'n':
            total = atoi(optarg);
            break;
        case 'h':
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        case ':':
            cerr << "option -" << optopt << " requires an argument" << endl;
            print_usage(argv[0]);
            // fall through
        default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (total < rate) {
        cerr << "[-n total flow_logs] should be >= rate per second" << endl;
        exit(EXIT_FAILURE);
    }

    flow_log_generator = test::flow_logs::generator::factory(rate, total);
    if (flow_log_generator == NULL) {
        cerr << "Failed to create flow_logs generator object" << endl;
        exit(EXIT_FAILURE);
    }
    flow_log_generator->show();
    flow_log_generator->generate();
    test::flow_logs::generator::destroy(flow_log_generator);

    exit(EXIT_SUCCESS);
}
