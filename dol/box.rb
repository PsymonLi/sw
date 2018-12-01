from "registry.test.pensando.io:5000/pensando/nic:1.29"

env GOPATH: "/usr"
run "yum install tcpdump -y"
run "pip3 install mock zmq grpcio"

workdir "/sw"

copy "dol/entrypoint.sh", "/entrypoint.sh"
run "chmod +x /entrypoint.sh"

entrypoint "/entrypoint.sh"
