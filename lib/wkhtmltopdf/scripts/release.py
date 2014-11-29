#!/usr/bin/env python
#
# Copyright 2014 wkhtmltopdf authors
#
# This file is part of wkhtmltopdf.
#
# wkhtmltopdf is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# wkhtmltopdf is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with wkhtmltopdf.  If not, see <http:#www.gnu.org/licenses/>.

import os, sys, platform, subprocess, build

def get_build_targets():
    map = {}
    for k, v in build.BUILDERS.iteritems():
        if not v in map:
            map[v] = []
        map[v].append(k)
        map[v].sort()
    return map

def get_targets():
    if platform.system() == 'Windows':
        return ['msvc2013-win32', 'msvc2013-win64']
    elif platform.system() == 'Darwin':
        builders = ['osx']
    else:
        builders = ['source_tarball', 'linux_schroot', 'mingw64_cross']

    targets, map = [], get_build_targets()
    for builder in builders:
        targets.extend(map[builder])
    return targets

def build_target(basedir, target):
    build.message('*************** building: %s\n\n' % target)
    build.mkdir_p(basedir)
    log  = open(os.path.join(basedir, '%s.log' % target), 'w')
    proc = subprocess.Popen([sys.executable, 'scripts/build.py', target],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        bufsize=1, cwd=os.path.join(basedir, '..'))

    proc.stdin.close()
    for line in iter(proc.stdout.readline, ''):
        line = line.rstrip()+'\n'
        if '\r' in line:
            line = line[1+line.rindex('\r'):]
        build.message(line)
        log.write(line)
        log.flush()
    proc.stdout.close()
    return proc.wait() == 0

def main():
    rootdir = os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..'))
    basedir = os.path.join(rootdir, 'static-build')

    os.chdir(os.path.join(rootdir, 'qt'))
    build.shell('git clean -fdx')
    build.shell('git reset --hard HEAD')
    os.chdir(rootdir)
    build.shell('git clean -fdx')
    build.shell('git reset --hard HEAD')
    build.shell('git submodule update')

    status = {}
    for target in get_targets():
        if not build_target(basedir, target):
            status[target] = 'failed'
            continue
        status[target] = 'success'
        build.rmdir(os.path.join(basedir, target))

    build.message('\n\n\nSTATUS\n======\n')
    width = max([len(target) for target in status])
    for target in sorted(status.keys()):
        build.message('%s: %s\n' % (target.ljust(width), status[target]))

if __name__ == '__main__':
    main()
