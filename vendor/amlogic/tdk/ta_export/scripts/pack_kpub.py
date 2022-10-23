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
	parser.add_argument('--rsk', required=True, help='root signing key')
	parser.add_argument('--rek', required=True, help='root encrypt key')
	parser.add_argument('--in', required=True, dest='inf', help='input Secure OS image file')
	parser.add_argument('--out', type=str, default='null', help='output Secure OS image file')
	return parser.parse_args()

def main():
	import struct
	import array
	from Crypto.PublicKey import RSA
	from Crypto.Util.number import long_to_bytes

	key_size_max = 1024

	args = get_args()
	if args.out == 'null':
		args.out = args.inf

	kf = open(args.rsk, 'rb+')
	rsk = RSA.importKey(kf.read())
	kf.close

	kf = open(args.rek, 'rb+')
	rek = kf.read()
	kf.close

	# magic and key count
	magic = 0x534F5441
	key_count = 2

	key_hdr = struct.pack('<II', magic, key_count)
	key_hdr_len = len(key_hdr)

	# root sign key
	rsk_type = 0x70004830	# KEY_TYPE
	rsk_size = (rsk.size() + 1) / 8

	rsk_hdr = struct.pack('<II', \
		rsk_type, rsk_size)
	rsk_hdr_len = len(rsk_hdr)

	# root encrypt key
	rek_type = 0x10000110	# KEY_TYPE
	rek_size = len(rek)

	rek_hdr = struct.pack('<II', \
		rek_type, rek_size)
	rek_hdr_len = len(rek_hdr)

	inf = open(args.inf, 'rb+')
	raw = inf.read()
	inf.close()

	ext_flag = struct.unpack('i',raw[:4])
	if (ext_flag[0] != 0x12348765):
		key_offset = 1024
	else:
		key_offset = 1536

	outf = open(args.out, 'wb+')
	outf.write(raw)
	outf.seek(key_offset, 0)
	outf.write(key_hdr)
	outf.write(rsk_hdr)

	outf.seek(key_offset + key_hdr_len + rsk_hdr_len)
	for x in array.array("B", long_to_bytes(rsk.publickey().n)):
		pub_n_root = struct.pack("B", x)
		outf.write(pub_n_root)

	outf.seek(key_offset + key_hdr_len + rsk_hdr_len + key_size_max)
	outf.write(rek_hdr)
	outf.write(rek)

	outf.close()

	print 'Packing root public key ...'
	print '    Input:     rsk.name = ' + args.rsk
	print '               rsk.size = {}'.format(8 * rsk_size)
	print '               rek.name = ' + args.rek
	print '               rek.size = ' + str(len(rek))
	print '    Input:   image.name = ' + args.inf
	print '    Output:  image.name = ' + args.out

if __name__ == "__main__":
	main()
