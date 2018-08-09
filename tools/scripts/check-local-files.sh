#!/bin/bash

A=$(git status  --porcelain=v1 --ignore-submodules=all | egrep -v '^[MADRC] ' | grep -v gitmodules | egrep -v '^DD ' | awk '{print $2}')
if [ -n "$A" ] ; then
    printf "=================================================================\n"
    printf "Failing compilation due to presence of locally modified files/conflicting changes for files : %s \n"  $A
    echo
    printf "Local changes i.e git-diff output of the tree follows. You can typically ignore changes to gitmodules and look into rest of diff..: \n"
    echo
    git --no-pager diff --ignore-submodules=all
    echo
    printf "**** This typically means the tree needs to be rebased to top of git-repo and any locally generated files need to be commited and PR needs to be resubmitted ****\n"
    printf "=================================================================\n"
	exit 1
fi
echo No uncomitted locally modified files
exit 0
