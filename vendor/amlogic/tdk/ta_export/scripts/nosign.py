#!/usr/bin/python
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
	parser.add_argument('--in', required=True, dest='inf', \
			help='Name of in file')
	parser.add_argument('--out', required=True, help='Name of out file')
	return parser.parse_args()

def main():
	from Crypto.Hash import SHA256
	import struct

	args = get_args()

	f = open(args.inf, 'rb')
	img = f.read()
	f.close()

	h = SHA256.new()

	digest_len = h.digest_size
	img_size = len(img)

	magic = 0x4f545348	# SHDR_MAGIC
	version = 0x02000204# VERSION
	img_type = 1		# SHDR_TA
	algo = 0x70004830	# TEE_ALG_RSASSA_PKCS1_V1_5_SHA256
	shdr = struct.pack('<IIIIIIIIIIIIIIII', \
		magic, version, 0, algo, 0, img_type, img_size, 0,\
		0, 0, 0, 0, 0, 0, 0, 0)

	nonce = struct.pack('<IIII', \
		0, 0, 0, 0)
	aes_key = struct.pack('<IIIIIIII', \
		0, 0, 0, 0, 0, 0, 0, 0)
	aes_iv = struct.pack('<IIII', \
		0, 0, 0, 0)
	h.update(img)

	f = open(args.out, 'wb')
	f.write(shdr)
	f.write(nonce)
	f.write(aes_key)
	f.write(aes_iv)
	f.write(h.digest())
	f.write(img)
	f.close()

if __name__ == "__main__":
	main()
