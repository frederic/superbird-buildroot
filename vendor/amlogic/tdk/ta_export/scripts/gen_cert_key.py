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
	parser.add_argument('--root_rsa_key', dest = 'root_prv_key', \
			required = True, help = 'root rsa private key, input file')
	parser.add_argument('--ta_rsa_key', dest = 'ta_pub_key', required = True, \
			help = 'ta rsa public key, input file')
	parser.add_argument('--uuid', required = True, help = 'ta UUID')
	parser.add_argument('--ta_rsa_key_sig', dest = 'cert_sig', \
			required = True, \
			help = 'sig of ta cert header and ta rsa public key, output file')
	parser.add_argument('--root_aes_key', type = str, default = 'null', \
			help = 'root aes key, input file')
	parser.add_argument('--ta_aes_key', type = str, default = 'null', \
			help = 'ta aes key, input file')
	parser.add_argument('--ta_aes_iv', type = str, default = 'null', \
			help = 'ta aes iv, input file')
	parser.add_argument('--ta_aes_key_iv_enc', dest = 'ta_aes_key_enc', \
				type = str, default = 'null', \
			help = 'encrypted ta aes key and iv, output file')
	parser.add_argument('--market_id', type = int, default = 0, \
			help = 'market id')
	parser.add_argument('--chipset_id', type = int, default = 0, \
			help = 'chipset id')

	return parser.parse_args()

class ta_cert_hdr():
	def __init__(self, market_id, chipset_id):
		self.__magic = 0x43455254        # header magic, value = "CERT"
		self.__version = 0x00000100      # cert version, 1.0
		self.__uuid = ''                 # 16bytes uuid
		self.__market_id = market_id     # market id
		self.__chipset_id = chipset_id   # chipset id
		self.__rsv = [0] * 8             # 32bytes reserved

	def update_attrs(self, in_uuid):
		import uuid
		uuid_str = "{" + in_uuid + "}"
		tmp_id = uuid.UUID(uuid_str)
		self.__uuid = tmp_id.bytes_le

	def serialize(self):
		import struct
		return struct.pack('<2I', self.__magic, self.__version) + \
				self.__uuid + struct.pack('<10I', self.__market_id, \
						self.__chipset_id, *self.__rsv)

def aes256_cbc_enc(key, iv, src_data):
	from Crypto.Cipher import AES

	to_enc_data = src_data
	if (len(src_data) % 16) != 0:
		to_enc_data = src_data + '0' * (16 - (len(src_data) % 16))

	return AES.new(key, AES.MODE_CBC, iv).encrypt(to_enc_data)

def gen_nonce():
	import md5
	import uuid

	tmp_id = uuid.uuid4()
	tmp_md5 = md5.new()
	tmp_md5.update(tmp_id.bytes_le)

	return tmp_md5.digest()

def main():
	import struct
	import array
	import uuid
	from Crypto.Signature import PKCS1_v1_5
	from Crypto.Hash import SHA256
	from Crypto.PublicKey import RSA
	from Crypto.Util.number import long_to_bytes

	args = get_args()

	if args.root_aes_key != 'null' and args.ta_aes_key != 'null' and \
			args.ta_aes_iv != 'null' and args.ta_aes_key_enc != 'null':
		f = open(args.root_aes_key, 'rb')
		root_aes_key = f.read()
		f.close()

		f = open(args.ta_aes_key, 'rb')
		ta_aes_key = f.read()
		f.close()

		f = open(args.ta_aes_iv, 'rb')
		ta_aes_iv = f.read()
		f.close()

		root_aes_iv = struct.pack('<4I', *([0] * 4))
		nonce = gen_nonce()

		nonce_key_iv_enc = aes256_cbc_enc(root_aes_key, root_aes_iv, \
				nonce + ta_aes_key + ta_aes_iv)
		if (nonce_key_iv_enc == 'null'):
			print 'encrypt ta aes key and ta aes iv failed'
			sys.exit(1)

		f = open(args.ta_aes_key_enc, 'wb')
		f.write(nonce_key_iv_enc)
		f.close()

	f = open(args.root_prv_key, 'rb')
	root_prv_key = RSA.importKey(f.read())
	f.close()

	f = open(args.ta_pub_key, 'rb')
	ta_pub_key = RSA.importKey(f.read())
	f.close()

	sha256 = SHA256.new()

	cert_hdr_obj = ta_cert_hdr(args.market_id, args.chipset_id)
	cert_hdr_obj.update_attrs(args.uuid)
	sha256.update(cert_hdr_obj.serialize())

	for x in array.array("B", long_to_bytes(ta_pub_key.publickey().n)):
		sha256.update(struct.pack("B", x))

	cert_sig = PKCS1_v1_5.new(root_prv_key).sign(sha256)

	f = open(args.cert_sig, 'wb')
	f.write(cert_sig)
	f.close()

	print 'Generate Custom Key...'
	print '  Input:                       uuid = ' + args.uuid
	print '                          market_id = ' + str(args.market_id)
	print '                         chipset_id = ' + str(args.chipset_id)
	print '                  root_prv_key.name = ' + args.root_prv_key
	print '                  root_prv_key.size = {}'.\
		format(root_prv_key.size() + 1)
	print '                    ta_pub_key.name = ' + args.ta_pub_key
	print '                    ta_pub_key.size = {}'.\
		format(ta_pub_key.size() + 1)
	print '                  root_aes_key.name = ' + args.root_aes_key
	print '                    ta_aes_key.name = ' + args.ta_aes_key
	print '                    ta_aes_iv.name  = ' + args.ta_aes_iv
	print '  Output:             cert_sig.name = ' + args.cert_sig
	if args.ta_aes_key_enc != 'null':
		print '                ta_aes_key_enc.name = ' + args.ta_aes_key_enc

if __name__ == "__main__":
	main()
