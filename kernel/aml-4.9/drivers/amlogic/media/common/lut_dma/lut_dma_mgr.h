/*
 * drivers/amlogic/media/common/lut_dma/lut_dma_mgr.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef LUT_DMA_MGR_H_
#define LUT_DMA_MGR_H_

#define LUT_DMA_WR_CHANNEL     8
#define LUT_DMA_RD_CHANNEL     1
#define LUT_DMA_CHANNEL        (LUT_DMA_WR_CHANNEL + LUT_DMA_RD_CHANNEL)
#define LUT_DMA_RD_CHAN_NUM    8

#define VPU_DMA_RDMIF0_CTRL   0x2750
#define VPU_DMA_RDMIF1_CTRL   0x2751
#define VPU_DMA_RDMIF2_CTRL   0x2752
#define VPU_DMA_RDMIF3_CTRL   0x2753
#define VPU_DMA_RDMIF4_CTRL   0x2754
#define VPU_DMA_RDMIF5_CTRL   0x2755
#define VPU_DMA_RDMIF6_CTRL   0x2756
#define VPU_DMA_RDMIF7_CTRL   0x2757

#define VPU_DMA_RDMIF0_BADR0  0x2758
#define VPU_DMA_RDMIF0_BADR1  0x2759
#define VPU_DMA_RDMIF0_BADR2  0x275a
#define VPU_DMA_RDMIF0_BADR3  0x275b
#define VPU_DMA_RDMIF1_BADR0  0x275c
#define VPU_DMA_RDMIF1_BADR1  0x275d
#define VPU_DMA_RDMIF1_BADR2  0x275e
#define VPU_DMA_RDMIF1_BADR3  0x275f
#define VPU_DMA_RDMIF2_BADR0  0x2760
#define VPU_DMA_RDMIF2_BADR1  0x2761
#define VPU_DMA_RDMIF2_BADR2  0x2762
#define VPU_DMA_RDMIF2_BADR3  0x2763
#define VPU_DMA_RDMIF3_BADR0  0x2764
#define VPU_DMA_RDMIF3_BADR1  0x2765
#define VPU_DMA_RDMIF3_BADR2  0x2766
#define VPU_DMA_RDMIF3_BADR3  0x2767
#define VPU_DMA_RDMIF4_BADR0  0x2768
#define VPU_DMA_RDMIF4_BADR1  0x2769
#define VPU_DMA_RDMIF4_BADR2  0x276a
#define VPU_DMA_RDMIF4_BADR3  0x276b
#define VPU_DMA_RDMIF5_BADR0  0x276c
#define VPU_DMA_RDMIF5_BADR1  0x276d
#define VPU_DMA_RDMIF5_BADR2  0x276e
#define VPU_DMA_RDMIF5_BADR3  0x276f
#define VPU_DMA_RDMIF6_BADR0  0x2770
#define VPU_DMA_RDMIF6_BADR1  0x2771
#define VPU_DMA_RDMIF6_BADR2  0x2772
#define VPU_DMA_RDMIF6_BADR3  0x2773
#define VPU_DMA_RDMIF7_BADR0  0x2774
#define VPU_DMA_RDMIF7_BADR1  0x2775
#define VPU_DMA_RDMIF7_BADR2  0x2776
#define VPU_DMA_RDMIF7_BADR3  0x2777

#define VPU_DMA_WRMIF_CTRL1   0x27d1
#define VPU_DMA_WRMIF_CTRL2   0x27d2
#define VPU_DMA_WRMIF_CTRL3   0x27d3
#define VPU_DMA_WRMIF_BADDR0  0x27d4
#define VPU_DMA_WRMIF_CTRL    0x27dc
#define VPU_DMA_WRMIF_BADDR1  0x27dd
#define VPU_DMA_WRMIF_BADDR2  0x27de
#define VPU_DMA_WRMIF_BADDR3  0x27df

#define DMA_BUF_NUM   4

struct lut_dma_ins {
	struct mutex lut_dma_lock;/*lut dma mutex*/
	unsigned char registered;
	unsigned char enable;
	unsigned char dir;
	unsigned char mode;
	unsigned char baddr_set;
	u32 trigger_irq_type;
	u32 rd_table_size[DMA_BUF_NUM];
	u32 rd_phy_addr[DMA_BUF_NUM];
	u32 *rd_table_addr[DMA_BUF_NUM];
	u32 wr_table_size[DMA_BUF_NUM];
	u32 wr_phy_addr[DMA_BUF_NUM];
	u32 *wr_table_addr[DMA_BUF_NUM];
	u32 wr_size[DMA_BUF_NUM];
};

struct lut_dma_device_info {
	const char *device_name;
	struct platform_device *pdev;
	struct class *clsp;
	struct lut_dma_ins ins[LUT_DMA_CHANNEL];
};

#endif
