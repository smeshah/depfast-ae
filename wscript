#!/usr/bin/env python
APPNAME="janus"
VERSION="0.0"

import os
import sys
import glob
from waflib import Logs
from waflib import Options

pargs = ['--cflags', '--libs']
#BOOST_LIBS = 'BOOST_SYSTEM BOOST_FILESYSTEM BOOST_THREAD BOOST_COROUTINE'

def options(opt):
    opt.load("compiler_c")
    opt.load("compiler_cxx")
    opt.load(['boost', 'unittest_gtest'],
             tooldir=['.waf-tools'])
    opt.add_option('-g', '--use-gxx', dest='cxx',
                   default=False, action='store_true')
    opt.add_option('-c', '--use-clang', dest='clang',
                   default=False, action='store_true')
    opt.add_option('-p', '--enable-profiling', dest='prof',
                   default=False, action='store_true')
    opt.add_option('-d', '--debug', dest='debug',
                   default=False, action='store_true')
    opt.add_option('-t', '--enable-tcmalloc', dest='tcmalloc',
                   default=False, action='store_true')
    opt.add_option('-s', '--enable-rpc-statistics', dest='rpc_s',
                   default=False, action='store_true')
    opt.add_option('-P', '--enable-piece-count', dest='pc',
                   default=False, action='store_true')
    opt.add_option('-C', '--enable-conflict-count', dest='cc',
                   default=False, action='store_true')
    opt.add_option('-r', '--enable-logging', dest='log',
                   default=False, action='store_true')
    opt.add_option('-T', '--enable-txn-stat', dest='txn_stat',
                   default=False, action='store_true')
    opt.add_option('-D', '--disable-check-python', dest='disable_check_python',
                   default=False, action='store_true')
    opt.parse_args();

def configure(conf):
    _choose_compiler(conf)
    _enable_pic(conf)
    conf.load("compiler_c")
    conf.load("compiler_cxx unittest_gtest")
    conf.load("boost")

    _enable_tcmalloc(conf)
    _enable_cxx14(conf)
    _enable_debug(conf)
    _enable_profile(conf)
    _enable_rpc_s(conf)
    _enable_piece_count(conf)
    _enable_txn_count(conf)
    _enable_conflict_count(conf)
#    _enable_snappy(conf)
    #_enable_logging(conf)


    conf.env.append_value("CXXFLAGS", "-Wno-reorder")
    conf.env.append_value("CXXFLAGS", "-Wno-comment")
    conf.env.append_value("CXXFLAGS", "-Wno-unused-function")
    conf.env.append_value("CXXFLAGS", "-Wno-unused-variable")
    conf.env.append_value("CXXFLAGS", "-Wno-sign-compare")
    conf.check_boost(lib='system filesystem context thread coroutine')

    conf.env.append_value("CXXFLAGS", "-Wno-sign-compare")
    conf.env.append_value('INCLUDES', ['/usr/local/include'])
#    conf.check_cxx(lib='boost_system', use='BOOST_SYSTEM')
#    conf.check_cxx(lib='boost_filesystem', use='BOOST_FILESYSTEM')
#    conf.check_cxx(lib='boost_coroutine', use='BOOST_COROUTINE')

    # in case you use linuxbrew, uncomment the following 
#    conf.env.append_value('INCLUDES', [os.path.expanduser('~') + '/.linuxbrew/include'])
#    conf.env.append_value('LIBPATH', [os.path.expanduser('~') + '/.linuxbrew/lib'])
    conf.env.LIB_PTHREAD = 'pthread'
    conf.check_cfg(package='yaml-cpp', uselib_store='YAML-CPP', args=pargs)

    if sys.platform != 'darwin':
        conf.env.LIB_RT = 'rt'

    if Options.options.disable_check_python:
        pass
    else:
        conf.load("python")
        conf.check_python_headers()
        # check python modules
#        conf.check_python_module('tabulate')
#        conf.check_python_module('yaml')

def build(bld):
    _depend("src/rrr/pylib/simplerpcgen/rpcgen.py",
            "src/rrr/pylib/simplerpcgen/rpcgen.g",
            "src/rrr/pylib/yapps/main.py src/rrr/pylib/simplerpcgen/rpcgen.g")

#    _depend("rlog/log_service.h", "rlog/log_service.rpc",
#            "bin/rpcgen rlog/log_service.rpc")

    _depend("src/deptran/rcc_rpc.h src/deptran/rcc_rpc.py",
            "src/deptran/rcc_rpc.rpc",
            "bin/rpcgen --python --cpp src/deptran/rcc_rpc.rpc")

    _gen_srpc_headers()

    _depend("old-test/benchmark_service.h", "old-test/benchmark_service.rpc",
            "bin/rpcgen --cpp old-test/benchmark_service.rpc")

    bld.stlib(source=bld.path.ant_glob("extern_interface/scheduler.c"),
              target="externc",
              includes="",
              use="")

    bld.stlib(source=bld.path.ant_glob("src/rrr/base/*.cpp "
                                       "src/rrr/misc/*.cpp "
                                       "src/rrr/rpc/*.cpp "
                                       "src/rrr/reactor/*.cc"),
              target="rrr",
              includes="src src/rrr",
              uselib="BOOST",
              use="PTHREAD")

