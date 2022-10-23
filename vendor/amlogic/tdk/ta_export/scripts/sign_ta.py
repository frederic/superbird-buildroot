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
	parser.add_argument('--ta_rsa_key', dest = 'ta_prv_key', required = True, \
			help = 'ta rsa private key, input file')
	parser.add_argument('--ta_rsa_key_sig', dest = 'cert_sig', \
			required = True, \
			help = 'sig of ta cert header and ta pub key, input file')
	parser.add_argument('--ta_aes_key', type = str, default = 'null', \
			help = 'ta aes key, input file')
	parser.add_argument('--ta_aes_iv', type = str, default = 'null', \
			help = 'ta aes iv, input file')
	parser.add_argument('--ta_aes_key_iv_enc', dest = 'ta_aes_key_enc', \
			type = str, default = 'null', \
			help = 'encryped aes key and iv, input file')
	parser.add_argument('--ta_cvn', dest = 'arb_cvn', type = int, default = 0, \
			help = 'anti-rollback current version number')
	parser.add_argument('--ta_marketid', dest = 'market_id', type = int, \
			default = 0, help = 'market id')
	parser.add_argument('--chipset_id', type = int, default = 0, \
			help = 'chipset id')
	parser.add_argument('--arb_type', type = int, default = 0, \
			choices = [0, 1, 2], help = 'anti-rollback type, 0 = RPMB, \
			1 = OTP, 2 = Fixed Table')
	parser.add_argument('--in', dest = 'unsigned_ta', required = True, \
			help = 'unsigned ta, input file')
	parser.add_argument('--out', dest = 'signed_ta', type = str, \
			default = 'null', help = 'signed ta, output file')

	return parser.parse_args()

class ta_hdr():
	def __init__(self, ta_hdr_info):
		import struct

		self.__magic, self.__version, self.__flags, self.__algo, \
			self.__arb_cvn, self.__img_type, self.__img_size, self.__enc_type, \
			self.__arb_type = struct.unpack('<9I', ta_hdr_info)

		self.__version = 0x00000300
		self.__algo = 0x70004830
		self.__img_type = 2      # img type, 1:ta unsigned, 2:ta signed

		self.__rsv = [0] * 7     # 28bytes reserved
		self.__nonce = ''        # 16bytes nonce
		self.__ta_aes_key = ''   # 32bytes aes key
		self.__ta_aes_iv = ''    # 16bytes aes iv

	def update_attrs(self, arb_cvn, arb_type, enc_type, nonce, ta_aes_key, \
			ta_aes_iv):
		self.__arb_cvn = arb_cvn
		self.__arb_type = arb_type
		self.__enc_type = enc_type
		self.__nonce = nonce
		self.__ta_aes_key = ta_aes_key
		self.__ta_aes_iv = ta_aes_iv

	def serialize(self):
		import struct
		return struct.pack('<16I', self.__magic, self.__version, self.__flags, \
				self.__algo, self.__arb_cvn, self.__img_type, self.__img_size, \
				self.__enc_type, self.__arb_type, *self.__rsv) + \
				self.__nonce + self.__ta_aes_key + self.__ta_aes_iv

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

class ta_cert():
	def __init__(self, market_id, chipset_id):
		self.__cert_hdr = ta_cert_hdr(market_id, chipset_id)
		self.__ta_pub_key = ''  # 256bytes ta rsa pub key

		# 256bytes sig of cert header and ta pub key by root prv key
		self.__cert_sig = ''

		self.__ta_sig = ''      # 256bytes sig of ta by ta prv key

	def update_attrs(self, ta_pub_key, cert_sig, ta_sig, uuid):
		import struct
		import array
		from Crypto.Util.number import long_to_bytes

		for x in array.array("B", long_to_bytes(ta_pub_key.n)):
			self.__ta_pub_key += struct.pack("B", x)
		self.__cert_sig = cert_sig
		self.__ta_sig = ta_sig
		self.__cert_hdr.update_attrs(uuid)

	def serialize(self):
		import struct
		return self.__cert_hdr.serialize() + self.__ta_pub_key + \
			self.__cert_sig + self.__ta_sig

class signed_ta_hdr():
	def __init__(self, ta_hdr_info, market_id, chipset_id):
		self.__ta_hdr = ta_hdr(ta_hdr_info)
		self.__payload_digest = ''
		self.__ta_cert = ta_cert(market_id, chipset_id)

	def update_attrs(self, arb_cvn, arb_type, enc_type, market_id, chipset_id, \
			nonce, ta_aes_key, ta_aes_iv, payload_digest, ta_pub_key, \
			cert_sig, ta_sig, uuid):
		self.__ta_hdr.update_attrs(arb_cvn, arb_type, enc_type, nonce, \
				ta_aes_key, ta_aes_iv)
		self.__payload_digest = payload_digest
		self.__ta_cert.update_attrs(ta_pub_key, cert_sig, ta_sig, uuid)

	def serialize(self):
		import struct
		return self.__ta_hdr.serialize() + self.__payload_digest + \
			self.__ta_cert.serialize()

