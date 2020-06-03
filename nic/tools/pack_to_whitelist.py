#!/usr/bin/env python3

#
# Convert a pack_xxx.txt file to a whitelist.
# Used to add items in pack_minigold.txt to the minigold whitelist.txt
#

import sys, os, fnmatch

packfile = sys.argv[1]

with open(packfile) as fp:
    d = fp.read()
for i in d.splitlines():
    words = i.split()
    if len(words) != 2 or i[0] == '#':
        continue
    fname = words[0][words[0].rindex('/')+1:]
    if words[1][-1:] == '/':
        print(words[1][1:] + fname)
    else:
        print(words[1][1:])
