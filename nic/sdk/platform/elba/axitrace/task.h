// {C} Copyright 2020 Pensando Systems Inc. All rights reserved

#ifndef __AXITRACE_TASK_HH__
#define __AXITRACE_TASK_HH__

#include <map>
using namespace std;

namespace axitrace {


class job_base {
public:
    job_base(const string& n) { name = n; terminate = false; }
    virtual ~job_base() {}
    string                          name;
    virtual bool                    run();
    virtual bool                    pre_main() { return true; }
    virtual bool                    main() { return true; }
    virtual bool                    post_main() { return true; }
    virtual void                    collect_active_ports() const { assert(0); }
    bool                            terminate;
};


class task 
{
public:
    task();
    virtual ~task();
    static task*                    instancep();
    static task&                    instance();
    bool                            run(const string& jobname = "");
    bool                            run_driver_calibrate();
    void                            setup();
    void                            collect_active_ports() const;
    job_base*                       current;

private:
    map<string, job_base*>          m_jobs;
};


class job_driver_calibrate : public job_base
{
public:
    job_driver_calibrate(const string& n):job_base(n) {}
    virtual ~job_driver_calibrate() {}
    virtual bool                    main();
};


class job_show_filters : public job_base
{
public:
    job_show_filters(const string& n):job_base(n) {}
    virtual ~job_show_filters() {}
    virtual bool                    main();
    virtual bool                    post_main();
    virtual void                    collect_active_ports() const;
};


class job_trace: public job_base
{
public:
    job_trace(const string& n):job_base(n)  { restore_needed = false; }
    virtual ~job_trace()                    {}
    virtual bool                            pre_main();
    virtual bool                            main();
    virtual bool                            post_main();
    virtual void                            collect_active_ports() const;
    bool                                    restore_needed;
};



class job_write_test_buffer : public job_base
{
public:
    job_write_test_buffer(const string& n):job_base(n) { }
    virtual ~job_write_test_buffer()        { }
    virtual bool                            main();
};


} ///namespace axitrace


#endif //__AXITRACE_TASK_HH__
