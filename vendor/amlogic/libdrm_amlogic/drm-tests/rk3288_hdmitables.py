#!/usr/bin/env python2
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

# Things seem to magically change in the tables at these TMDS rates.
# Specifically looking at NO pixel repetition in the table:
#
#   0       - 44.9    - output divider is 0b11
#   49.5    - 90.0    - output divider is 0b10
#   94.5    - 182.75  - output divider is 0b01
#   185.625 -         - output divider is 0b00
#
# You can also notice that MPLL charge pump settings change at similar times.

RATE1 = 46000000
RATE2 = 92000000
RATE3 = 184000000


def make_mpll(rate, depth, pixel_rep=0):
  assert pixel_rep == 0, "Untested with non-zero pixel rep and probably wrong"

  tmds = (rate * depth) / 8. * (pixel_rep + 1)

  if depth == 8:
    prep_div = 0
  elif depth == 10:
    prep_div = 1
  elif depth == 12:
    prep_div = 2
  elif depth == 16:
    prep_div = 3

  # Rates higher than 340MHz are HDMI 2.0
  # From tables, tmdsmhl_cntrl is 100% correlated with HDMI 1.4 vs 2.0
  if tmds <= 340000000:
    opmode = 0 # HDMI 1.4
    tmdsmhl_cntrl = 0x0
  else:
    opmode = 1 # HDMI 2.0
    tmdsmhl_cntrl = 0x3

  # Keep the rate within the proper range with the output divider control
  if tmds <= RATE1:
    n_cntrl       = 0x3  # output divider: 0b11
  elif tmds <= RATE2:
    n_cntrl       = 0x2  # output divider: 0b10
  elif tmds <= RATE3:
    n_cntrl       = 0x1  # output divider: 0b01
  else:
    n_cntrl       = 0x0  # output divider: 0b00

  # Need to make the dividers work out
  #
  # This could be done algorithmically, but let's not for now.  We show the
  # math to make this work out below as an assert.
  if n_cntrl == 0x3:
    if depth == 8:
      fbdiv2_cntrl  = 0x2  # feedback div1: / 2 (no +1)
      fbdiv1_cntrl  = 0x3  # feedback div2: / 4
      ref_cntrl     = 0x0  # input divider: / 1
    elif depth == 10:
      fbdiv2_cntrl  = 0x5  # feedback div1: / 5 (no +1)
      fbdiv1_cntrl  = 0x1  # feedback div2: / 2
      ref_cntrl     = 0x0  # input divider: / 1
    elif depth == 12:
      fbdiv2_cntrl  = 0x3  # feedback div1: / 3 (no +1)
      fbdiv1_cntrl  = 0x3  # feedback div2: / 4
      ref_cntrl     = 0x0  # input divider: / 1
    elif depth == 16:
      # Guess:
      fbdiv2_cntrl  = 0x4  # feedback div1: / 4 (no +1)
      fbdiv1_cntrl  = 0x3  # feedback div2: / 4
      ref_cntrl     = 0x0  # input divider: / 1

  elif n_cntrl == 0x2:
    if depth == 8:
      fbdiv2_cntrl  = 0x1  # feedback div1: / 1 (no +1)
      fbdiv1_cntrl  = 0x3  # feedback div2: / 4
      ref_cntrl     = 0x0  # input divider: / 1
    elif depth == 10:
      fbdiv2_cntrl  = 0x5  # feedback div1: / 5 (no +1)
      fbdiv1_cntrl  = 0x0  # feedback div2: / 1
      ref_cntrl     = 0x0  # input divider: / 1
    elif depth == 12:
      fbdiv2_cntrl  = 0x2  # feedback div1: / 2 (no +1)
      fbdiv1_cntrl  = 0x2  # feedback div2: / 3
      ref_cntrl     = 0x0  # input divider: / 1
    elif depth == 16:
      fbdiv2_cntrl  = 0x2  # feedback div1: / 2 (no +1)
      fbdiv1_cntrl  = 0x3  # feedback div2: / 4
      ref_cntrl     = 0x0  # input divider: / 1

  elif n_cntrl == 0x1:
    if depth == 8:
      fbdiv2_cntrl  = 0x1  # feedback div1: / 1 (no +1)
      fbdiv1_cntrl  = 0x1  # feedback div2: / 2
      ref_cntrl     = 0x0  # input divider: / 1
    elif depth == 10:
      fbdiv2_cntrl  = 0x5  # feedback div1: / 5 (no +1)
      fbdiv1_cntrl  = 0x0  # feedback div2: / 1
      ref_cntrl     = 0x1  # input divider: / 2
    elif depth == 12:
      fbdiv2_cntrl  = 0x1  # feedback div1: / 1 (no +1)
      fbdiv1_cntrl  = 0x2  # feedback div2: / 3
      ref_cntrl     = 0x0  # input divider: / 1
    elif depth == 16:
      fbdiv2_cntrl  = 0x1  # feedback div1: / 1 (no +1)
      fbdiv1_cntrl  = 0x3  # feedback div2: / 4
      ref_cntrl     = 0x0  # input divider: / 1

  elif n_cntrl == 0x0:
    if depth == 8:
      fbdiv2_cntrl  = 0x1  # feedback div1: / 1 (no +1)
      fbdiv1_cntrl  = 0x0  # feedback div2: / 1
      ref_cntrl     = 0x0  # input divider: / 1
    elif depth == 10:
      fbdiv2_cntrl  = 0x5  # feedback div1: / 5 (no +1)
      fbdiv1_cntrl  = 0x0  # feedback div2: / 1
      ref_cntrl     = 0x3  # input divider: / 4
    elif depth == 12:
      fbdiv2_cntrl  = 0x1  # feedback div1: / 1 (no +1)
      fbdiv1_cntrl  = 0x2  # feedback div2: / 3
      ref_cntrl     = 0x1  # input divider: / 2
    elif depth == 16:
      fbdiv2_cntrl  = 0x1  # feedback div1: / 1 (no +1)
      fbdiv1_cntrl  = 0x1  # feedback div2: / 2
      ref_cntrl     = 0x0  # input divider: / 1

  # Double check with math; this formula derived from the table.
  total_div = (fbdiv2_cntrl * (fbdiv1_cntrl + 1) * (1 << (3 - n_cntrl)) /
	       (ref_cntrl + 1))
  assert depth == total_div, \
	"Error with rate=%d, tmds=%d, depth=%d, n_cntrl=%d, pixel_rep=%d" % (
	  rate, tmds, depth, n_cntrl, pixel_rep)

  # Could be done by math, but this makes it more obvious I think...
  if n_cntrl == 3:
    gmp_cntrl = 0
  elif n_cntrl == 2:
    gmp_cntrl = 1
  elif n_cntrl == 1:
    gmp_cntrl = 2
  elif n_cntrl == 0:
    gmp_cntrl = 3

  return ((n_cntrl << 0) |
	  (ref_cntrl << 2) |
	  (fbdiv1_cntrl << 4) |
	  (fbdiv2_cntrl << 6) |
	  (opmode << 9) |
	  (tmdsmhl_cntrl << 11) |
	  (prep_div << 13),
	  gmp_cntrl)

