# env
export NIC_DIR='/nic'
export CONFIG_PATH=$NIC_DIR/conf/
export LD_LIBRARY_PATH=$NIC_DIR/lib:$NIC_DIR/conf/sdk:$NIC_DIR/conf/linkmgr:$NIC_DIR/conf/sdk/external:/usr/local/lib:/usr/lib/aarch64-linux-gnu:$LD_LIBRARY_PATH

kill -STOP `pidof hal`

echo "Step 1: Disable MAC"
/nic/bin/port_client -g localhost:50053 -o 4 -w 0 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 4 -w 1 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 4 -w 2 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 4 -w 3 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 4 -w 4 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 4 -w 5 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 4 -w 6 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 4 -w 7 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 3 -w 0 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 3 -w 1 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 3 -w 2 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 3 -w 3 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 3 -w 4 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 3 -w 5 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 3 -w 6 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 3 -w 7 -x 25 -y 1 -z 0

sleep 5

echo "Step 2: serdes init"
/nic/bin/aapl serdes-init -server 0.0.0.0 -port 9000 -addr 34-41 -divider 165 -width-mode 40
/nic/bin/aapl serdes -server localhost -port 9000 -addr 34-41 -pre 2 -post 6
/nic/bin/aapl serdes -server 0.0.0.0 -port 9000 -addr 34-41 -interrupt 0xd 0x800f
/nic/bin/aapl serdes -server 0.0.0.0 -port 9000 -addr 34-41 -interrupt 0xe 0x9e00
sleep 5
/nic/bin/aapl serdes -server 0.0.0.0 -port 9000 -addr 34-41 -tx-data core
/nic/bin/aapl serdes -server 0.0.0.0 -port 9000 -addr 34-41 -rx-input 0

sleep 5

echo "Step 3: MAC CFG"
/nic/bin/port_client -g localhost:50053 -o 2 -w 1 -x 0
/nic/bin/port_client -g localhost:50053 -o 2 -w 2 -x 0
/nic/bin/port_client -g localhost:50053 -o 33 -w 1 -x 0 -y 1027 -z 9216
/nic/bin/port_client -g localhost:50053 -o 33 -w 1 -x 0 -y 1028 -z 9232
/nic/bin/port_client -g localhost:50053 -o 33 -w 1 -x 0 -y 1029 -z 9232
/nic/bin/port_client -g localhost:50053 -o 33 -w 1 -x 1 -y 1027 -z 9216
/nic/bin/port_client -g localhost:50053 -o 33 -w 1 -x 1 -y 1028 -z 9232
/nic/bin/port_client -g localhost:50053 -o 33 -w 1 -x 1 -y 1029 -z 9232

sleep 5

echo "Step 4: MAC ENABLE"
/nic/bin/port_client -g localhost:50053 -o 3 -w 0 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 3 -w 1 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 3 -w 2 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 3 -w 3 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 3 -w 4 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 3 -w 5 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 3 -w 6 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 3 -w 7 -x 25 -y 1 -z 1
/nic/bin/port_client -g localhost:50053 -o 4 -w 0 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 4 -w 1 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 4 -w 2 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 4 -w 3 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 4 -w 4 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 4 -w 5 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 4 -w 6 -x 25 -y 1 -z 0
/nic/bin/port_client -g localhost:50053 -o 4 -w 7 -x 25 -y 1 -z 0

sleep 10

echo "Step 5: Serdes tuning"
/nic/bin/aapl serdes -server 0.0.0.0 -port 9000 -addr 34-41 -int 10 1

kill -CONT `pidof hal`