def aes256_cbc_enc(key, iv, src_data):
	from Crypto.Cipher import AES

	to_enc_data = src_data
	if (len(src_data) % 16) != 0:
		to_enc_data = src_data + '0' * (16 - (len(src_data) % 16))

	return AES.new(key, AES.MODE_CBC, iv).encrypt(to_enc_data)

def is_signed_ta(ta):
	import struct

	f = open(ta, 'rb')
	f.seek(20)
	img_type = struct.unpack('<I', f.read(4))[0]
	f.close()
	if img_type == 2:
		print 'ta has been signed'
		return True
	else:
		return False

def main():
	import sys
	import struct
	from Crypto.Signature import PKCS1_v1_5
	from Crypto.Hash import SHA256
	from Crypto.PublicKey import RSA

	args = get_args()

	if is_signed_ta(args.unsigned_ta) == True:
		sys.exit(0)

	enc_type = 0
	if args.ta_aes_key != 'null' and args.ta_aes_iv != 'null' and \
				args.ta_aes_key_enc != 'null':
		enc_type = 2
	elif args.ta_aes_key == 'null' and args.ta_aes_iv == 'null' and \
				args.ta_aes_key_enc == 'null':
		enc_type = 0
	else:
		print 'aes key file or iv file or enc aes key file is null, exit'
		sys.exit(0)

	f = open(args.unsigned_ta, 'rb')
	ta_hdr_info = f.read(36)
	f.seek(128)
	payload_digest = f.read(32)
	payload = f.read()
	f.close()

	f = open(args.ta_prv_key, 'rb')
	ta_prv_key = RSA.importKey(f.read())
	ta_pub_key = ta_prv_key.publickey()
	f.close()

	f = open(args.cert_sig, 'rb')
	cert_sig = f.read()
	f.close()

	enc_nonce = struct.pack('<4I', *([0] * 4))
	enc_ta_aes_key = struct.pack('<8I', *([0] * 8))
	enc_ta_aes_iv = struct.pack('<4I', *([0] * 4))
	if enc_type == 2:
		f = open(args.ta_aes_key_enc, 'rb')
		enc_nonce = f.read(16)
		enc_ta_aes_key = f.read(32)
		enc_ta_aes_iv = f.read(16)
		f.close()

		f_ta_aes_key = open(args.ta_aes_key, 'rb')
		f_ta_aes_iv = open(args.ta_aes_iv, 'rb')
		payload = aes256_cbc_enc(f_ta_aes_key.read(), f_ta_aes_iv.read(), \
				payload)
		f_ta_aes_key.close()
		f_ta_aes_iv.close()

	uuid = "" + args.unsigned_ta.split('/')[-1]
	uuid = uuid[:-3]

	# gen ta sig
	ta_hdr_obj = ta_hdr(ta_hdr_info);
	ta_hdr_obj.update_attrs(args.arb_cvn, args.arb_type, enc_type, \
			enc_nonce, enc_ta_aes_key, enc_ta_aes_iv)
	sha256 = SHA256.new()
	sha256.update(ta_hdr_obj.serialize())
	sha256.update(payload_digest)
	ta_sig = PKCS1_v1_5.new(ta_prv_key).sign(sha256)

	signed_hdr_obj = signed_ta_hdr(ta_hdr_info, args.market_id, args.chipset_id)
	signed_hdr_obj.update_attrs(args.arb_cvn, args.arb_type, enc_type, \
			args.market_id, args.chipset_id, enc_nonce, enc_ta_aes_key, \
			enc_ta_aes_iv, payload_digest, ta_pub_key, cert_sig, ta_sig, uuid)

	if args.signed_ta == 'null':
		args.signed_ta = args.unsigned_ta

	f = open(args.signed_ta, 'wb')
	f.write(signed_hdr_obj.serialize())
	f.write(payload)
	f.close()

	print 'Signing TA ...'
	print '  Input:                  arb_cvn = ' + str(args.arb_cvn)
	print '                        market_id = ' + str(args.market_id)
	print '                       chipset_id = ' + str(args.chipset_id)
	print '                         arb_type = ' + str(args.arb_type)
	print '                  ta_prv_key.name = ' + args.ta_prv_key
	print '                  ta_prv_key.size = {}'.format(ta_prv_key.size() + 1)
	print '                    cert_sig.name = ' + args.cert_sig
	print '                  ta_aes_key.name = ' + args.ta_aes_key
	print '                   ta_aes_iv.name = ' + args.ta_aes_iv
	print '              ta_aes_key_enc.name = ' + args.ta_aes_key_enc
	print '                 unsigned_ta.name = ' + args.unsigned_ta
	print '  Output:          signed_ta.name = ' + args.signed_ta

if __name__ == "__main__":
	main()