def do_mpll_loop():
  mpll_cfg_table = {}
  last_mpll_cfg = None
  last_rate = None

  for rate in xrange(13500000, 600001000, 1000):
    for8bpp = make_mpll(rate, 8)
    for10bpp = make_mpll(rate, 10)
    for12bpp = make_mpll(rate, 12)

    mpll_cfg = (for8bpp, for10bpp, for12bpp)
    if (mpll_cfg != last_mpll_cfg) and (last_rate is not None):
      mpll_cfg_table[last_rate] = last_mpll_cfg

    last_rate = rate
    last_mpll_cfg = mpll_cfg

  mpll_cfg_table[last_rate] = last_mpll_cfg

  print "\t",
  for rate in sorted(mpll_cfg_table.keys()):
    print ("{\n"
	   "\t\t%d, {\n"
	   "\t\t\t{ %#06x, %#06x },\n"
	   "\t\t\t{ %#06x, %#06x },\n"
	   "\t\t\t{ %#06x, %#06x },\n"
	   "\t\t},\n"
	   "\t}, ") % (
	   rate,
	   mpll_cfg_table[rate][0][0], mpll_cfg_table[rate][0][1],
	   mpll_cfg_table[rate][1][0], mpll_cfg_table[rate][1][1],
	   mpll_cfg_table[rate][2][0], mpll_cfg_table[rate][2][1]),
  print

