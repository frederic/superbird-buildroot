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
	parser.add_argument('-i', '--in', type=str, required=True, \
			dest='inf', help='Name of input RSA private key')
	parser.add_argument('-o', '--out', type=str, \
			default='rsa_pub_key.pem', \
			help="Name of output RSA public key, \
			default name is 'rsa_pub_key.pem'")

	return parser.parse_args()

def main():
	import array
	from Crypto.PublicKey import RSA

	args = get_args();
	infile = args.inf
	outfile = args.out

	inf = open(infile, 'r')
	key = RSA.importKey(inf.read())
	inf.close()
	keysize = key.size() + 1

	print 'Generating RSA public key from private key ...'
	print '    Input:   rsa_prv_key.name = ' + infile
	print '             rsa_prv_key.size = {}'.format(keysize)
	print '    Output:  rsa_pub_key.name = ' + outfile

	outf = open(outfile, 'w')
	outf.write(key.publickey().exportKey('PEM'))
	outf.close()

if __name__ == "__main__":
	main()
