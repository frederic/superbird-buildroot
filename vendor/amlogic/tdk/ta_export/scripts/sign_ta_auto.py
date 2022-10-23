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
	parser.add_argument('--keydir', type = str, default = 'null', \
			help = 'key file path')
	parser.add_argument('--encrypt', type = int, default = 0, \
			choices = [0, 1], help = '0 = not encrypt, 1 = encrypt')
	parser.add_argument('--arb_cvn', type = int, default = 0, \
			help = 'anti-rollback current version number')
	parser.add_argument('--arb_type', type = int, default = 0, \
			choices = [0, 1, 2], help = 'anti-rollback type, 0 = RPMB, \
			1 = OTP, 2 = Fixed Table')
	parser.add_argument('--market_id', type = int, default = 0, \
			help = 'market id')
	parser.add_argument('--chipset_id', type = int, default = 0, \
			help = 'chipset id')
	parser.add_argument('--in', dest = 'inf_unsigned_ta', required = True, \
			help = 'unsigned ta, input file')
	parser.add_argument('--out', dest = 'outf_signed_ta', type = str, \
			default = 'null', help = 'signed ta, output file')

	return parser.parse_args()

def is_signed_ta(ta):
	import struct

	with open(ta, 'rb') as f:
		__magic, __version, __flags, __algo, \
		__arb_cvn, __img_type, __img_size, __enc_type, \
		__arb_type = struct.unpack('<9I', f.read(36))
		if __img_type == 2:
			return True
		else:
			return False

def main():
	import sys
	import os
	import logging
	import subprocess

	args = get_args()

	if is_signed_ta(args.inf_unsigned_ta) == True:
		if args.outf_signed_ta == 'null':
			sys.exit(0)

		with open(args.inf_unsigned_ta, 'rb') as f:
			payload = f.read()
		with open(args.outf_signed_ta, 'wb') as f:
			f.write(payload)
			sys.exit(0);

	log = logging.getLogger("Core.Analysis.Processing")
	INTERPRETER = "/usr/bin/python"

	if not os.path.exists(INTERPRETER):
		log.error("Cannot find INTERPRETER at path \"%s\"." % INTERPRETER)

	file_path = str(sys.path[0])
	gen_cert_cmd = [INTERPRETER, file_path + "/gen_cert_key.py"]
	sign_cmd = [INTERPRETER, file_path + "/sign_ta.py"]

	target_path = os.path.abspath(os.path.dirname(args.inf_unsigned_ta))
	if args.outf_signed_ta == 'null':
		args.outf_signed_ta = args.inf_unsigned_ta

	uuid = "" + args.inf_unsigned_ta.split('/')[-1]
	uuid = uuid[:-3]

	if args.keydir == 'null':
		args.keydir = file_path + '/../keys/'

	gen_cert_cmd.extend(["--uuid=" + uuid])
	gen_cert_cmd.extend(["--market_id=" + str(args.market_id)])
	gen_cert_cmd.extend(["--chipset_id=" + str(args.chipset_id)])
	gen_cert_cmd.extend(["--root_rsa_key=" + args.keydir + \
			"root_rsa_prv_key.pem"])
	gen_cert_cmd.extend(["--ta_rsa_key=" + args.keydir + \
			"ta_rsa_pub_key.pem"])
	gen_cert_cmd.extend(["--ta_rsa_key_sig=" + target_path + "/cert.sig"])
	gen_cert_cmd.extend(["--root_aes_key=" + args.keydir + \
			"root_aes_key.bin"])
	gen_cert_cmd.extend(["--ta_aes_key=" + args.keydir + "ta_aes_key.bin"])
	gen_cert_cmd.extend(["--ta_aes_iv=" + args.keydir + "ta_aes_iv.bin"])
	gen_cert_cmd.extend(["--ta_aes_key_iv_enc=" + target_path + \
			"/ta_aes_key_enc.bin"])
	sub = subprocess.Popen(gen_cert_cmd)
	sub.communicate()

	sign_cmd.extend(["--ta_cvn=" + str(args.arb_cvn)])
	sign_cmd.extend(["--arb_type=" + str(args.arb_type)])
	sign_cmd.extend(["--ta_marketid=" + str(args.market_id)])
	sign_cmd.extend(["--chipset_id=" + str(args.chipset_id)])
	sign_cmd.extend(["--ta_rsa_key=" + args.keydir + "ta_rsa_prv_key.pem"])
	sign_cmd.extend(["--ta_rsa_key_sig=" + target_path + "/cert.sig"])
	if args.encrypt != 0:
		sign_cmd.extend(["--ta_aes_key=" + args.keydir + "ta_aes_key.bin"])
		sign_cmd.extend(["--ta_aes_iv=" + args.keydir + "ta_aes_iv.bin"])
		sign_cmd.extend(["--ta_aes_key_iv_enc=" + target_path + \
				"/ta_aes_key_enc.bin"])
	sign_cmd.extend(["--in=" + args.inf_unsigned_ta])
	sign_cmd.extend(["--out=" + args.outf_signed_ta])
	sub = subprocess.Popen(sign_cmd)
	sub.communicate()

	os.remove(target_path + "/cert.sig")
	os.remove(target_path + "/ta_aes_key_enc.bin")

if __name__ == "__main__":
	main()
