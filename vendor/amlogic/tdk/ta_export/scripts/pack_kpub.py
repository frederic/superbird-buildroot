#!/usr/bin/env python
#
# Copyright (C) 2016 Amlogic, Inc. All rights reserved.
#
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.
#

def get_args():
	from argparse import ArgumentParser

	parser = ArgumentParser()
	parser.add_argument('--rsk', dest = 'inf_rsk', type = str, \
			default = 'null', \
			help = 'root rsa public key(root sign key), input file')
	parser.add_argument('--rek', dest = 'inf_rek', type = str, \
			default = 'null', \
			help = 'root aes key(root encrypt key), input file')
	parser.add_argument('--pek', dest = 'provision_rek', type = str, default = 'null', \
			help = 'provision root encrypt key, input file')
	parser.add_argument('--in', dest = 'inf_os', required = True, \
			help = 'Secure OS image file, input file')
	parser.add_argument('--out', dest = 'outf_os', type = str, \
			default = 'null', help = 'output Secure OS image file')
	return parser.parse_args()

def get_key_base(outf_os):
	import struct

	outf_os.seek(0)
	ext_flag = struct.unpack('<I', outf_os.read(4))[0]
	if ext_flag != 0x12348765:
		key_base = 1024
	else:
		key_base = 1536
	return key_base

def calc_rsk_pos(key_base, key_idx):
	key_hdr_size = 8
	key_item_size = 1032
	if key_idx == 0:
		rsk_pos = key_base + key_hdr_size + key_item_size * key_idx * 2
	else:
		print 'calc_rsk_pos parameter error'
		sys.exit(0)
	return rsk_pos

def calc_rek_pos(key_base, key_idx):
	key_hdr_size = 8
	key_item_size = 1032
	if key_idx == 0:
		rek_pos = key_base + key_hdr_size + key_item_size * (key_idx * 2 + 1)
	elif key_idx == 1:
		rek_pos = key_base + key_hdr_size + key_item_size * key_idx * 2
	else:
		print 'calc_rek_pos parameter error'
		sys.exit(0)
	return rek_pos

def add_key_cnt(outf_os):
	import struct

	key_base = get_key_base(outf_os)
	key_cnt_pos = key_base + 4
	outf_os.seek(key_cnt_pos)
	key_cnt = struct.unpack('<I', outf_os.read(4))[0]
	key_cnt += 1

	key_magic = 0x524F4F54  #"ROOT"
	key_hdr = struct.pack('<II', key_magic, key_cnt)

	outf_os.seek(key_base)
	outf_os.write(key_hdr)

def insert_rsk(outf_os, rsk, rsk_pos, key_type):
	import struct
	import array
	from Crypto.Util.number import long_to_bytes

	outf_os.seek(rsk_pos)
	rsk_type = struct.unpack('<I', outf_os.read(4))[0]
	if rsk_type == 0:
		add_key_cnt(outf_os)

	rsk_type = key_type
	rsk_size = (rsk.size() + 1) / 8
	rsk_hdr = struct.pack('<II', rsk_type, rsk_size)

	outf_os.seek(rsk_pos)
	outf_os.write(rsk_hdr)
	for x in array.array("B", long_to_bytes(rsk.publickey().n)):
		outf_os.write(struct.pack("B", x))

def insert_rek(outf_os, rek, rek_pos, key_type):
	import struct

	outf_os.seek(rek_pos)
	rek_type = struct.unpack('<I', outf_os.read(4))[0]
	if rek_type == 0:
		add_key_cnt(outf_os)

	rek_type = key_type
	rek_size = len(rek)
	rek_hdr = struct.pack('<II', rek_type, rek_size)

	outf_os.seek(rek_pos)
	outf_os.write(rek_hdr)
	outf_os.write(rek)

def read_rsk(inf_rsk):
	from Crypto.PublicKey import RSA

	f = open(inf_rsk, 'rb+')
	rsk = RSA.importKey(f.read())
	f.close()
	return rsk

def read_rek(inf_rek):
	f = open(inf_rek, 'rb+')
	rek = f.read()
	f.close()
	return rek

def get_raw_os(inf_os):
	f = open(inf_os, 'rb+')
	raw_os = f.read()
	f.close()
	return raw_os

def print_hint(args):
	print 'Packing root key ...'
	print '    Input:'
	if args.inf_rsk != 'null':
		rsk_size = (read_rsk(args.inf_rsk).size() + 1) / 8 * 8
		print '               rsk.name = ' + args.inf_rsk
		print '               rsk.size = {}'.format(rsk_size)
	if args.inf_rek != 'null':
		rek_size = len(read_rek(args.inf_rek))
		print '               rek.name = ' + args.inf_rek
		print '               rek.size = ' + str(rek_size)
	if args.provision_rek != 'null':
		pro_rek_size = len(read_rek(args.provision_rek))
		print '     provision_rek.name = ' + args.provision_rek
		print '     provision_rek.size = ' + str(pro_rek_size)
	print '             image.name = ' + args.inf_os
	print '    Output:  image.name = ' + args.outf_os

def main():
	args = get_args()

	if args.outf_os == 'null':
		args.outf_os = args.inf_os

	if args.outf_os != args.inf_os:
		outf_os = open(args.outf_os, 'wb')
		outf_os.write(get_raw_os(args.inf_os))
		outf_os.close()

	outf_os = open(args.outf_os, 'rb+')
	if args.inf_rsk != 'null':
		key_idx = 0
		key_type = 0x00000001
		insert_rsk(outf_os, read_rsk(args.inf_rsk), \
				calc_rsk_pos(get_key_base(outf_os), key_idx), key_type)

	if args.inf_rek != 'null':
		key_idx = 0
		key_type = 0x00000002
		insert_rek(outf_os, read_rek(args.inf_rek), \
			calc_rek_pos(get_key_base(outf_os), key_idx), key_type)

	if args.provision_rek != 'null':
		key_idx = 1
		key_type = 0x00000003
		insert_rek(outf_os, read_rek(args.provision_rek), \
			calc_rek_pos(get_key_base(outf_os), key_idx), key_type)

	outf_os.close()

	print_hint(args)

if __name__ == "__main__":
	main()
