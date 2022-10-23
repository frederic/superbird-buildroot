#!/usr/bin/env python
#
# Copyright (C) 2016 Amlogic, Inc. All rights reserved.
#
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.
#
#

def get_args():
	from argparse import ArgumentParser

	parser = ArgumentParser()
	parser.add_argument('--arb_table', required=True, help='TA antirollback table file')
	parser.add_argument('--in', required=True, dest='inf', help='input Secure OS image file')
	parser.add_argument('--out', type=str, default='null', help='output Secure OS image file')
	return parser.parse_args()

def ta_antirollback_table_check(infile):
	import uuid

	tmp = []

	try:
		f = open(infile, 'r')
		lines = f.readlines()
	except:
		print "Open File: %s fail!" %(infile)
		return False
	else:
		f.close

	for line in lines:
		line = line.strip()
		if not len(line) or line.startswith('#'):
			continue

		uuid_str = "{" + line.split(':')[0].strip() + "}"
		try:
			ta_uuid = uuid.UUID(uuid_str)
		except:
			print "Bad format UUID!"
			return False
		else:
			if ta_uuid in tmp:
				print "Dumplicate UUID: ",
				print ta_uuid
				return False
			tmp.append(ta_uuid)

	return True

def ta_antirollback_table_parser(infile, outfile):
	import struct
	import uuid
	import binascii

	count = 0

	try:
		f = open(infile, 'r')
		lines = f.readlines()
	except:
		print "Open File: %s fail!" %(infile)
		return 0
	else:
		f.close

	try:
		f = open(outfile, 'wb+')
	except:
		print "Open File: %s fail!" %(outfile)
		return 0

	for line in lines:
		line = line.strip()
		if not len(line) or line.startswith('#'):
			continue

		uuid_str = "{" + line.split(':')[0].strip() + "}"
		try:
			ta_uuid = uuid.UUID(uuid_str)
		except:
			print "Bad format UUID!"
			return 0
		else:
			ta_uuid_hex = binascii.hexlify(ta_uuid.bytes_le)
			ta_uuid_bin = binascii.a2b_hex(ta_uuid_hex)

		try:
			ta_ver_str = line.split(':')[1]
			version = int(ta_ver_str.split('.')[0])
			assert version<256
			patchlevel = int(ta_ver_str.split('.')[1])
			assert patchlevel<256
			sublevel = int(ta_ver_str.split('.')[2])
			assert sublevel<256
		except:
			print "Bad format version!"
			return 0
		else:
			ta_ver = struct.pack('<I', version<<16|patchlevel<<8|sublevel)

		f.write(ta_uuid_bin)
		f.write(ta_ver)
		count += 1

	f.close()

	return count

def main():
	import struct
	import os

	arb_table_length_max = 32
	tmpfile = "./tmp"

	args = get_args()
	if args.out == 'null':
		args.out = args.inf

	if not ta_antirollback_table_check(args.arb_table):
		print "BAD TA antirollback table format!"
		return -1

	count = ta_antirollback_table_parser(args.arb_table, tmpfile)
	if not count:
		exit(-1)
	if count > arb_table_length_max:
		print "TA antirollback table size exceed(max is %d)!" %(arb_table_length_max)
		exit(-2)

	try:
		f = open(tmpfile, 'rb+')
		arb = f.read()
	except:
		print "Open File: %s fail!" %(tmpfile)
		exit(-3)
	else:
		f.close
		os.remove(tmpfile)

	# magic and count
	magic = 0x40544154
	hdr = struct.pack('<II', magic, count)

	try:
		inf = open(args.inf, 'rb+')
		raw = inf.read()
	except:
		print "Open File: %s fail!" %(args.inf)
		exit(-4)
	else:
		inf.close()

	try:
		offset = raw.index("TAT@")
	except:
		print "Bad format of %s, pack fail" %(args.inf)
		exit(-5)

	try:
		outf = open(args.out, 'wb+')
		outf.write(raw)
		outf.seek(offset, 0)
		outf.write(hdr)
		outf.write(arb)
	except:
		print "Open File: %s fail!" %(args.outf)
	else:
		outf.close()

	print 'Packing TA antirollback table...'
	print '    Input:   arb_table.name = ' + args.arb_table
	print '    Input:       image.name = ' + args.inf
	print '    Output:      image.name = ' + args.out

if __name__ == "__main__":
	main()
