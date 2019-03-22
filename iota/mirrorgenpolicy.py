#! /usr/bin/python3
import argparse
import json
import os
import collections
import random

mirrorpolicy_template = {
    "type" : "netagent",
    "rest-endpoint"    : "api/mirror/sessions/",
    "object-key" : "meta.tenant/meta.namespace/meta.name",
    "objects" : [
    {
    "kind"                  : "MirrorSession",
    "meta": {
        "name"              : "mirror-2",
        "tenant"            : "default",
        "namespace"         : "default",
        "creation-time"     : "1970-01-01T00:00:00Z",
        "mod-time"          : "1970-01-01T00:00:00Z"
    },
    "spec": {
        "enable": True,
        "packet-size": 128,
        "stop-condition": {},
        "collectors": [
          {
             "type": "ERSPAN",
             "export-config": {
              "destination": "192.168.100.101",
              "transport": "TCP/8000"
            },
            "pcap-dir-name": ""
          }
        ],
        "match-rules": [
        ]
    },
    "status"                : {}
    }
    ]
}

def get_verif(dst_ip, src_ip, protocol, port, result):
    verif = {}
    verif['dst_ip'] = dst_ip
    verif['src_ip'] = src_ip
    verif['protocol'] = protocol
    verif['port'] = port
    verif['result'] = result
    return verif

def get_appconfig(protocol, port):
    app_config = {}
    app_config['protocol'] = protocol
    app_config['port'] = port
    return app_config

def get_app_proto(protocol, dport):
    if protocol == 'icmp':
        proto_str = str(protocol).upper() + '/' + '0' + '/' + '0'
    else:
        if dport == 'any':
            proto_str = str(protocol).upper() + '/' + '0'
        else:
            proto_str = str(protocol).upper() + '/' + str(dport)
    app = {}
    app['ports'] = []
    app['ports'].append(proto_str)
    return app

def get_destination(dst_ip, protocol, port):
    if dst_ip == 'any':
        dst_ip = '0.0.0.0/0'
    dst = {}
    dst['ip-addresses'] = []
    dst['ip-addresses'].append(dst_ip)
    #dst['app-protocol-selectors'] = []
    #dst['app-protocol-selectors'].append(get_appconfig(protocol, port))
    return dst

def get_source(src_ip):
    if src_ip == 'any':
        src_ip = '0.0.0.0/0'
    src = {}
    src['ip-addresses'] = []
    src['ip-addresses'].append(src_ip)
    return src

def get_rule(dst_ip, src_ip, protocol, port, action):
    rule = {}
    rule['destination'] = get_destination(dst_ip, protocol, port)
    rule['source'] = get_source(src_ip)
    rule['app-protocol-selectors'] = get_app_proto(protocol, port)
    #rule['action'] = action
    return rule

parser = argparse.ArgumentParser(description='Naples Mirror Policy Generator')
parser.add_argument('--topology', dest='topology_dir', required = dir,
                    default=None, help='Path to the JSON file having IOTA endpoint information.')

GlobalOptions = parser.parse_args()
GlobalOptions.endpoint_file = GlobalOptions.topology_dir + "/endpoints.json"
GlobalOptions.protocols = ["udp", "tcp", "icmp"]
GlobalOptions.directories = ["udp", "tcp", "icmp", "mixed"]
#GlobalOptions.ports = ["10", "22", "24", "30", "50-100", "101-200", "201-250, 251-290", "10000-20000", "65535"]
GlobalOptions.ports = ["120"]
GlobalOptions.actions = ["mirror"]

def StripIpMask(ip_address):
    if '/' in ip_address:
        return ip_address[:-3].encode("ascii")
    return ip_address.encode("ascii")

def GetIpRange(ip_address):
    stripped_ip = ip_address[:ip_address.rfind('.')]
    return "{}.1-{}.100".format(stripped_ip, stripped_ip)

def GetIpCidr(ip_address):
    stripped_ip = ip_address[:ip_address.rfind('.')]
    return "{}.0/24".format(stripped_ip)

