//
// {C} Copyright 2020 Pensando Systems Inc. All rights reserved
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementation for elbmon tool
/// usage: elbmon -v[erbose] -r[eset] -q[ueues] -p[cie] -b[wmon] -s[pps]
///  -l[lif] -t[qtype] -i[qid] -R[ring] -p[poll]<interval>
///
//===----------------------------------------------------------------------===//

#include <getopt.h>
#include <inttypes.h>
#include <iostream>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <unistd.h>

#include "nic/sdk/platform/pal/include/pal.h"

#include "generic.h"
#include "setup.h"
#include "config.h"
#include "option.h"
#include "task.h"
#include "pio.h"
#include "asset_elba.h"
#include "msg_undef.inc"
#include "msg_redef.inc"

using namespace std;
using namespace axitrace;

// Default options
int main (int argc, char *argv[])
{
    cout << "==============================================================================" << endl;
    cout << TOKEN_APP << TOKEN_INFO << " AXITRACE for Elba, Ver 1.0" << endl;
    cout << "==============================================================================" << endl;

    if (!axitrace::option::instance().parse(argc, argv)) {
        return 1;
    }


    //////////////////
    // setup
    ///////////////////////
    cout << TOKEN_APP << TOKEN_INFO << " Setup checking ... " << endl;
    if (!setup::instance().check()) {
        return 1;
    } 
    cout << TOKEN_APP << TOKEN_INFO << " Setup checking done successfully" << endl;
    setup::instance().show();

    ////cout << "[axitrace][INFO] Testing json parser ..." << endl;
    ////if (!config::instance().test_json()) {
    ////    return 1;
    ////}
    ////cout << "[axitrace][INFO] Testing json parser done successfully" << endl;

    /////////////////////////
    // cfg
    ///////////////////////////
    MY_INFO << " Reading cfg files ... " << endl;
    if (!config::instance().setup()) {
        MY_ERROR << " Reading cfg files FAILED" << endl;
        return 1;
    }
    MY_INFO << " Reading cfg files done successfully" << endl;


    ////////////////////////
    ////////PIO
    ////////////////////////
    cout << TOKEN_APP << TOKEN_INFO << " Setting up HW drivers ... " << endl;
    if (!pio::instance().setup()) {
        cout << TOKEN_APP << TOKEN_ERROR << " Setting up HW drivers FAILED " << endl;
        return 1;
    }
    cout << TOKEN_APP << TOKEN_INFO << " Setting up HW drivers done successfully" << endl;


    //////////////////////////////
    ///// driver calibrate
    //////////////////////////////
    if (axitrace::option::instance().opt["driver_calibrate_mode"][0].get<bool>()) {
        cout << TOKEN_APP << TOKEN_INFO << " Start running driver calibration (-dc) ..." << endl;
        if (!task::instance().run("driver_calibrate")) {
            cout << TOKEN_APP << TOKEN_ERROR << " Running driver calibration (-dc) FAILED" << endl;
            return 1;
        }
        cout << TOKEN_APP << TOKEN_INFO << " Running driver calibration (-dc) done successfully" << endl;
        return 0;
    }



    ///////////////////////////////
    // load asic cfg
    //////////////////////////////
    MY_INFO << " Setting up Elba assets ... " << endl;
    if (!asset::instance().setup()) {
        MY_ERROR << " Setting up Elba assets FAILED " << endl;
        return 1;
    }
    MY_INFO << " Setting up Elba assets done successfully" << endl;


    ////////////////////
    // print csr
    ///////////////////
    if (!task::instance().run()) {
        MY_ERROR << " JOB FAILED, errors found" << endl;
        return 1;
    }
    MY_INFO << " JOB DONE successfully" << endl;

    //asset::instance().test();


    return 0;
}
