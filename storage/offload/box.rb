from "registry.test.pensando.io:5000/pensando/nic:1.40"

env GOPATH: "/usr"

workdir "/sw"
copy "storage/offload/entrypoint.sh", "/entrypoint.sh"
run "chmod +x /entrypoint.sh"

entrypoint "/entrypoint.sh"