def Main():
    #import pdb; pdb.set_trace()
    with open(GlobalOptions.endpoint_file, 'r') as fp:
        obj = json.load(fp)
    EP = []

    for i in range(0, len(obj["objects"])):
        print("EP[%d] : %s" % (i, obj["objects"][i]["spec"]["ipv4-address"]))
        EP.append(StripIpMask(obj["objects"][i]["spec"]["ipv4-address"]))

    #EP.append(GetIpRange(EP[0]))
    EP = [ep.decode() for ep in EP]
    json.dump(EP, open("EP.json", "w"))
    GlobalOptions.topology_dir = GlobalOptions.topology_dir + '/gen/telemetry/mirror'
    for dir in GlobalOptions.directories:
        if not os.path.exists(GlobalOptions.topology_dir + "/{}".format(dir)):
            os.makedirs(GlobalOptions.topology_dir + "/{}".format(dir))

    # Specific match policy
    for protocol in GlobalOptions.protocols:
        for action in GlobalOptions.actions:
            mirrorpolicy = mirrorpolicy_template
            match_rules = mirrorpolicy_template['objects'][0]['spec']['match-rules']
            del match_rules[:]
            verif =[]
            for i in range(0, len(EP)):
                for j in range(i+1, len(EP)):
                    for k in GlobalOptions.ports:
                        rule = get_rule(EP[i], EP[j], protocol, k, action)
                        match_rules.append(rule)
                        verif.append(get_verif(EP[i], EP[j], protocol, k, action))
                        rule = get_rule(EP[j], EP[i], protocol, k, action)
                        match_rules.append(rule)
                        verif.append(get_verif(EP[j], EP[i], protocol, k, action))
            json.dump(verif, open(GlobalOptions.topology_dir +"/{}/{}_{}_specific_verif.json".format(protocol, protocol, action), "w"), indent=4)
            json.dump(mirrorpolicy, open(GlobalOptions.topology_dir + "/{}/{}_{}_specific_policy.json".format(protocol, protocol, action), "w"), indent=4)

    EP_SUBNET = []
    EP_SUBNET.append(GetIpCidr(EP[0]))
    # Subnet policy
    for protocol in GlobalOptions.protocols:
        for action in GlobalOptions.actions:
            mirrorpolicy = mirrorpolicy_template
            match_rules = mirrorpolicy_template['objects'][0]['spec']['match-rules']
            del match_rules[:]
            verif =[]
            for i in range(0, len(EP_SUBNET)):
                for j in range(0, len(EP)):
                    for k in GlobalOptions.ports:
                        rule = get_rule(EP_SUBNET[i], EP[j], protocol, k, action)
                        match_rules.append(rule)
                        verif.append(get_verif(EP_SUBNET[i], EP[j], protocol, k, action))
                        rule = get_rule(EP[j], EP_SUBNET[i], protocol, k, action)
                        match_rules.append(rule)
                        verif.append(get_verif(EP[j], EP_SUBNET[i], protocol, k, action))
            json.dump(mirrorpolicy, open(GlobalOptions.topology_dir +"/{}/{}_{}_subnet_policy.json".format(protocol, protocol, action), "w"), indent=4)
            json.dump(verif, open(GlobalOptions.topology_dir + "/{}/{}_{}_subnet_verif.json".format(protocol, protocol, action), "w"), indent=4)

    EP_ANY = []
    EP_ANY.append("any")
    GlobalOptions.ports.append("any")
    # Generic (any) Policy
    for protocol in GlobalOptions.protocols:
        for action in GlobalOptions.actions:
            mirrorpolicy = mirrorpolicy_template
            match_rules = mirrorpolicy_template['objects'][0]['spec']['match-rules']
            del match_rules[:]
            verif =[]
            for i in range(0, len(EP_ANY)):
                for j in range(i, len(EP)):
                    for k in GlobalOptions.ports:
                        rule = get_rule(EP_ANY[i], EP[j], protocol, k, action)
                        match_rules.append(rule)
                        verif.append(get_verif(EP_ANY[i], EP[j], protocol, k, action))
                        rule = get_rule(EP[j], EP_ANY[i], protocol, k, action)
                        match_rules.append(rule)
                        verif.append(get_verif(EP[j], EP_ANY[i], protocol, k, action))
            json.dump(mirrorpolicy, open(GlobalOptions.topology_dir + "/{}/{}_{}_any_policy.json".format(protocol, protocol, action), "w"), indent=4)
            json.dump(verif, open(GlobalOptions.topology_dir + "/{}/{}_{}_any_verif.json".format(protocol, protocol, action), "w"), indent=4)

    # Mixed Config
    del match_rules[:]
    verif =[]
    for protocol in GlobalOptions.protocols:
        for action in GlobalOptions.actions:
            mirrorpolicy = mirrorpolicy_template
            match_rules = mirrorpolicy_template['objects'][0]['spec']['match-rules']
            for i in range(0, len(EP)):
                for j in range(i+1, len(EP)):
                    for k in GlobalOptions.ports:
                        rule = get_rule(EP[i], EP[j], protocol, k, action)
                        match_rules.append(rule)
                        verif.append(get_verif(EP[i], EP[j], protocol, k, action))
                        rule = get_rule(EP[j], EP[i], protocol, k, action)
                        match_rules.append(rule)
                        verif.append(get_verif(EP[j], EP[i], protocol, k, action))
    json.dump(verif, open(GlobalOptions.topology_dir +"/{}/mirror_mixed_verif.json".format('mixed'), "w"), indent=4)
    json.dump(mirrorpolicy, open(GlobalOptions.topology_dir + "/{}/mirror_mixed_policy.json".format('mixed'), "w"), indent=4)

if __name__ == '__main__':
    Main()
