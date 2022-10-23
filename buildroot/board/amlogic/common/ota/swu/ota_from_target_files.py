#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# author: zhigang.yu@amlogic.com
# contributor: jun.zhang@amlogic.com
# modifications:
# 2018.11.15 use tar package instead of zip to keep the file permission of rootfs

import sys

import os
import re
import zipfile
import tarfile
from StringIO import StringIO

try:
    from hashlib import sha1 as sha1
except ImportError:
    from sha import sha as sha1

if sys.version_info < (2, 4):
    raise RuntimeError('At least Python 2.4 is required')


def write_local_file(name, data):
    fp = open(name, "wb+")
    fp.write(data)
    fp.flush()
    fp.close()


class TarFile(object):
    def __init__(self, info, data):
        self.info = info
        self.info.name = self.__strip_leading_point__(info.name)
        self.data = data
        if data is not None:
            self.sha1 = sha1(data).hexdigest()
        else:
            self.sha1 = 0

    @staticmethod
    def __strip_leading_point__(s):
        return re.sub('^\./', '/', s)

    def add_to_tgz(self, tgz):
        if self.data is None:
            tgz.addfile(self.info)
        else:
            tgz.addfile(self.info, StringIO(self.data))

    def add_to_tgz_recursively(self, tgz_source_list, tgz_target):
        parents = self.info.name.split('/')
        level = len(parents) - 1
        for l in range(2, level):
            parent = '/'.join(parents[:l])
            tgz_target.addfile(tgz_source_list.get(parent).info)

        self.add_to_tgz(tgz_target)


def load_ota_package(filename):
    """load ota zip file and return zip object

  If filename is of the form "foo.zip+bar.zip", use foo.zip

  Returns zipobj where zipobj is a zipfile.ZipFile (of the
  main file), open for reading.
  """
    m = re.match(r"^(.*[.]zip)\+(.*[.]zip)$", filename, re.IGNORECASE)
    if m:
        filename = m.group(1)

    return zipfile.ZipFile(filename, "r")


def load_rootfs_package(z):
    """Load all the files from rootfs.tgz into a given tarfile
  """
    try:
        data = z.read("rootfs.tgz")
        tgz = tarfile.open(mode="r|gz", fileobj=StringIO(data))
        filelist = {}
        while True:
            t = tgz.next()
            if t is None:
                break
            if t.isdir() or t.islnk() or t.issym():
                filelist[t.name] = TarFile(info=t, data=None)
            else:
                filelist[t.name] = TarFile(info=t, data=tgz.extractfile(t).read())

        tgz.close()
        return filelist
    except KeyError as e:
        print e
    except tarfile.ReadError as e:
        print e
        return None


def write_update_script(output_dir, cmd):
    with open(os.path.join(output_dir, "update.sh"), 'a') as f:
        f.write(cmd)


def update_shell_start(output_dir, name, sha_s, sha_d):
    write_update_script(output_dir,
                        '''\
    value_tmp=$(sha1sum /tmp{0})
    if [ ${{value_tmp:0:40}} != "{1}" ]; then
        if [ ${{value_tmp:0:40}} != "{2}" ]; then
            exit 1
        fi
    fi\n'''.format(name, sha_s, sha_d))


def update_shell_preinst(output_dir):
    write_update_script(output_dir,
                        '''\
    echo increment check success!
fi

if [ $1 == "postinst" ]; then
    echo -----------swupdate update.sh postinst---------------------
    gunzip -c /tmp/rootfs.tgz | tar xf - -C /tmp/rootfs
''')


def update_shell_postinst(output_dir, cmd):
    write_update_script(output_dir,
                        '''\
    {}
'''.format(cmd))


def update_shell_end(output_dir):
    write_update_script(output_dir,
                        '''\
    umount /tmp/rootfs
    rm /tmp/rootfs -rf
    rm /tmp/rootfs.tgz -rf
fi
''')


def build_sw_image(new_zip, output_dir):
    script_path = os.path.join(output_dir, "increment.sh")
    cmd = 'chmod +x {0}; ./{0} {1} {2}'.format(script_path,
                                               output_dir,
                                               'emmc' if 'u-boot.bin' in new_zip.namelist() else 'nand'
                                               )
    os.system(cmd)


def WriteIncrementalOTAPackage(new_zip, old_zip, output_rootfs_tgz, output_dir):
    print "Writing local files..."
    files = [
        'boot.img',
        'dtb.img',
        'update.sh',
        'sw-description',
        'increment.sh',
        'swupdate-priv.pem',
        'u-boot.bin',
        'u-boot.bin.usb.bl2',
        'u-boot.bin.usb.tpl'
    ]

    for f in files:
        if f in new_zip.namelist():
            write_local_file(os.path.join(output_dir, f), new_zip.read(f))

    print "Loading new rootfs..."
    new_rootfs_tgz = load_rootfs_package(new_zip)
    new_rootfs_flist = new_rootfs_tgz.keys()
    print "Loading old rootfs..."
    old_rootfs_tgz = load_rootfs_package(old_zip)
    old_rootfs_flist = old_rootfs_tgz.keys()

    print "Checking rootfs modifications..."
    file_matched = list(set(new_rootfs_flist).intersection(set(old_rootfs_flist)))
    file_deleted = list(set(old_rootfs_flist) - set(new_rootfs_flist))
    file_added = list(set(new_rootfs_flist) - set(old_rootfs_flist))

    for f in file_added:
        print "  ++++++++++ %s" % f
        new_rootfs_tgz.get(f).add_to_tgz_recursively(new_rootfs_tgz, output_rootfs_tgz)

    for f in file_matched:
        nf = new_rootfs_tgz.get(f)
        of = old_rootfs_tgz.get(f)
        n = nf.sha1
        o = of.sha1
        # check symlink
        if n == 0:
            if nf.info.issym() or nf.info.islnk():
                if o != 0 or nf.info.linkname != of.info.linkname:
                    print "  ********** %s" % f
                    new_rootfs_tgz.get(f).add_to_tgz_recursively(new_rootfs_tgz, output_rootfs_tgz)
                    continue

        if n != o:
            print "  ********** %s" % f
            new_rootfs_tgz.get(f).add_to_tgz_recursively(new_rootfs_tgz, output_rootfs_tgz)
            update_shell_start(output_dir, f, o, n)

    output_rootfs_tgz.close()

    update_shell_preinst(output_dir)

    for f in file_deleted:
        print "  ---------- %s" % f
        update_shell_postinst(output_dir, 'rm /tmp/rootfs{} -rf'.format(f))

    update_shell_end(output_dir)
    build_sw_image(new_zip, output_dir)


def main():
    if len(sys.argv) < 4:
        print('Usage: %s <old.zip> <new.zip> <out_dir>' % sys.argv[0])
        return 1

    ozn = sys.argv[1]
    nzn = sys.argv[2]
    ofd = sys.argv[3]

    if os.path.isdir(ofd):
        print "loading old ota file: %s..." % ozn
        old_zip = load_ota_package(ozn)
        print "loading new ota file: %s..." % nzn
        new_zip = load_ota_package(nzn)

        output_rootfs_tgz = tarfile.open(os.path.join(ofd, "rootfs.tgz"), "w:gz")
        WriteIncrementalOTAPackage(new_zip, old_zip, output_rootfs_tgz, ofd)

    else:
        print "%s is not a dir" % (ofd)
        return 1

    print "done."


if __name__ == '__main__':
    sys.exit(main())