def CLK_SLOP(clk): return ((clk) / 1000)
def CLK_PLUS_SLOP(clk): return ((clk) + CLK_SLOP(clk))
def CLK_MINUS_SLOP(clk): return ((clk) - CLK_SLOP(clk))

def make_cur_ctr(rate, depth, pixel_rep=0):
  assert pixel_rep == 0, "Untested with non-zero pixel rep and probably wrong"

  tmds = (rate * depth) / 8. * (pixel_rep + 1)

  adjust_for_jittery_pll = True

  # If the PIXEL clock (not the TMDS rate) is using the special 594 PLL
  # and is slow enough, we can use normal rates...
  if ((CLK_MINUS_SLOP(74250000) <= rate <= CLK_PLUS_SLOP(74250000)) or
      (CLK_MINUS_SLOP(148500000) <= rate <= CLK_PLUS_SLOP(148500000))):
    adjust_for_jittery_pll = False

  # If rate is slow enough then our jitter isn't a huge issue.
  # ...allowable clock jitter is 362.3 or higher and we're OK there w/ plenty of
  # margin as long as we're careful about our PLL settings.
  if rate <= 79000000:
    adjust_for_jittery_pll = False

  if not adjust_for_jittery_pll:
    # This is as documented
    if tmds <= RATE1:   # 46000000
      return 0x18
    elif tmds <= RATE2: # 92000000
      return 0x28

    # I have no idea why the below is true, but it is the simplest rule I could
    # come up with that matched the tables...
    if depth == 8:
      if tmds <= 340000000:
	# HDMI 1.4
	return 0x38
      # HDMI 2.0
      return 0x18
    elif depth == 16:
      if tmds < 576000000:
	return 0x38
      return 0x28
    else:
      return 0x38

  # The output of rk3288 PLL is the source of the HDMI's MPLL.  Apparently
  # the rk3288 PLL is too jittery.  We can lower the PLL bandwidth of MPLL
  # to compensate.
  #
  # Where possible, we try to use the MPLL bandwidth suggested by Synopsis
  # and we just use lower bandwidth when testing has shown that it's needed.
  # We try to stick to 0x28 and 0x18 since those numbers are present in
  # Synopsis tables.  We go down to 0x08 if needed and finally to 0x00.

  if rate <= 79000000:
    # Supposed to be 0x28 here, but we'll do 0x18 to reduce jitter
    return 0x18
  elif rate <= 118000000:
    # Supposed to be 0x28/0x38 here, but we'll do 0x08 to reduce jitter
    return 0x08
  # Any higher clock rates go to bandwidth = 0
  return 0

def do_curr_ctrl_loop():
  cur_ctrl_table = {}
  last_cur_ctrl = None
  last_rate = None

  for rate in xrange(13500000, 600001000, 1000):
    for8bpp = make_cur_ctr(rate, 8)
    for10bpp = make_cur_ctr(rate, 10)
    for12bpp = make_cur_ctr(rate, 12)

    cur_ctrl = (for8bpp, for10bpp, for12bpp)
    if (cur_ctrl != last_cur_ctrl) and (last_rate is not None):
      cur_ctrl_table[last_rate] = last_cur_ctrl

    last_rate = rate
    last_cur_ctrl = cur_ctrl

  cur_ctrl_table[last_rate] = last_cur_ctrl

  print "\t",
  for rate in sorted(cur_ctrl_table.keys()):
    print ("{\n"
	   "\t\t%d, { %#06x, %#06x, %#06x },\n"
	   "\t}, ") % (
	   rate,
	   cur_ctrl_table[rate][0],
	   cur_ctrl_table[rate][1],
	   cur_ctrl_table[rate][2]),
  print



