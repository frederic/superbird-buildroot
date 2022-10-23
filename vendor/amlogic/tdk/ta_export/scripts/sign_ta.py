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
	parser.add_argument('--ta_rsa_key', required=True, help='Name of TA rsa private key file')
	parser.add_argument('--ta_rsa_key_sig', required=True, help='Name of TA rsa public key signature file')
	parser.add_argument('--ta_aes_key', type=str, default='null', help='Name of aes key file')
	parser.add_argument('--ta_aes_iv', type=str, default='null', help='Name of aes iv file')
	parser.add_argument('--ta_aes_key_iv_enc', type=str, default='null', help='Name of encryped aes key iv file')
	parser.add_argument('--ta_cvn', type=str, default='null', help='TA Current Version Number')
	parser.add_argument('--in', required=True, dest='inf', help='Name of input file')
	parser.add_argument('--out', type=str, default='null', help='Name of output file')

	return parser.parse_args()

def aes256_cbc_enc(key, iv, text):
	import struct
	from Crypto.Cipher import AES

	cipher = AES.new(key, AES.MODE_CBC, iv)

	#if text is not a multiple of 16, padding
	x = len(text) % 16
	if x != 0:
		text_pad = text + '0'*(16 - x)
	else:
		text_pad = text

	msg = cipher.encrypt(text_pad)

	return msg

def main():
	import sys
	import os
	import struct
	import array
	import uuid
	import md5
	from Crypto.Hash import SHA256
	from Crypto.Signature import PKCS1_v1_5
	from Crypto.Hash import SHA256
	from Crypto.PublicKey import RSA
	from Crypto.Util.number import long_to_bytes
	import binascii

	# parse arguments
	args = get_args()
	if args.out == 'null':
		args.out = args.inf

	# TA version
	if args.ta_cvn == 'null':
		ta_cvn = 0
	else:
		try:
			ta_cvn_str = args.ta_cvn.strip()
			version = int(ta_cvn_str.split('.')[0])
			assert version<256
			patchlevel = int(ta_cvn_str.split('.')[1])
			assert patchlevel<256
			sublevel = int(ta_cvn_str.split('.')[2])
			assert sublevel<256
		except:
			print "Bad format version!"
			return 0
		else:
			ta_cvn = version<<16|patchlevel<<8|sublevel

	# aes key
	if args.ta_aes_key == 'null':
		aes_key_type = 0
		aes_key = struct.pack('<IIIIIIII', \
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0)
		aes_iv = struct.pack('<IIII', \
			0x0, 0x0, 0x0, 0x0)
		enc_aes_key = struct.pack('<IIIIIIIIIIIIIIII', \
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0,
			0x0, 0x0, 0x0, 0x0)
		enc_aes_key_len = len(enc_aes_key)
	else:
		aes_key_type = 2
		f = open(args.ta_aes_key, 'rb')
		aes_key = f.read()
		f.close()
		f = open(args.ta_aes_iv, 'rb')
		aes_iv = f.read()
		f.close()
		f = open(args.ta_aes_key_iv_enc, 'rb')
		enc_aes_key = f.read()
		f.close()
		enc_aes_key_len = len(enc_aes_key)

	# ta private key
	f = open(args.ta_rsa_key, 'rb')
	ta_key = RSA.importKey(f.read())
	f.close()
	key_len = (ta_key.size() + 1) / 8

	# ta public key signature
	f = open(args.ta_rsa_key_sig, 'rb')
	ta_key_sig = f.read()
	sig_key_len = len(ta_key_sig)
	f.close()

	h_hdr = SHA256.new()
	digest_len = h_hdr.digest_size
	signer_ta = PKCS1_v1_5.new(ta_key)
	sig_len = len(signer_ta.sign(h_hdr))

	f = open(args.inf, 'rb+')
	shdr_len = struct.calcsize('<IIIIIIIIIIIIIIII')
	shdr_check = f.read(shdr_len)
	magic_check, version, flags, algo_check, arb_cvn, img_type_check,\
	img_size_check,rsv0_check, rsv1_check, rsv2_check, rsv3_check, rsv4_check,\
	rsv5_check, rsv6_check, rsv7_check, rsv8_check  = struct.unpack('<IIIIIIIIIIIIIIII', shdr_check)

	if img_type_check == 1:
		skip_len = shdr_len+digest_len+enc_aes_key_len
	elif img_type_check == 2:
		print 'TA has been signed, exit.'
		sys.exit(0)
	else:
		print 'ERROR: TA image type wrong!'
		sys.exit(1)

	f.seek(skip_len)
	img = f.read()
	f.close()

	img_size = len(img)

	magic = 0x4f545348	# SHDR_MAGIC
	version = 0x02000204# VERSION
	img_type = 2		# SHDR_TA_SIGNED
	algo = 0x70004830	# TEE_ALG_RSASSA_PKCS1_V1_5_SHA256
	arb_cvn = ta_cvn

	h_key = SHA256.new()
	shdr = struct.pack('<IIIIIIIIIIIIIIII', \
		magic, version, 0, algo, arb_cvn, img_type, img_size, aes_key_type,\
		0, 0, 0, 0, 0, 0, 0, 0)

	h_elf = SHA256.new()
	h_elf.update(img)
	h_hdr.update(shdr)
	h_hdr.update(enc_aes_key)
	h_hdr.update(h_elf.digest())
	sig_ta = signer_ta.sign(h_hdr)
	shdr_len = struct.calcsize('<IIIIIIIIIIIIIIII')

	print 'Signing TA ...'
	print '    Input:    ta_rsa_key.name = ' + args.ta_rsa_key
	print '              ta_rsa_key.size = {}'.format(ta_key.size() + 1)
	print '              ta_rsa_key.sig  = ' + args.ta_rsa_key_sig
	print '              ta_aes_key.name = ' + args.ta_aes_key
	print '              ta_aes_iv.name  = ' + args.ta_aes_iv
	print '       ta_aes_key_iv_enc.name = ' + args.ta_aes_key_iv_enc
	print '                      ta.name = ' + args.inf
	print '    Output:           ta.name = ' + args.out

	f = open(args.out, 'wb')
	f.write(shdr)
	f.write(enc_aes_key)
	f.write(h_elf.digest())
	f.write(sig_ta)

	skip_len = shdr_len + enc_aes_key_len + digest_len + sig_len
	f.seek(skip_len)
	for x in array.array("B", long_to_bytes(ta_key.publickey().n)):
		pub_n_ta = struct.pack("B", x)
		h_key.update(pub_n_ta)
		f.write(pub_n_ta)

	uuidStr = "{" + os.path.basename(args.inf).split(".")[0] + "}"
	uuid = uuid.UUID(uuidStr)
	h_key.update(uuid.bytes_le)

	skip_len = shdr_len + enc_aes_key_len + digest_len + sig_len + key_len
	f.seek(skip_len, 0)
	f.write(ta_key_sig)

	skip_len = shdr_len + enc_aes_key_len + digest_len + sig_len + key_len + sig_key_len
	f.seek(skip_len, 0)
	if aes_key_type == 2:
		f.write(aes256_cbc_enc(aes_key, aes_iv, img))
	else:
		f.write(img)
	f.close()

if __name__ == "__main__":
	main()
