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
	parser.add_argument('--in', required=True, dest='inf', help='unsigned ta file')
	parser.add_argument('--out', type=str, default='null', help='signed ta file')
	parser.add_argument('--keydir', type=str, default='null', help='key file path')
	parser.add_argument('--encrypt', type=int, default=0, help='0 means no \
			encryption')

	return parser.parse_args()

def main():
	import sys
	import os
	import logging
	import subprocess

	log = logging.getLogger("Core.Analysis.Processing")
	INTERPRETER = "/usr/bin/python"

	file_path = str(sys.path[0])
	processor1 = file_path + "/gen_cert_key.py"
	processor2 = file_path + "/sign_ta.py"
	cmd1 = [INTERPRETER, processor1]
	cmd2 = [INTERPRETER, processor2]

	if not os.path.exists(INTERPRETER):
		log.error("Cannot find INTERPRETER at path \"%s\"." % INTERPRETER)

	# parse arguments
	args = get_args()
	target_path = os.path.abspath(os.path.dirname(args.inf));
	if args.out == 'null':
		args.out = args.inf

	uuid = "" + args.inf.split('/')[-1]
	uuid = uuid[:-3]

	if args.keydir == 'null':
		args.keydir = file_path + '/../keys/'

	cmd1.extend(["--root_rsa_key=" + args.keydir + "root_rsa_prv_key.pem"])
	cmd1.extend(["--ta_rsa_key=" + args.keydir + "ta_rsa_pub_key.pem"])
	cmd1.extend(["--uuid=" + uuid])
	cmd1.extend(["--ta_rsa_key_sig=" + target_path + "/ta_rsa_key.sig"])
	cmd1.extend(["--root_aes_key=" + args.keydir + "root_aes_key.bin"])
	cmd1.extend(["--ta_aes_key=" + args.keydir + "ta_aes_key.bin"])
	cmd1.extend(["--ta_aes_iv=" + args.keydir + "ta_aes_iv.bin"])
	cmd1.extend(["--ta_aes_key_iv_enc=" + target_path + "/ta_aes_key_enc.bin"])
	sub = subprocess.Popen(cmd1)
	sub.communicate()

	cmd2.extend(["--ta_rsa_key=" + args.keydir + "ta_rsa_prv_key.pem"])
	cmd2.extend(["--ta_rsa_key_sig=" + target_path + "/ta_rsa_key.sig"])
	if args.encrypt != 0:
		cmd2.extend(["--ta_aes_key=" + args.keydir + "ta_aes_key.bin"])
		cmd2.extend(["--ta_aes_iv=" + args.keydir + "ta_aes_iv.bin"])
		cmd2.extend(["--ta_aes_key_iv_enc=" + target_path + "/ta_aes_key_enc.bin"])

	cmd2.extend(["--in=" + args.inf])
	cmd2.extend(["--out=" + args.out])
	sub = subprocess.Popen(cmd2)
	sub.communicate()

	os.remove(target_path + "/ta_rsa_key.sig")
	os.remove(target_path + "/ta_aes_key_enc.bin")

if __name__ == "__main__":
	main()