# From HDMI spec
VPH_RXTERM = 3.3
RXTERM = 50

def get_phy_preemphasis(symon, traon, trbon):
  if (symon, traon, trbon) == (0, 0, 0):
    assert False, "Not valid?"
  elif (symon, traon, trbon) == (1, 0, 0):
    preemph = 0.00
  elif (symon, traon, trbon) == (1, 0, 1):
    # Numbers match examples better if I assume .25 / 3 rather than .08
    preemph = 0.25 / 3
  elif (symon, traon, trbon) == (1, 1, 0):
    # Numbers match examples better if I assume .50 / 3 rather than .17
    preemph = 0.50 / 3
  elif (symon, traon, trbon) == (1, 1, 1):
    preemph = 0.25
  else:
    assert False, "Not valid"

  return preemph

def phy_lvl_to_voltages(lvl, preemph, rterm):
    v_lo = VPH_RXTERM - (.772 - 0.01405 * lvl)
    v_swing = ((VPH_RXTERM - v_lo) * (1 - preemph) /
	      (1 + (RXTERM * (1 + preemph)) / (2 * rterm)))
    v_hi = v_lo + v_swing

    return v_lo, v_swing, v_hi

def print_phy_config(symbol, term, vlev):
  ck_symon = bool(symbol & (1 << 0))
  tx_trbon = bool(symbol & (1 << 1))
  tx_traon = bool(symbol & (1 << 2))
  tx_symon = bool(symbol & (1 << 3))

  slopeboost = {
    0: "no slope boost",
    1: " 5-10% decrease on TMDS rise/fall times",
    2: "10-20% decrease on TMDS rise/fall times",
    3: "20-35% decrease on TMDS rise/fall times",
  }[(symbol >> 4) & 0x3]

  override = bool(symbol & (1 << 15))

  rterm = (50, 57.14, 66.67, 80, 100, 133, 200)[term]

  sup_ck_lvl = (vlev >> 0) & 0x1f
  sup_tx_lvl = (vlev >> 5) & 0x1f

  preemph = get_phy_preemphasis(tx_symon, tx_traon, tx_trbon)

  print "symbol=%#06x, term=%#06x, vlev=%#06x" % (symbol, term, vlev)
  for name, lvl in [("ck", sup_ck_lvl), ("tx", sup_tx_lvl)]:
    v_lo, v_swing, v_hi = phy_lvl_to_voltages(lvl, preemph, rterm)
    print " %s: lvl = %2d, term=%3d, vlo = %.2f, vhi=%.2f, vswing = %.2f, %s" % (
      name, lvl, rterm, v_lo, v_hi, v_swing, slopeboost)

#def calc_ideal_phy_lvl(swing_mv, preemph, rterm):
  #"""Get the ideal "lvl" for the given swing, preemph, and termination.

  #This might not be integral, but and might not fit the 0-31 range.
  #"""
  #v_lo = (VPH_RXTERM -
	  #v_swing / (1 - preemph) -
	  #(v_swing * RXTERM * (1 + preemph)) / (2 * rterm * (1 - preemph)))
  #lvl = (.772 - (VPH_RXTERM - v_lo)) / 0.01405

  #return lvl

