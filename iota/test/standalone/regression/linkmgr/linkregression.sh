trap cleanup EXIT TERM INT
set -x

cleanup ()
{
	sshpass -p lab123 ssh -o StrictHostKeyChecking=no diag@192.168.69.245 "rm $LinkMgrTestPath/$FW"
}

FW=$1
LinkMgrTest=$2
LinkMgrTestPath=/home/diag/linkMGR
LinkMgrTestScript=$LinkMgrTestPath/link_stress_run.py 

if   [ $LinkMgrTest = "100Gb" ]; then
	sshpass -p lab123 ssh -o StrictHostKeyChecking=no diag@192.168.69.245 "$LinkMgrTestScript -s $LinkMgrTestPath/setup_6_100G_only.json -t 103 -v $LinkMgrTestScript/$FW.tar"
elif [ $LinkMgrTest = "40Gb" ]; then
	sshpass -p lab123 ssh -o StrictHostKeyChecking=no diag@192.168.69.245 "$LinkMgrTestScript -s $LinkMgrTestPath/setup_3_40G_only.json -t 103 -v /home/diag/${TARGET_ID}.tar"
elif [ $LinkMgrTest = "25Gb" ]; then
	sshpass -p lab123 ssh -o StrictHostKeyChecking=no diag@192.168.69.245 "$LinkMgrTestScript -s $LinkMgrTestPath/setup_4_10-25G_only.json -t 103 -v /home/diag/${TARGET_ID}.tar"
elif [ $LinkMgrTest = "100Gb_vomero" ]; then
	sshpass -p lab123 ssh -o StrictHostKeyChecking=no diag@192.168.69.245 "$LinkMgrTestScript -s $LinkMgrTestPath/setup_4_100g_vomero.json -t 103 -v /home/diag/${TARGET_ID}.tar"
fi

