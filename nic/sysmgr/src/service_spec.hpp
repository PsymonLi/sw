#ifndef __SERVICE_SPEC_HPP__
#define __SERVICE_SPEC_HPP__

#include <memory>
#include <string>
#include <vector>

#define DEFAULT_SPEC_FLAGS   0x0000
#define RESTARTABLE          0x0001
#define COPY_STDOUT_ON_CRASH 0x0002

enum service_spec_dep_kind {
    SERVICE_SPEC_DEP_SERVICE,
    SERVICE_SPEC_DEP_FILE,
};

class ServiceSpecDep {
public:
    enum service_spec_dep_kind kind;
    std::string file_name;
    std::string service_name;
    static std::shared_ptr<ServiceSpecDep> create();
};
typedef std::shared_ptr<ServiceSpecDep> ServiceSpecDepPtr;

class ServiceSpec {
public:
    std::string                    name;
    std::string                    command;
    std::vector<ServiceSpecDepPtr> dependencies;
    int                            flags;
    double                         timeout;
    static std::shared_ptr<ServiceSpec>   create();
};
typedef std::shared_ptr<ServiceSpec> ServiceSpecPtr;

#endif // __SERVICE_SPEC_HPP__
