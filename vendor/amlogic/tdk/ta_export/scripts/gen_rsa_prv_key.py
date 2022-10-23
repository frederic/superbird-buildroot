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
	import argparse

	parser = argparse.ArgumentParser()
	parser.add_argument('-s', '--size', type=int, \
			choices=[1024, 2048, 4096], default=2048, \
			help="size of RSA key, could be 1024/2048/4096, \
			default value is 2048")
	parser.add_argument('-o', '--out', type=str, \
			default='rsa_prv_key.pem', \
			help="Name of output RSA private key, \
			default name is 'rsa_prv_key.pem'")

	return parser.parse_args()

def main():
	import array
	from Crypto.PublicKey import RSA

	args = get_args();
	outfile = args.out
	keysize = args.size

	print 'Generating RSA private key ...'
	print '    Input:   rsa_prv_key.size = {}'.format(keysize)
	print '    Output:  rsa_prv_key.name = ' + outfile

	key = RSA.generate(keysize)
	f = open(outfile, 'w')
	f.write(key.exportKey('PEM'))
	f.close

if __name__ == "__main__":
	main()
