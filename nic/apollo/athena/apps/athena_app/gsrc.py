#!/usr/bin/python3

'''
Example:
To create a customer src tree at /local/jason/orca1/zig1[/sw/nic/...]
from regular full tree at /local/jason/b1[/sw/nic/...]
rm -rf /local/jason/orca1/zig1 # delete the old tree
cd /local/jason/b1/sw/nic/apollo/athena/apps/athena_app
./gsrc.py --src /local/jason/b1 --dst /local/jason/orca1/zig1/src/github.com/pensando/
'''

from os import system, mkdir, chdir, path, makedirs, getcwd, environ
from  shutil import copytree, ignore_patterns, make_archive, copyfile, copymode, copy
import argparse
# from getDepHeaders import Parser
import sys
import os

asic = environ.get('ASIC', 'capri')

def parse_input ():
    parser = argparse.ArgumentParser(description="Create mini athena src tree")
    parser.add_argument('--dst', default='pensando', 
           help='Dst folder relative to /sw where all the src and Athena App should be copied', dest='dst')
    parser.add_argument('--src', default='.', 
           help='Src tree folder', dest='src')
    parser.add_argument('--spec', default='athena_spec', 
           help='src selection specification', dest='spec')
    args = parser.parse_args()
    return args

def copy_src(src, dst):
    with open(args.spec, 'r') as file:
        sl1 = file.readlines()
    sl2 = []
    for line in sl1:
        #sl2.append(line.rstrip('\n').rstrip('/').split('#')[0])
        sl2.append(line.rstrip('\n').split('#')[0])

    for line in sl2:
        if line.startswith('-'):
            dst1 = dst + '/' + line[1:]
            cmd = 'rm -rf ' + dst1
            os.system(cmd)
            print('R: %s' % cmd)
            continue

        if line.startswith('!'):
            dst1 = dst + '/' + line[1:]
            cmd = 'rm -rf ' + '`find ' + dst + '/sw' + ' -name ' + line[1:] + '`'
            os.system(cmd)
            print('R: %s' % cmd)
            continue

        #if line.startswith('#') or line.startswith('-'):
        if not line.startswith('/'):
            print('N: %s' % line)
            continue

        if line.find(' ') == -1:
            dst1 = dst + line
            src1 = src + line
        else:
            dst1 = dst + line.split()[1]
            if line.split()[0].startswith('/tool/toolchain'):
                src1 = line.split()[0]
            else:
                src1 = src + line.split()[0]

            if '*' not in line:
                print("T: %s" % line)
                copy(src1, dst1)
                continue

        if '*' not in line:
            if os.path.exists(dst1):
                print("IGNORE: %s" % line)
            else:
                if os.path.isdir(src1):
                    copytree(src1, dst1, symlinks=True)
                    print("D: %s" % line)
                else:
                    if not os.path.isdir(os.path.dirname(os.path.abspath(dst1))):
                        os.makedirs(os.path.dirname(os.path.abspath(dst1)))
                        print("C: %s" % os.path.dirname(line))
                    copy(src1, dst1, follow_symlinks=False)
                    print("F: %s" % line)
        else:
            # if not os.path.isdir(os.path.dirname(os.path.abspath(dst1))):
            #    os.makedirs(os.path.dirname(os.path.abspath(dst1)))
            if dst1[-1] == '/':
                if not os.path.isdir(dst1):
                    print("C: %s" % dst1)
                    os.makedirs(dst1)
                cmd = 'cp -ar ' + src1 + ' ' + os.path.abspath(dst1)
            else:
                if not os.path.isdir(os.path.dirname(dst1)):
                    print("C: %s" % os.path.dirname(dst1))
                    os.makedirs(os.path.dirname(dst1))
                cmd = 'cp -ar ' + src1 + ' ' + os.path.dirname(os.path.abspath(dst1))
            # print("* %s" % line)
            print("* %s" % cmd)
            os.system(cmd)
    cmd = 'cd ' + src + '/sw; git rev-parse HEAD > ' + dst + '/sw/git_hash'
    print(cmd)
    os.system(cmd)
    cmd = 'cd ' + src + '/sw; git describe --tags > ' + dst + '/sw/git_tag'
    print(cmd)
    os.system(cmd)

    '''
    for line in sl2:
        if line.startswith('-'):
            dst1 = dst + '/' + line[1:]
            cmd = 'rm -rf ' + dst1
            os.system(cmd)
            print('R: %s' % cmd)

        if line.startswith('!'):
            dst1 = dst + '/' + line[1:]
            cmd = 'rm -rf ' + '`find ' + dst + '/sw' + ' -name ' + line[1:] + '`'
            os.system(cmd)
            print('R: %s' % cmd)
    '''

