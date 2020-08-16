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
#include <iostream>
#include <string>
#include "nic/infra/operd/tests/flowlog_generator/flowlog_gen.hpp"

using namespace std;

// max flow logs per second supported
static constexpr uint32_t g_max_rate = 10000;

static inline
void print_usage (char* name)
{
    cerr << "Usage: " << name << endl
         << "\t -r rate per second [default: 100]\n"
         << "\t -n number of flow_logs [default: 500]\n"
         << "\t -t type of flow {l2|ipv4}\n"
         << "\t -l lookup id (vpc/bd hw_id) to be used for flows\n"
         << "\t -h help \n"
         << endl;
}

int main (int argc, char** argv)
{
    int opt;
    uint32_t rate = 100;
    uint32_t total = 1000;
    uint32_t lookup_id = 0;
    string type;
    test::flow_logs_generator *generator;

    while ((opt = getopt(argc, argv, "r:n:t:l:h")) != -1) {
        switch (opt) {
        case 'r':
            rate = atoi(optarg);
            if (rate > g_max_rate) {
                cerr << "ERR: rate option -r "
                     << " requires value in between 1 and "
                     << g_max_rate
                     << endl;
                exit(EXIT_FAILURE);
            }
            break;
        case 'n':
            total = atoi(optarg);
            break;
        case 't':
            type = string(optarg);
            if (type != "ipv4" && type != "l2") {
                cerr << "ERR: flow type option -t "
                     << " can take only ipv4 or l2" << endl;
                exit(EXIT_FAILURE);
            }
            break;
        case 'l':
            // TODO: change to UUID once support comes in
            lookup_id = atoi(optarg);
            break;
        case 'h':
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        case ':':
            cerr << "option -" << optopt << " requires an argument" << endl;
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

    generator = test::flow_logs_generator::factory(rate, total,
                                                   lookup_id, type);
    if (generator == NULL) {
        cerr << "Failed to create flow_logs_generator object" << endl;
        exit(EXIT_FAILURE);
    }
    generator->show();
    generator->generate();
    test::flow_logs_generator::destroy(generator);

    exit(EXIT_SUCCESS);
}
