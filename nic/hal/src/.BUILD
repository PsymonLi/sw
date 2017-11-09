package(default_visibility = ["//visibility:public"])
licenses(["notice"])  # MIT license

cc_library(
    name = "hal_src",
    srcs = ["utils.cc",
            "vrf.cc",
            "telemetry.cc",
            "tcpcb.cc",
            "qos.cc",
            "qos_api.cc",
            "nwsec.cc",
            "nwsec_api.cc",
            "network.cc",
            "lif_manager.cc",
            "lif_manager_base.cc",
            "l2segment.cc",
            "l2segment_api.cc",
            "interface.cc",
            "interface_api.cc",
            "if_utils.cc",
            "endpoint.cc",
            "endpoint_api.cc",
            "descriptor_aol.cc",
            "acl_api.cc",
            "wring.cc",
            "tlscb.cc",
            "session.cc",
            # "rdma.cc",
            "proxy.cc",
            "ipseccb.cc",
            "crypto_keys.cc",
            "acl.cc",
            ],
    hdrs = [
            "session.hpp",
            "utils.hpp",
            "vrf.hpp",
            "telemetry.hpp",
            "tcpcb.hpp",
            "qos.hpp",
            "nwsec.hpp",
            "network.hpp",
            "lif_manager.hpp",
            "lif_manager_base.hpp",
            "l2segment.hpp",
            "interface.hpp",
            "if_utils.hpp",
            "endpoint.hpp",
            "descriptor_aol.hpp",
            "wring.hpp",
            "tlscb.hpp",
            "rdma.hpp",
            "proxy.hpp",
            "ipseccb.hpp",
            "crypto_keys.hpp",
            "acl.hpp"],
    deps = ["//nic/include:base_includes",
            "//nic/hal:hal_includes",
            "//nic/utils/bm_allocator:bm_allocator",
            "//nic/utils/thread:thread",
            "//nic/utils/print:print",
            "//nic/utils/slab:slab",
            "//nic/utils/indexer:indexer",
            "//nic/utils/ht:ht",
            #"//nic/proto:types_proto",
            #"//nic/proto/hal:hal_proto",
            #"//nic/proto/agents:agents_proto",
            "//nic:gen_includes",
            "//nic/hal/pd/capri:capri_includes",
            "//nic/hal/pd/iris:iris_includes",
            "//nic/utils/host_mem:host_mem_includes",
            # "//nic/utils/host_mem:host_mem",
            "//nic/third-party/spdlog:spdlog"],
)