def copy_mod(dst):
    with open('/usr/src/github.com/pensando/sw/nic/apollo/athena/apps/athena_app/Makefile', 'r') as file:
        data = file.read().replace('\\\n', '')
    data1 = data.split("LDLIBS += ")
    lib1 = ' '.join((data1[1].split("ifeq")[0]).split()).replace('-l', 'lib')
    lib1 = lib1.split()
    print(lib1, len(lib1))
    print("aarch64/x86_64 common libs")
    lib2 = data1[2].split("endif")[0].replace('-l', 'lib')
    lib2 = lib2.split()
    print(lib2)
    print("x86_64 lib only")
    liba = lib1 + lib2
    print(liba)
    print(len(liba))

    d1 = dict() 
    rootdir=('/usr/src/github.com/pensando/sw')
    for x in liba:
        for folder, dirs, files in os.walk(rootdir):
            for file in files:
                # if file.endswith('.log'):
                if file == 'module.mk':
                    fullpath = os.path.join(folder, file)
                    with open(fullpath, 'r') as f:
                        for line in f:
                            if 'MODULE_TARGET' in line and x in line :
                                d1[x] = fullpath
                                print(fullpath)
                                print(line)
                                break
        # else:
        #    print("Did not find ", x)

    print('Dict len =%s ' % len(d1))

    d2 = {'libp4pd_athena_rxdma' : '/sw/nic/apollo/p4/athena_rxdma',
            'libp4pd_athena_txdma' : '/sw/nic/apollo/p4/athena_txdma',
            'libp4pd_athena' : '/sw/nic/apollo/p4/athena_p4',
            'libsdkp4utils' : '/sw/nic/sdk/lib/p4',
            'libsdklinkmgrcsr' : '/sw/nic/sdk/linkmgr/csr',
            'libnicmgr_athena' : '/sw/platform/src/lib/nicmgr/src',
            'libkvstore_lmdb' : '/sw/nic/sdk/lib/kvstore',
            'libsdkcapri_csrint' : '/sw/nic/sdk/platform/capri/csrint',
            'libpdsupg_nicmgr_stub' : '/sw/nic/apollo/nicmgr'}

    for x in d2.keys():
        d1[x] = d2[x]
    print('Dict len =%s ' % len(d1))
    print('lib path found')
    for x in liba:
        if x in d1.keys():
            # print('%s, %s' % (x, d1[x].replace('/usr/src/github.com/pensando', '').replace('/module.mk', '')))
            #print('%s, %s' % (x, d1[x].replace('/usr/src/github.com/pensando', '')))
            src1 = d1[x].replace('/module.mk', '')
            dst1 = dst + d1[x].replace('/module.mk', '').replace('/usr/src/github.com/pensando', '')
            dst2 = d1[x].replace('/module.mk', '').replace('/usr/src/github.com/pensando', '')
            # print('src=%s, dst=%s' % (src1, dst1))
            if os.path.exists(dst1):
                print('%s already exist, ignore' % dst2)
            else:
                copytree(src1, dst1)
                # print('cp -ar %s %s' % (src1, dst1))
                # print('%s %s' % (src1, dst1))
                print('%s' % dst2)

    print('Lib not found')
    for x in liba:
        if not x in d1.keys():
            print('%s ' % x)

    m1 = ['sw/nic/sdk/mkinfra', 'sw/nic/sdk/mkdefs', 'sw/nic/mkdefs', 'sw/nic/make']
    for x in m1:
        # copytree('/usr/src/github.com/pensando/' + x, dst + '/' + x)
        # print('cp -ar /usr/src/github.com/pensando/%s %s' % (x, dst + '/' + x)
        # print('%s %s' % (x, dst + '/' + x))
        print('/%s'% x)

    m2 = ['sw/nic/sdk/Makefile', 'sw/nic/Makefile']
    for x in m2:
        # copyfile('/usr/src/github.com/pensando/' + x, dst + '/' + x)
        # print('cp -ar /usr/src/github.com/pensando/%s %s' % (x, dst + '/' + x)
        # print('%s %s' % (x, dst + '/' + x))
        print('/%s'% x)