#    bld.stlib(source=bld.path.ant_glob("rpc/*.cc"), target="simplerpc",
#              includes=". rrr rpc",
#              use="base PTHREAD")

    bld.stlib(source=bld.path.ant_glob("src/memdb/*.cc"), target="memdb",
              includes="src src/rrr src/deptran src/base",
              use="rrr PTHREAD")

    bld.shlib(features="pyext",
              source=bld.path.ant_glob("src/rrr/pylib/simplerpc/*.cpp"),
              target="_pyrpc",
              includes="src src/rrr src/rrr/rpc",
              use="rrr simplerpc PYTHON")


    bld.program(source=bld.path.ant_glob("src/deptran/*.cc "
                                         "src/deptran/*/*.cc "
                                         "src/bench/*/*.cc"),
                target="deptran_server",
                includes="src src/rrr src/deptran ",
                uselib="YAML-CPP BOOST",
                use="externc rrr memdb PTHREAD PROFILER RT")

#
# waf helper functions
#
def _choose_compiler(conf):
    # use clang++ as default compiler (for c++11 support on mac)
    if Options.options.cxx:
        os.environ["CXX"] = "g++"
    elif (sys.platform == 'darwin' and "CXX" not in os.environ) or Options.options.clang:
        os.environ["CXX"] = "clang++"
        conf.env.append_value("CXXFLAGS", "-stdlib=libc++")
        conf.env.append_value("LINKFLAGS", "-stdlib=libc++")
        Logs.pprint("PINK", "libc++ used")
    else:
        Logs.pprint("PINK", "use system default compiler")

def _enable_rpc_s(conf):
    if Options.options.rpc_s:
        conf.env.append_value("CXXFLAGS", "-DRPC_STATISTICS")
        Logs.pprint("PINK", "RPC statistics enabled")

def _enable_piece_count(conf):
    if Options.options.pc:
        conf.env.append_value("CXXFLAGS", "-DPIECE_COUNT")
        Logs.pprint("PINK", "Piece count enabled")

def _enable_txn_count(conf):
    if Options.options.txn_stat:
        conf.env.append_value("CXXFLAGS", "-DTXN_STAT")
        Logs.pprint("PINK", "Txn stat enabled")

def _enable_conflict_count(conf):
    if Options.options.cc:
        conf.env.append_value("CXXFLAGS", "-DCONFLICT_COUNT")
        Logs.pprint("PINK", "Conflict count enabled")

def _enable_logging(conf):
    #if Options.options.log:
    conf.check(compiler='cxx', lib='aio', mandatory=True, uselib_store="AIO")
    conf.env.append_value("CXXFLAGS", "-DRECORD")
    conf.env.append_value("LINKFLAGS", "-laio")
    Logs.pprint("PINK", "Logging enabled")

def _enable_snappy(conf):
    Logs.pprint("PINK", "google snappy enabled")
    conf.env.append_value("LINKFLAGS", "-Wl,--no-as-needed")
    conf.env.append_value("LINKFLAGS", "-lsnappy")
    conf.env.append_value("LINKFLAGS", "-Wl,--as-needed")

def _enable_tcmalloc(conf):
    if Options.options.tcmalloc:
        Logs.pprint("PINK", "tcmalloc enabled")
        conf.env.append_value("LINKFLAGS", "-Wl,--no-as-needed")
        conf.env.append_value("LINKFLAGS", "-ltcmalloc")
        conf.env.append_value("LINKFLAGS", "-Wl,--as-needed")

def _enable_pic(conf):
    conf.env.append_value("CXXFLAGS", "-fPIC")
    conf.env.append_value("LINKFLAGS", "-fPIC")

def _enable_cxx14(conf):
    Logs.pprint("PINK", "C++14 features enabled")
    if sys.platform == "darwin":
        conf.env.append_value("CXXFLAGS", "-stdlib=libc++")
        conf.env.append_value("LINKFLAGS", "-stdlib=libc++")
    conf.env.append_value("CXXFLAGS", "-std=c++14")

def _enable_profile(conf):
    if Options.options.prof:
        Logs.pprint("PINK", "CPU profiling enabled")
        conf.env.append_value("CXXFLAGS", "-DCPU_PROFILE")
        conf.env.LIB_PROFILER = 'profiler'

def _enable_debug(conf):
    if Options.options.debug:
        Logs.pprint("PINK", "Debug support enabled")
        conf.env.append_value("CXXFLAGS", "-Wall -pthread -O0 -DNDEBUG -g "
                "-ggdb -DLOG_LEVEL_AS_DEBUG -DLOG_DEBUG -rdynamic -fno-omit-frame-pointer".split())
    else:
        conf.env.append_value("CXXFLAGS", "-g -pthread -O2 -DNDEBUG -DLOG_INFO".split())

def _properly_split(args):
    if args == None:
        return []
    else:
        return args.split()

def _gen_srpc_headers():
    for srpc in glob.glob("deptran/*/*.rpc"):
        target = os.path.splitext(srpc)[0]+'.h'
        _depend(target,
                srpc,
                "bin/rpcgen --cpp " + srpc)
    pass

def _depend(target, source, action):
    target = _properly_split(target)
    source = _properly_split(source)
    for s in source:
        if not os.path.exists(s):
            Logs.pprint('RED', "'%s' not found!" % s)
            exit(1)
    for t in target:
        if not os.path.exists(t):
            _run_cmd(action)
    if not target or min([os.stat(t).st_mtime for t in target]) < max([os.stat(s).st_mtime for s in source]):
        _run_cmd(action)

def _run_cmd(cmd):
    Logs.pprint('PINK', cmd)
    os.system(cmd)

