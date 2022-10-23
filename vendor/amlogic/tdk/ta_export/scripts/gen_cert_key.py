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
	parser.add_argument('--root_rsa_key', required=True, help='Name of root private key file')
	parser.add_argument('--ta_rsa_key', required=True, help='Name of TA public key file')
	parser.add_argument('--uuid', required=True, help='Name of TA UUID')
	parser.add_argument('--ta_rsa_key_sig', type=str, default='null', help='Name of TA key signature file')

	parser.add_argument('--root_aes_key', type=str, default='null', help='Name of root aes file')
	parser.add_argument('--ta_aes_key', type=str, default='null', help='Name of ta aes key file')
	parser.add_argument('--ta_aes_iv', type=str, default='null', help='Name of ta aes iv file')
	parser.add_argument('--ta_aes_key_iv_enc', type=str, default='null', help='Name of encrypted ta aes key iv file')

	return parser.parse_args()

def aes256_cbc_enc(key, iv, text):
	import struct
	from Crypto.Cipher import AES

	cipher = AES.new(key, AES.MODE_CBC, iv)

	#if text is not a multiple of 16, return
	x = len(text) % 16
	if x != 0:
		text_pad = text + '0'*(16 - x)
	else:
		text_pad = text

	msg = cipher.encrypt(text)

	return msg

def gen_nonce():
	import md5
	import uuid

	uuid = uuid.uuid4()

	h = md5.new()
	h.update(uuid.bytes_le)
	return h.digest()

def main():
	import struct
	import array
	import uuid
	from Crypto.Signature import PKCS1_v1_5
	from Crypto.Hash import SHA256
	from Crypto.PublicKey import RSA
	from Crypto.Util.number import long_to_bytes

	# parse arguments
	args = get_args()

	if args.root_aes_key != 'null' and args.ta_aes_key != 'null':
		root_aes_iv = struct.pack('<IIII', \
				0x0, 0x0, 0x0, 0x0)

		f = open(args.root_aes_key, 'rb')
		root_aes_key = f.read()
		f.close()

		f = open(args.ta_aes_key, 'rb')
		ta_aes_key = f.read()
		f.close()

		f = open(args.ta_aes_iv, 'rb')
		ta_aes_iv = f.read()
		f.close()

		nonce = gen_nonce()

		nonce_key_iv_enc = aes256_cbc_enc(root_aes_key, root_aes_iv, nonce+ta_aes_key+ta_aes_iv)
		if (nonce_key_iv_enc == 'null'):
			sys.exit(1)

		f = open(args.ta_aes_key_iv_enc, 'wb')
		f.write(nonce_key_iv_enc)
		f.close()

	f = open(args.root_rsa_key, 'rb')
	key_root = RSA.importKey(f.read())
	f.close()

	f = open(args.ta_rsa_key, 'rb')
	key_ta = RSA.importKey(f.read())
	f.close()

	print 'Generate Custom Key...'
	print '    Input:    root_rsa_key.name = ' + args.root_rsa_key
	print '              root_rsa_key.size = {}'.format(key_root.size() + 1)
	print '                ta_rsa_key.name = ' + args.ta_rsa_key
	print '                ta_rsa_key.size = {}'.format(key_ta.size() + 1)
	print '                           uuid = ' + args.uuid
	print '                   root_aes_key = ' + args.root_aes_key
	print '                     ta_aes_key = ' + args.ta_aes_key
	print '                     ta_aes_iv  = ' + args.ta_aes_key
	print '    Output:      ta_rsa_key.sig = ' + args.ta_rsa_key_sig
	print '                 ta_aes_key_enc = ' + args.ta_aes_key_iv_enc

	h_key = SHA256.new()
	for x in array.array("B", long_to_bytes(key_ta.publickey().n)):
		pub_n_ta = struct.pack("B", x)
		h_key.update(pub_n_ta)

	uuidStr = "{" + args.uuid + "}"
	uuid = uuid.UUID(uuidStr)
	h_key.update(uuid.bytes_le)

	signer_key = PKCS1_v1_5.new(key_root)
	sig_key = signer_key.sign(h_key)

	f = open(args.ta_rsa_key_sig, 'wb')
	f.write(sig_key)
	f.close()

if __name__ == "__main__":
	main()
