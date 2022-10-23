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
	parser.add_argument('--out', required=True, help='Name of out file')
	return parser.parse_args()

def main():
	import uuid
	from Crypto.Hash import SHA256

	args = get_args()

	uuid = uuid.uuid4()

	h = SHA256.new()
	h.update(uuid.bytes_le)

	f = open(args.out, 'wb')
	f.write(h.digest())
	f.close()

	print 'Generate Custom Key...'
	print '    Output:      ta aes key = ' + args.out

if __name__ == "__main__":
	main()