def do_phy_config_list(rate=16500000):
  # From HDMI spec
  VPH_RXTERM = 3.3
  RXTERM = 50

  # Set to True to print even things that don't meet requirements.
  print_invalid = False

  # Totally a guess based on what's in IMX6DQRM
  if rate <= 165000000:
    symon = 1 # tx_symon
    traon = 0 # tx_traon
    trbon = 0 # tx_trbon
  else:
    symon = 1 # tx_symon
    traon = 0 # tx_traon
    trbon = 1 # tx_trbon

  print "Guessing symon, traon, trbon based on rate: (%d, %d, %d)" % (
    symon, traon, trbon)

  preemph = get_phy_preemphasis(symon, traon, trbon)

  # Notes:
  # - swing needs to be between .4 and .6
  # - If <= 165MHz, vhi is (VPH_RXTERM + .01) thru (VPH_RXTERM - .01)
  # - If >  165MHz, vhi is (VPH_RXTERM + .01) thru (VPH_RXTERM - .2)
  # - If <= 165MHz, vlo is (VPH_RXTERM - .4)  thru (VPH_RXTERM - .6)
  # - If >  165MHz, vlo is (VPH_RXTERM - .4)  thru (VPH_RXTERM - .7)
  #
  # TODO: I'm not sure we can actually reach vhi of 3.3 +/- .01
  # TODO: How do we pick amongst all of these?

  if rate <= 165000000:
    v_hi_min = 3.19  # Should be (VPH_RXTERM - .01), but not possible?
    v_hi_max = (VPH_RXTERM + .01)
    v_lo_min = (VPH_RXTERM - .6)
    v_lo_max = (VPH_RXTERM - .4)
  else:
    v_hi_min = (VPH_RXTERM - .2)
    v_hi_max = (VPH_RXTERM + .01)
    v_lo_min = (VPH_RXTERM - .7)
    v_lo_max = (VPH_RXTERM - .4)

  for lvl in xrange(0, 31):
    for rterm in (50, 57.14, 66.67, 80, 100, 133, 200):
      v_lo, v_swing, v_hi = phy_lvl_to_voltages(lvl, preemph, rterm)

      if (print_invalid or
	  ((.4 <= v_swing <= .6) and
	   (v_hi_min <= v_hi <= v_hi_max) and
	   (v_lo_min <= v_lo <= v_lo_max))):
	print "lvl = %2d, term=%3d, vlo = %.2f, vhi=%.2f, vswing = %.2f" % (
	  lvl, rterm, v_lo, v_hi, v_swing)

# Examples:
#
# $ ./rk3288_hdmitables.py mpll
# $ ./rk3288_hdmitables.py curr
# $ ./rk3288_hdmitables.py phy_list 165000000
# $ ./rk3288_hdmitables.py phy_list 600000000
# $ ./rk3288_hdmitables.py phy_print 0x8009, 0x0005, 0x01ad

def main(todo, *args):
  if todo == "mpll":
    do_mpll_loop()
  elif todo == "curr":
    do_curr_ctrl_loop()
  elif todo == "phy_list":
    (rate,) = args
    rate = int(rate, 0)
    do_phy_config_list(rate)
  elif todo == "phy_print":
    (symbol, term, vlev) = args
    symbol = int(symbol.rstrip(","), 0)
    term = int(term.rstrip(","), 0)
    vlev = int(vlev.rstrip(","), 0)

    print_phy_config(symbol, term, vlev)

  # These ought to match the tables in the docs.  They are close, but not
  # perfect.  ...but my math is definitely right since v_lo doesn't match
  # and v_lo should be very simple.  Even if we try to find more significant
  # digits for 0.772 and 0.01405 we still can't make it match, so I'm assuming
  # that they rounded somewhere in their math...

  #print "%.3f, %.3f, %.3f" % phy_lvl_to_voltages(19, get_phy_preemphasis(1, 0, 0), 100)
  #print "%.3f, %.3f, %.3f" % phy_lvl_to_voltages(10, get_phy_preemphasis(1, 0, 0), 100)
  #print "%.3f, %.3f, %.3f" % phy_lvl_to_voltages( 6, get_phy_preemphasis(1, 0, 1), 100)

  #print "%.3f, %.3f, %.3f" % phy_lvl_to_voltages(21, get_phy_preemphasis(1, 0, 0), 133)
  #print "%.3f, %.3f, %.3f" % phy_lvl_to_voltages(13, get_phy_preemphasis(1, 0, 0), 133)
  #print "%.3f, %.3f, %.3f" % phy_lvl_to_voltages( 8, get_phy_preemphasis(1, 0, 1), 133)


if __name__ == '__main__':
  main(*sys.argv[1:])
