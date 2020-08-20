//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//----------------------------------------------------------------------------
///
/// \file
/// test app to generate events
///
//----------------------------------------------------------------------------

#include <getopt.h>
#include <string>
#include <iostream>
#include "nic/infra/operd/tests/event_generator/event_gen.hpp"

using namespace std;
using namespace test;

// max event per second supported
static constexpr int g_max_rps = 10000;

static void inline
print_usage(char* name)
{
    cerr << "Usage: " << name << endl         
         << "\t -r rate per second [default: 1]\n"
         << "\t -n number of event [default: 1]\n"
         << "\t -t type of event [default: -1 (random type)] \n"
         << "\t -p prefix for event messages [default: 'TEST_EVENT_GEN']\n"
         << "\t -h help \n"
         << endl;
}

int
main (int argc, char** argv)
{
    int opt;
    int rate_per_second = 1;
    int total_event = 1;
    int event_type = -1;
    string event_msg_pfx = "TEST_EVENT_GEN";
    event_generator *generator;

    while ((opt = getopt(argc, argv, "r:n:t:p:h")) != -1) {
        switch (opt) {
        case 'r':
            rate_per_second = atoi(optarg);
            if (rate_per_second > g_max_rps) {
                cerr << "option -"
                     << optopt
                     << " requires value in between 1 and "
                     << g_max_rps
                     << endl;
                exit(EXIT_FAILURE);
            }
            break;
        case 'n':
            total_event = atoi(optarg);
            break;
        case 't':
            event_type = atoi(optarg);
            if ((event_type < -1) ||
                (event_type >= event_generator::k_max_event_types)) {
                cerr << "option -"
                     << optopt
                     << " requires value in between -1 and "
                     << event_generator::k_max_event_types - 1
                     << endl;
                exit(EXIT_FAILURE);
            }
            break;
        case 'p':
            event_msg_pfx = string(optarg);
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

    if (total_event < rate_per_second) {
        cerr << "[-n total event] should be >= rate per second" << endl;
        exit(EXIT_FAILURE);
    }

    generator = event_generator::factory(rate_per_second, total_event,
                                          event_type, event_msg_pfx);
    if (generator == NULL) {
        cerr << "Failed to create event generator object" << endl;
        exit(EXIT_FAILURE);
    }
    generator->show_config();
    generator->generate_event();
    event_generator::destroy(generator);

    exit(EXIT_SUCCESS);
}
