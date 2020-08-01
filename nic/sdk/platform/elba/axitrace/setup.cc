// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#include <memory>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <cstring>
#include <stdlib.h>
#include "setup.h"
#include "option.h"
#include "console.h"


namespace axitrace {

static unique_ptr<setup> setup_ptr = unique_ptr<setup>(new setup());

setup* setup::instancep()
{
    return setup_ptr.get();
}


setup& setup::instance()
{
    return *instancep();
}


setup::setup()
{
    char szbuff[1024];
    char* szptr;
    int bytes;
    struct stat stat_buff;
    string binpath;

    getcwd(szbuff, sizeof(szbuff));
    pwd = szbuff;

    bytes = readlink("/proc/self/exe", szbuff, sizeof(szbuff));
    szbuff[bytes] = 0;
    bin = szbuff;
    
    if (char* p = strrchr(szbuff, '/')) {
        *p = '\0';
        binpath = szbuff;
    }

    if ((szptr = getenv("ELBA_AXITRACE_ROOT"))) {
        root = szptr;
    } 
    else if ((szptr = getenv("SRC_TREE"))) {
        root = szptr;
    }
    else {
        bool find = false;
        if (realpath("../cfg/elba.cfg", szbuff)) {
            if (stat(szbuff, &stat_buff) == 0) {
                find = true;
                root = realpath("..", szbuff);
            }
        }
        if (!find && realpath("./cfg/elba.cfg", szbuff)) {
            if (stat(szbuff, &stat_buff) == 0) {
                find = true;
                root = realpath(".", szbuff);
            }
        }
        if (!find && realpath((binpath + "/../cfg/elba.cfg").c_str(), szbuff)) {
            if (stat(szbuff, &stat_buff) == 0) {
                find = true;
                root = realpath((binpath + "/../cfg/elba.cfg").c_str(), szbuff);
            }
        }
        if (!find && realpath((binpath + "/cfg/elba.cfg").c_str(), szbuff)) {
            if (stat(szbuff, &stat_buff) == 0) {
                find = true;
                root = realpath((binpath + "/cfg/elba.cfg").c_str(), szbuff);
            }
        }
        if (!find) {
            root = "/usr/src/github.com/pensando/sw/nic/sdk/platform/elba/axitrace"; ///in docker
            find = true;
        }
    }

    cfg = root + "/cfg";

}


setup::~setup()
{
}


void setup::show()
{
    cout << "asic\t\t\t\t: " << axitrace::option::instance().opt["asic"][0].get() << endl;
    cout << "mode\t\t\t\t: " << axitrace::option::instance().opt["mode"][0].get() << endl;
    cout << "bin\t\t\t\t: " << bin << endl;
    cout << "pwd\t\t\t\t: " << pwd << endl;
    cout << "root\t\t\t\t: " << root << endl;
    cout << "cfg\t\t\t\t: " << cfg << endl;
}


bool setup::check()
{
    if (root == "") {
        cout << "[axitrace][ERROR] Please define 'ELBA_AXITRACE_ROOT' in environment variables" << endl;
        return false; 
    }
    struct stat stat_buff;
    if (stat(cfg.c_str(), &stat_buff) != 0) {
        cout << "[axitrace][ERROR] Can't find 'cfg' directory, it may require to define 'ELBA_AXITRACE_ROOT' in environment variables" << endl;
        return false; 
    }

    system((string("mkdir -p ") + axitrace::option::inst()["odir"][0].get()).c_str());
    console::instance() << console::stream_control("cout");
    console::instance() << console::stream_control("console", axitrace::option::inst()["odir"][0].get() + "/console.log", (axitrace::option::inst()["clean_console"][0].get<bool>())? ios_base::trunc: ios_base::app);
    return true;
}



} ////namespace axitrace
