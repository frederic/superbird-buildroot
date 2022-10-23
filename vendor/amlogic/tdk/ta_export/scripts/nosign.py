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
	parser.add_argument('--in', dest = 'inf_elf', required = True, \
			help = 'elf file, input file')
	parser.add_argument('--out', dest = 'outf_unsigned_ta', required = True, \
			help = 'unsigned ta, output file')

	return parser.parse_args()

class ta_hdr():
	def __init__(self, payload):
		self.__magic = 0x4f545348       # header magic, value = "OTSH"
		self.__version = 0x00000300     # ta header version
		self.__flags = 0
		self.__algo = 0x70004830        # sign algorithm
		self.__arb_cvn = 0              # anti-rollback cur version number
		self.__img_type = 1             # img type, 1:ta unsigned, 2:ta signed
		self.__img_size = len(payload)
		self.__enc_type = 0             # encrypt type
		self.__arb_type = 0   # anti-rollback type, 0:RPMB, 1:OTP, 2:Fixed Table
		self.__rsv = [0] * 7            # 28bytes reserved
		self.__nonce = [0] * 4          # 16bytes nonce
		self.__ta_aes_key = [0] * 8     # 32bytes aes key
		self.__ta_aes_iv = [0] * 4      # 16bytes aes iv

	def serialize(self):
		import struct
		return struct.pack('<16I', self.__magic, self.__version, self.__flags, \
				self.__algo, self.__arb_cvn, self.__img_type, self.__img_size, \
				self.__enc_type, self.__arb_type, *self.__rsv) + \
				struct.pack('<4I', *self.__nonce) + \
				struct.pack('<8I', *self.__ta_aes_key) + \
				struct.pack('<4I', *self.__ta_aes_iv)

class unsigned_ta_hdr():
	def __init__(self, payload):
		from Crypto.Hash import SHA256

		self.__ta_hdr = ta_hdr(payload)

		# genarate payload digest
		sha256 = SHA256.new()
		sha256.update(payload)
		self.__payload_digest = sha256.digest()

	def serialize(self):
		import struct
		return self.__ta_hdr.serialize() + self.__payload_digest

def main():
	args = get_args()

	f = open(args.inf_elf, 'rb')
	payload = f.read()
	f.close()

	f = open(args.outf_unsigned_ta, 'wb')
	f.write(unsigned_ta_hdr(payload).serialize())
	f.write(payload)
	f.close()

if __name__ == "__main__":
	main()
