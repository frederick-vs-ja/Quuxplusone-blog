#!/usr/bin/env python

# Run this program from the 'hana' directory:
#   git clone https://github.com/boostorg/hana/
#   cd hana
#   ./harness.py run
# It spews timings to the console. I used the
#   ./harness.py table
#   ./harness.py svg
# commands to parse and munge those timings into the format
# expected for the Markdown tables and SVG generator.

import os
import random
import shutil
import sys

COMPILER = '$HOME/llvm-project/build/bin/clang++'


def buildWithCommand(msg, cmakeCmd):
    os.mkdir('build')
    os.chdir('build')
    os.system(cmakeCmd)
    print('!!!!!! ' + msg)
    os.system('time make -j1 compile.test.tuple >/dev/null')
    os.chdir('..')
    shutil.rmtree('build')


def cmakeCommand(link, buildType, noBuiltin):
    assert link in [True, False]
    assert buildType in ['Debug', 'RelWithDebInfo', 'Release']
    cmd = 'cmake ..'
    cmd += ' -DCMAKE_CXX_COMPILER=%s' % COMPILER
    cmd += ' -DCMAKE_BUILD_TYPE=%s' % buildType
    if noBuiltin:
        cmd += ' -DCMAKE_CXX_FLAGS=-fno-builtin-std-forward'
    if not link:
        cmd += ' -DCMAKE_CXX_LINK_EXECUTABLE=/usr/bin/true'
    cmd += ' >/dev/null'
    return cmd


def createCheckout(source):
    if source == 'detail':
        os.system('git stash --quiet; git checkout --quiet 540f665e51~; git stash pop --quiet')
    elif source == 'std':
        os.system('git stash --quiet; git checkout --quiet 540f665e51~')
        os.system("git grep -l 'std::forward' | xargs sed -i -e 's/::boost::hana::detail::std::forward/::std::forward/g'")
        os.system("git grep -l 'std::forward' | xargs sed -i -e 's/boost::hana::detail::std::forward/::std::forward/g'")
        os.system("git grep -l 'std::forward' | xargs sed -i -e 's/hana::detail::std::forward/::std::forward/g'")
        os.system("git grep -l 'std::forward' | xargs sed -i -e 's/detail::std::forward/::std::forward/g'")
        os.system("find . -name '*-e' -delete")
        os.system('git commit -a -m dummy')
        os.system('git stash pop --quiet')
    elif source == 'cast':
        os.system('git stash; git checkout --quiet 540f665e51; git stash pop --quiet')

assert len(sys.argv) == 2
assert sys.argv[1] in ['run', 'table', 'svg']
if sys.argv[1] == 'run':
    builds = 4 * [
        ('detail -O0', 'detail', 'Debug', False, False),
        ('detail -O0 no-builtin', 'detail', 'Debug', True, False),
        ('detail -O2', 'detail', 'RelWithDebInfo', False, False),
        ('detail -O2 no-builtin', 'detail', 'RelWithDebInfo', True, False),
        ('detail -O3', 'detail', 'Release', False, False),
        ('detail -O3 no-builtin', 'detail', 'Release', True, False),
        ('std -O0', 'std', 'Debug', False, False),
        ('std -O0 no-builtin', 'std', 'Debug', True, False),
        ('std -O2', 'std', 'RelWithDebInfo', False, False),
        ('std -O2 no-builtin', 'std', 'RelWithDebInfo', True, False),
        ('std -O3', 'std', 'Release', False, False),
        ('std -O3 no-builtin', 'std', 'Release', True, False),
        ('cast -O0', 'cast', 'Debug', False, False),
        ('cast -O0 no-builtin', 'cast', 'Debug', True, False),
        ('cast -O2', 'cast', 'RelWithDebInfo', False, False),
        ('cast -O2 no-builtin', 'cast', 'RelWithDebInfo', True, False),
        ('cast -O3', 'cast', 'Release', False, False),
        ('cast -O3 no-builtin', 'cast', 'Release', True, False),
        ('detail -O0 including linker', 'detail', 'Debug', True, True),
    ]
    random.shuffle(builds)
    for (msg, source, buildType, noBuiltin, link) in builds:
        createCheckout(source)
        buildWithCommand(msg, cmakeCommand(link, buildType, noBuiltin))
elif sys.argv[1] == 'table':
    def doit():
        line = sys.stdin.readline()
        timing = line.split()[1]
        minsec = timing.split('m')
        return float(minsec[0])*60 + float(minsec[1][:-1])
    while True:
        r = doit()
        u = doit()
        s = doit()
        print('| %.3fs | %.3fs | %.3fs |' % (u,s,r))
elif sys.argv[1] == 'svg':
    def doit():
        line = sys.stdin.readline().split()
        assert line[0] == '|'
        assert line[3] == '|'
        assert line[5] == '|'
        assert line[7] == '|'
        assert line[9] == '|'
        assert line[8][-1] == 's'
        return line[8][:-1]
    while True:
        a = doit()
        b = doit()
        c = doit()
        d = doit()
        print('%s, %s, %s, %s' % (a,b,c,d))