def build (arch="aarch64", clean=False):
    """
       Build Athena APP. 
       TODO: Need to get the return value and bailout if make failed.
    """
    cmds = {
       "x86_64" : "make PIPELINE=athena",
       "aarch64" : "make PIPELINE=athena ARCH=aarch64 PLATFORM=hw"
    }
    target = " athena_app.bin" if clean == False else " clean"
    if args.arch == "all" :
        tmp = [v+target for v in cmds.itervalues()]
        cmd = " && ".join(tmp)
    else:
       #cmd will return exception if arch is not aarch64 or x86_64
       cmd = cmds[arch]+target

    system(cmd)

def modifyMakeFile (dest):
    """
        Modify any static definitions in Makefile to reflect 
        new location.
    """
    system("pwd")
    fileLoc = "{0}/athena_app/Makefile".format(dest)
    with open(fileLoc , "rt") as fp:
        data = fp.read()
        #Modify TOPDIR to point to correct location
        wdata = data.replace("TOPDIR = ../../../..", "TOPDIR = ../")
    
    with open(fileLoc, "wt") as fp:
        fp.write(wdata)

def copy_gen (dest, arch="aarch64"):
    """
        Copy all auto generated files and libaries to dest.
        TODO: Handle Exceptions
    """
    if arch == "all" :

        copytree("nic/build/x86_64/athena/{0}/lib".format(asic),
             "{0}/nic/build/x86_64/athena/{1}/lib".format(dest, asic))

        #copytree("build/x86_64/athena/{0}/gen/p4gen/p4/include".format(asic),
        #      "{0}/nic/build/x86_64/athena/{1}/gen/p4gen/p4/include".format(dest, asic))

        copytree("nic/build/aarch64/athena/{0}/lib".format(asic),
             "{0}/nic/build/aarch64/athena/{1}/lib".format(dest, asic))

        #copytree("build/aarch64/athena/{0}/gen/p4gen/p4/include".format(asic),
        #      "{0}/nic/build/aarch64/athena/{1}/gen/p4gen/p4/include".format(dest, asic))
    else :
        copytree("nic/build/{0}/athena/{1}/lib".format(arch, asic),
             "{0}/nic/build/{1}/athena/{2}/lib".format(dest, arch, asic))

        #copytree("build/{0}/athena/{1}/gen/p4gen/p4/include".format(arch, asic),
        #      "{0}/nic/build/{1}/athena/{2}/gen/p4gen/p4/include".format(dest, arch, asic))

def copy_test_app (dest="nic"):
    """
        Copy athena_app code except for module.mk 
        and blg_pkg_athena files.
    """
    copytree("nic/apollo/athena/apps/athena_app", 
             "{0}/athena_app".format(dest),
             ignore=ignore_patterns("*.mk", "bld_pkg_athena", "*.sh", "*.py*"))

