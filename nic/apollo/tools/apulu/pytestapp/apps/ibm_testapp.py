#
# Execute this script from nic/ directory
#
# Todo:
# a. Print help: --testbed is optional if using naples connected back to back
# b. Check if this script is executed from nic/ directory. Else bail.
# c. Configure workloads
# d. Configure static route - "ip route add 192.168.0.0/16 via 172.16.0.1"
# 
export PYTESTAPPROOT=/home/$USER/pytest
export PYTHONPATH=${PYTESTAPPROOT}/configobjects:${PYTESTAPPROOT}/proto
export ALLOW_GRPC_PORTS=1

mkdir -p ${PYTESTAPPROOT}
mkdir -p ${PYTESTAPPROOT}/apps
mkdir -p ${PYTESTAPPROOT}/configobjects
mkdir -p ${PYTESTAPPROOT}/proto
mkdir -p ${PYTESTAPPROOT}/proto/meta

cp build/aarch64/apulu/capri/gen/proto/* ${PYTESTAPPROOT}/proto/
cp build/aarch64/apulu/capri/gen/proto/meta/* ${PYTESTAPPROOT}/proto/meta/
cp apollo/tools/apulu/pytestapp/configobjects/* ${PYTESTAPPROOT}/configobjects/
cp apollo/tools/apulu/pytestapp/apps/* ${PYTESTAPPROOT}/apps/

cd ${PYTESTAPPROOT}

#User changes go here:
export OOB_NODE1=172.16.111.<???>                                        #DSC1 OOB address
export OOB_NODE2=172.16.111.<???>                                        #DSC2 OOB address
export TB_NAME="/vol/jenkins/iota/eq-testbeds/tb-eq<???>.json"           #Eg: tb-eq9.json

python3 apps/config_host1_naples.py $OOB_NODE1 --remote $OOB_NODE2 --testbed $TB_NAME
python3 apps/config_host2_naples.py $OOB_NODE2 --remote $OOB_NODE1 --testbed $TB_NAME
