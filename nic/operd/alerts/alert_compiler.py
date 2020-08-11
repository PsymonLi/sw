#!/bin/python3
# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

import json
import os
import sys

Categories = ['Cluster', 'Network', 'System', 'Rollout', 'Config', 'Resource',
              'Orchestration']

Severities = ['INFO', 'WARN', 'CRITICAL', 'DEBUG']

class Top():
    def __init__(self):
        self.alerts_ = []
        self.names_ = {}

    def add_alert(self, alert):
        self.alerts_.append(alert)
        if alert.name_ in self.names_:
            raise Exception('Duplicate name: %s' % (alert.name_))
        self.names_[alert.name_] = True

    def generate_implementation(self, f):
        def __add_empty_alert(fileHandle, alertName):
            fileHandle.write('\t{\n')
            fileHandle.write(f'\t\tname: "{alertName}",\n')
            fileHandle.write('\t\tcategory: "NONE",\n')
            fileHandle.write('\t\tseverity: "DEBUG",\n')
            fileHandle.write('\t\tdescription: "Invalid event",\n')
            fileHandle.write('\t\tmessage: NULL\n')
            fileHandle.write('\t},\n')
            return

        f.write('#include <cstdlib>\n')
        f.write('#include "alert_defs.h"\n')
        f.write('\n')
        f.write('namespace operd {\n')
        f.write('namespace alerts {\n')
        f.write('\n')
        # +2 to include OPERD_EVENT_TYPE_NONE and OPERD_EVENT_TYPE_MAX
        f.write('const alert_t alerts[%d] = {\n' % (len(self.alerts_)+2))
        __add_empty_alert(f, "OPERD_EVENT_TYPE_NONE")
        for alert in self.alerts_:
            f.write('\t{\n')
            f.write('\t\tname: "%s",\n' % (alert.name_))
            f.write('\t\tcategory: "%s",\n' % (alert.category_))
            f.write('\t\tseverity: "%s",\n' % (alert.severity_))
            f.write('\t\tdescription: "%s",\n' % (alert.description_))
            f.write('\t\tmessage: NULL\n')
            f.write('\t},\n')
        __add_empty_alert(f, "OPERD_EVENT_TYPE_MAX")
        f.write('};\n')
        f.write('\n')
        f.write('}\n')
        f.write('}\n')
        
    def generate_header(self, f):
        f.write('#ifndef __OPERD_ALERT_DEFS_H__\n')
        f.write('#define __OPERD_ALERT_DEFS_H__\n')
        f.write('\n')
        f.write('#include "nic/operd/alerts/alert_type.hpp"\n')
        f.write('\n')
        f.write('namespace operd {\n')
        f.write('namespace alerts {\n')
        f.write('\n')
        f.write('typedef enum operd_alerts_ {\n')
        f.write('\t%s = %d,\n' % ("OPERD_EVENT_TYPE_NONE", 0))
        f.write('\t%s = %d,\n' % ("OPERD_EVENT_TYPE_MIN", 1))
        numAlerts = len(self.alerts_)
        # first event to use OPERD_EVENT_TYPE_MIN
        f.write('\t%s = %s,\n' % (self.alerts_[0].name_, "OPERD_EVENT_TYPE_MIN"))
        for i in range(1, numAlerts):
            alert = self.alerts_[i]
            f.write('\t%s = %d,\n' % (alert.name_, i+1))
        f.write('\t%s = %d,\n' % ("OPERD_EVENT_TYPE_MAX", numAlerts+1))
        f.write('} operd_alerts_t;\n')
        f.write('\n')
        f.write('extern const alert_t alerts[%d];' % (numAlerts+2))
        f.write('\n')
        f.write('}\n')
        f.write('}\n')
        f.write('#endif\n')

class Alert():
    def __init__(self, name=None, category=None, severity=None,
                 description=None):
        if name is None or name == '':
            raise Exception(name, category, severity, description)
        if category not in Categories:
            raise Exception(name, category, severity, description)
        if severity not in Severities:
            raise Exception(name, category, severity, description)
        if description is None or description == '':
            raise Exception(name, category, severity, description)
        self.name_ = name
        self.category_ = category
        self.severity_ = severity
        self.description_ = description
        
def parse(f, top):
    alerts = json.load(f)
    for alert in alerts:
        top.add_alert(Alert(**alert))
        
def main():
    top = Top()
    outdir = sys.argv[1]
    for filename in sys.argv[2:]:
        with open(filename) as f:
            parse(f, top)
    with open(os.path.join(outdir, 'alert_defs.h'), 'w+') as f:
        top.generate_header(f)
    with open(os.path.join(outdir, 'alert_defs.cc'), 'w+') as f:
        top.generate_implementation(f)

if __name__ == '__main__':
    main()