def copy_includes (dest, arch):
    """
    copytree("hal/third-party/spdlog/include", 
             "{0}/nic/hal/third-party/spdlog/include".format(dest))
    copytree("build/{0}/athena/{1}/out/pen_dpdk_submake/include".format(arch, asic), 
             "{0}/nic/sdk/dpdk/build/include".format(dest))
    #nic/include and nic/sdk/include have some dead symlinks
    try:
        copytree("include", 
             "{0}/nic/include".format(dest))
    except :
         pass
    try:
        copytree("sdk/include", 
             "{0}/nic/sdk/include".format(dest))
    except :
         pass
    copytree("apollo/api/impl/athena", 
             "{0}/nic/apollo/api/impl/athena".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("apollo/api/include", "{0}/nic/apollo/api/include".format(dest))
    copytree("p4/ftl_dev/include","{0}/nic/p4/ftl_dev/include".format(dest))
    copytree("p4/common","{0}/nic/p4/common".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("hal/pd", "{0}/nic/hal/pd".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile")) #try to copy only nic/hal/pd/pd.hpp
    copytree("apollo/core", "{0}/nic/apollo/core".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("apollo/upgrade","{0}/nic/apollo/upgrade".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("apollo/framework","{0}/nic/apollo/framework".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("apollo/include","{0}/nic/apollo/include".format(dest))
    copytree("apollo/p4/include","{0}/nic/apollo/p4/include".format(dest))
    copytree("apollo/test/api/utils","{0}/nic/apollo/test/api/utils".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("apollo/test/base", "{0}/nic/apollo/test/base".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("sdk/asic","{0}/nic/sdk/asic".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("sdk/p4/loader","{0}/nic/sdk/p4/loader".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("sdk/linkmgr","{0}/nic/sdk/linkmgr".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("sdk/lib", "{0}/nic/sdk/lib".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("sdk/upgrade", "{0}/nic/sdk/upgrade".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("sdk/platform/utils","{0}/nic/sdk/platform/utils".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("sdk/platform/ring","{0}/nic/sdk/platform/ring".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))

    copytree("sdk/platform/fru","{0}/nic/sdk/platform/fru".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("sdk/platform/devapi","{0}/nic/sdk/platform/devapi".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("sdk/platform/capri","{0}/nic/sdk/platform/capri".format(dest), 
             ignore=ignore_patterns("*.cc", "*.mk", "Makefile"))
    copytree("sdk/platform/pal/include",
             "{0}/nic/sdk/platform/pal/include".format(dest))
    copytree("sdk/model_sim/include","{0}/nic/sdk/model_sim/include".format(dest))

    copytree("sdk/third-party/googletest-release-1.8.0/googletest/include", 
        "{0}/nic/sdk/third-party/googletest-release-1.8.0/googletest/include/".format(dest))
    copytree("sdk/third-party/boost/include/", 
        "{0}/nic/sdk/third-party/boost/include/".format(dest))
    """
    parser = Parser(rwd="/usr/src/github.com/pensando/sw", arch=arch) 
    hfiles = parser.process("nic/apollo/athena/apps/athena_app")
    for src in hfiles:
        dirIdx = src.rfind("/")
        print("file is {0}, dirIdx is {1}".format(src, dirIdx))
        dstDir = "{0}/{1}".format(dest, src[:dirIdx])
        if not path.exists(dstDir):
            makedirs(dstDir)
        copyfile(src, "{0}/{1}".format(dest, src))
    """
       This script does not resolve .h/.hpp files which are #defined.
       ex:
       #  define BOOST_USER_CONFIG <boost/config/user.hpp>
       #  include BOOST_USER_CONFIG
       until this support comes in copy these files statically.
    """
    static_dirs = ["nic/sdk/third-party/boost/include/boost/mpl/aux_/preprocessed/gcc",
                   "nic/sdk/third-party/boost/include/boost/mpl/vector",
                   "nic/hal/third-party/google/include/google/protobuf",
                   "nic/hal/third-party/grpc/include"]

    for d in static_dirs :
        copytree(d, "{0}/{1}".format(dest, d))

    static_files = ["nic/sdk/third-party/boost/include/boost/config/user.hpp",
                    "nic/sdk/third-party/boost/include/boost/mpl/aux_/config/typeof.hpp",
                    "nic/sdk/third-party/boost/include/boost/mpl/front_fwd.hpp",
                    "nic/sdk/third-party/boost/include/boost/mpl/pop_front_fwd.hpp",
                    "nic/sdk/third-party/boost/include/boost/mpl/pop_back_fwd.hpp",
                    "nic/sdk/third-party/boost/include/boost/mpl/back_fwd.hpp",
                    "nic/sdk/third-party/boost/include/boost/mpl/plus.hpp",
                    "nic/sdk/third-party/boost/include/boost/mpl/aux_/arithmetic_op.hpp",
                    "nic/sdk/third-party/boost/include/boost/mpl/aux_/largest_int.hpp",
                    "nic/sdk/third-party/boost/include/boost/mpl/minus.hpp",
                    "nic/sdk/third-party/googletest-release-1.8.0/googletest/make/gtest.a",
                    "nic/build/{0}/athena/{1}/gen/proto/gogo.pb.h".format(arch, asic),
                    "nic/sdk/third-party/libev/include/ev.h"]
    for fp in static_files:
        idx = fp.rfind("/")
        src_dir = "{0}/{1}".format(dest, fp[:idx])
        if not path.exists(src_dir):
            makedirs(src_dir)
        copyfile(fp, "{0}/{1}".format(dest, fp))
    """   
    copytree("third-party/gflags/include/gflags", 
             "{0}/nic/third-party/gflags/include/gflags".format(dest))
    copytree("sdk/third-party/zmq/include", 
             "{0}/nic/sdk/third-party/zmq/include".format(dest))
    """
if __name__ == '__main__':
    args = parse_input()
    copy_src(args.src, args.dst)
