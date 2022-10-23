# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

include common.mk

CC_BINARY(stripe): bsdrm/example/stripe.o \
  CC_STATIC_LIBRARY(libbsdrm.pic.a)
