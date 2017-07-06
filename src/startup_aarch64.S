/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sangjong, Han <hans@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "nx_peridot.h"

#include "aarch64_Vectors.inc"

    .align 9, 0    // 2^9 = 512 bytes sector size
    // see reference-nsih/drone-sd-64.txt
    .word 0xE3A00103	// 0x000 : MOV R0, #0xC0000000
    .word 0xE3800A11	// 0x004 : ORR R0, R0, #0x11000
    .word 0xE590113C	// 0x008 : LDR R1, [R0, #0x13C]		// tieoff reg select AArch mode
    .word 0xE3811A0F	// 0x00C : ORR R1, R1, #0xF000		// all group 0 cpu AArch64 mode
    .word 0xE580113C	// 0x010 : STR R1, [R0, #0x13C]
    .word 0xE3A025FF	// 0x014 : MOV R2, #0x3FC00000
    .word 0xE38229FF	// 0x018 : ORR R2, R2, #0x3FC000
    .word 0xE3822080	// 0x01C : ORR R2, R2, #0x80		// 0xFFFF0200>>2
    .word 0xE5802140	// 0x020 : STR R2, [R0, #0x140]		// Set AArch64 Reset Vector
    .word 0xE3A08103	// 0x024 : MOV R8, #0xC0000000
    .word 0xE3888801	// 0x028 : ORR R8, R8, #0x10000
    .word 0xE59892AC	// 0x02C : LDR R9, [R8, #0x2AC]		// system control reset req
    .word 0xE3899001	// 0x030 : ORR R9, R9, #0x1
    .word 0xE58892AC	// 0x034 : STR R9, [R8, #0x2AC]
    .word 0xE320F003	// 0x038 : WFI				// core 0 will be reset when wfi
    .word 0xEAFFFFFE	// 0x03C : B .
    .word 0x00008000	// 0x040 : Device Read Address from 2ndboot Device.
    .word 0x00040000	// 0x044 : Load Size for 2ndboot.
    .word 0xFFFF0000	// 0x048 : Load Address for 2ndboot.
    .word 0xFFFF0000	// 0x04C : Launch Address for 2ndboot.
    .word 0x00000000	// 0x050	[7:0] - Channel number
    .word 0x03000000	// 0x054	Boot From SDMMC	[31:24] - 0: USB, 1: SPI, 2: NAND, 3:SDMMC, 4: SDFS
    .word 0x00000000	// 0x058
    .word 0x100CC801	// 0x05C : PLL0		800MHz	P:3	M:200	S:1
    .word 0x100CC801	// 0x060 : PLL1		800MHz	P:3	M:200	S:1
    .word 0x50086601	// 0x064 : PLL2		614.4MHz	P:2	M:102	S:1	K:26214
    .word 0x100CC801	// 0x068 : PLL3		800MHz	P:3	M:200	S:1	K:0
    .word 0x66660104	// 0x06C : PLL2 SPREAD
    .word 0x00000104	// 0x070 : PLL3 SPREAD
    .word 0x00000601	// 0x074 : CPU G0	PLL1	/FCLK:800	/HCLK:200
    .word 0x00000208	// 0x078 : BUS		PLL0	/BCLK:400	/PCLK:200
    .word 0x00208003	// 0x07C : MEM		PLL3	/MDCLK:800	/MCLK:800	/MBCLK:400	/MPCLK:200
    .word 0x00000208	// 0x080 : GR3D		PLL0	/GR3DBCLK:400
    .word 0x00000208	// 0x084 : MPEG		PLL0	/MPEGBCLK:400	/MPEGPCLK:200
    .word 0x00000208	// 0x088 : DISP		PLL0	/DISPBCLK:400	/DISPPCLK:200
    .word 0x00000038	// 0x08C : HDMI		PLL0	/HDMIPCLK:100
    .word 0x00000601	// 0x090 : CPU G1	PLL1	/FCLK:800	/HCLK:200
    .word 0x00000208	// 0x094 : CCI4		PLL0	/CCI4BCLK:400	/CCI4PCLK:200
    .word 0x03100301	// 0x098	/Chip Num:1	/Chip Row:15	/Bus Width:16	/Chip Col:10
    .word 0x004007C0	// 0x09C	512MB x 2ea (16bit) x 1CS
    .word 0x06000B08	// 0x0A0	/CWL:8	/CL:11	/MR1_AL:0	/MR0_WR:6
    .word 0x0C0C0C0C	// 0x0A4
    .word 0x04040404	// 0x0A8
    .word 0x0000401B	// 0x0AC	/tPZQ:16411
    .word 0x00620618	// 0x0B0	/Refresh Interval:7.8us
    .word 0x6836650E	// 0x0B4	/tRFC:104	/tRRD:3	/tRP:6	/tRCD:6	/tRC:20	/tRAS:14
    .word 0x3630580B	// 0x0B8	/tWTR:3	/tWR:6	/tRTP:3
    .word 0x41000A26	// 0x0BC	/tFAW:16	/tXSR:256	/tXP:10	/tCKE:2
    .word 0x00020102	// 0x0C0	[23:16] MR1_RTT_Nom - 001: RZQ/4, 010: RZQ/2, 011: RZQ/6, 100: RZQ/12, 101: RZQ/8, [15:0] MR1_ODS - 0: RZQ/6, 1 : RZQ/7, [7:0] MR2_RTT_WR - 0: ODT disable, 1: RZQ/4, 2: RZQ/2
    .word 0x06060606	// 0x0C4	[31:24] Byte3, [23:16] Byte2, [15:8] Byte1, [7:0] Byte0 - 240ohm / (n + 1), n = (1 ~ 7)
    .word 0x06060606	// 0x0C8	[31:24] CA, [23:16] CS, [15:8] CKE, [7:0] CK - 240ohm / (n + 1), n = (1 ~ 7)
    .word 0x00000104	// 0x0CC	[15:8] ZQ_ODT, [7:0] ZQ_DDS - 240ohm / (n + 1), n = (1 ~ 7)
    .word 0x00000004	// 0x0D0	WR_CAL[4], RD_CAL[3], GT_LVL[2], CA_CAL[1], WR_LVL[0]
	.skip 0x124
    .word 0x68180300	// 0x1F8, BuildInfo
    .word 0x4849534E	// 0x1FC	"NSIH"

/*
 * entry point of main function
 */
.global BootMain
.global SubCPUBoot
.global GetCPUID

//;==================================================================
//; Vectors
//;==================================================================
.global Startup
Startup:
        msr     DAIFSet, #(I_Bit|F_Bit|A_Bit)        //; disable interrupt & fast interrupt and Abort
        bl      GetCPUID
        mov     x29, x0
        bl      remap_vectors
        ands    x0, x0, #7
        b.ne    CPUBRINGUP
        b       Reset_Handler
//BuildInfo:
        .word   0x68180306      //; 24, Chip name - 6818, Build num - v0.3.06

Reset_Handler:
        cmp     x29, xzr
        b.ne    clbss_e
//;==================================================================
//; Clear SRAM
//;==================================================================
        //; Clear area of global data.
        ldr     x1, =__bss_start__                  // this is auto-relocated!
        ldr     x2, =__bss_end__                    // this is auto-relocated!

        mov     x3, xzr                             // prepare zero to clear BSS

clbss_l:
        cmp     x1, x2                              // while not at end of BSS
        b.hs    clbss_e                             // higher or same
        str     x3, [x1], #8                        // clear 64-bit BSS word
        b.lo    clbss_l
clbss_e:
.if 1
//=============================================================================
// Set L2ACTLR
//=============================================================================
        // L2CTLR_EL1
        mrs     x0, S3_1_c11_c0_2                   // Read L2 Control Register
        orr     x0, x0, #(1<<21)                    // [21]ECC, parity enable.
        orr     x0, x0, #(1<<20)                    // [22]Data inline ECC enable.
                                                    //     only applies if ECC is enabled
//        and     x0, x0, #~(1<<5)                    // [5]L2 Data RAM input latency (1 cycle)
        orr     x0, x0, #(1<<5)                     // [5]L2 Data RAM input latency (2 cycle)
//        and     x0, x0, #~(1<<5)                    // [0]L2 Data RAM output latency (2 cycle)
        orr     x0, x0, #(1<<5)                     // [0]L2 Data RAM output latency (3 cycle)
        msr     S3_1_c11_c0_2, x0                   // Write L2 Control Register

        // L2ECTLR_EL1 - do not touch yet
        // L2 internal asynchronous error
        // AXI or Skyros asynchronous error
        // L2 dynamic retention control
//        mrs     x0, S3_1_C11_C0_3                   // Read L2 Extented Control Register
//        and     x0, x0, #~(1<<30)                   // clear internal asynchronous error pending irq
//        and     x0, x0, #~(1<<29)                   // clear AXI asynchronous error irq
//        bic     x0, x0, #(7<<0)                     // L2 dynamic retention disabled.
//        msr     S3_1_C11_C0_3, x0                   // Write L2 Extented Control Register

        // L2ACTLR_EL1
        mrs     x0, s3_1_c15_c0_0
        and     x0, x0, #~(1<<14)                   // Disables UniqueClean evictions with data. This is the reset value for ACE.
        and     x0, x0, #~(1<<3)                    // Enable clean/evict to be pushed out to external. This is the reset value for ACE.
        msr     s3_1_c15_c0_0, x0

        // CPUACTLR_EL1
//        mrs     x0, S3_1_c15_c2_0
//        no touch yet
//        msr     S3_1_c15_c2_0, x0
        mrs     x0, ACTLR_EL3                       // Read ACTLR_ELx into Xt
        orr     x0, x0, #(1<<6)                     // L2ACTLR accessible from lower ELs
        orr     x0, x0, #(1<<5)                     // L2ECTLR accessible from lower ELs
        orr     x0, x0, #(1<<4)                     // L2CTLR accessible from lower ELs
        orr     x0, x0, #(1<<1)                     // CPUECTLR accessible from lower ELs
        orr     x0, x0, #(1<<0)                     // CPUACTLR accessible from lower ELs
        msr     ACTLR_EL3, x0                       // Write Xt to ACTLR_ELx
//        dsb     sy
.endif
//;==================================================================
//; Setup stacks
//;==================================================================
CPUBRINGUP:

        mrs     x0, SCR_EL3
        orr     x0, x0, #(3<<4)                     // RES1
        bic     x0, x0, #(1<<0)                     // 0: secure mode
        orr     x0, x0, #(1<<2)                     // 1: route fiq to EL3
        bic     x0, x0, #(1<<7)                     // 0: SMC is enabled at EL1, EL2, or EL3    1: SMC is undefined at all exception level
        bic     x0, x0, #(1<<8)                     // disable HVC. to be NOP
//;        orr     x0, x0, #(1<<12)                    // trap WFI
        orr     x0, x0, #(1<<10)                    // RW: 0: aarch32, 1:aarch64
        msr     SCR_EL3, x0

        msr     CPTR_EL3, xzr                       //; not traped FP, SIMD

        mrs     x0, S3_1_c15_c2_1
        orr     x0, x0, #(1<<6)                     // [6] SMPEN
        msr     S3_1_c15_c2_1, x0
		isb

//        ldr     x0, =200000000
//        msr     CNTFRQ_EL0, x0

        mrs     x0, CPACR_EL1                       //; printf use fpu, neon register. so for test, exception trap must be disabled.
        orr     x0, x0, #(3<<20)                    //; access fpu is not traped EL0, EL1
        msr     CPACR_EL1, x0

        mrs     x0, HCR_EL2
        orr     x0, x0, #(1<<31)                    //; rw 0:lower levels are all aarch32, 1: EL1 is aarch64
        bic     x0, x0, #(1<<27)                    //; TGE    - el1 exception routed to el2
//;        orr     x0, x0, #(1<<13)                 //; wfi traped
        orr     x0, x0, #(1<<4)                     //; IMO
        msr     HCR_EL2, x0

//;        mrs     x0, CPTR_EL2
        mov     x0, #0x33FF                         //; RES1
//;        bic     x0, x0, #(1<<31)                    //; TCPAC
//;        bic     x0, x0, #(1<<20)                    //; TTA
//;        bic     x0, x0, #(1<<10)                    //; TFP
        msr     CPTR_EL2, x0

        cmp     x29, xzr
        b.ne    0f
.if 1

        mrs     x0, SCTLR_EL3
        orr		x0, x0, #(1<<29 | 1<<28)			//; SBO
        orr		x0, x0, #(1<<23 | 1<<22)			//; SBO
        orr		x0, x0, #(1<<11)					//; SBO
        bic     x0, x0, #(1<<12)                    //; icache disable
        msr     SCTLR_EL3, x0
        ic      ialluis                             //; invalidate icache all
        isb     sy
        mrs     x0, SCTLR_EL3
        orr     x0, x0, #(1<<12)
        msr     SCTLR_EL3, x0                       //; icache enable
.endif

        mov     x0, #0x0830                         //; RES1
        movk    x0, #0x30C5, lsl #16                //; RES1
        msr     sctlr_el2, x0                       //; MMU off, I and C bit off, Align bit off, little endian, execute never
0:
        mov     w0, #0xFF000000
        orr     w0, w0, #0x00FF0000
        add     x0, x0, #INTERNAL_SRAM_SIZE

        mov     w1, #0x200                          // AArch64 stack point must be aligned by 16bytes
        sub     w2, w29, #1
        and     w2, w2, #0x7                        // cpu 0: -0x1C0, cpu 1: -0, cpu 2: -0x40,  3: -0x80, 4: -0xC0, 5: -0x100, 6: -0x140, 7: -0x180
        mul     w1, w1, w2
        sub     x0, x0, x1

        mov     sp, x0
		msr		sp_el2, x0

        mov     x0, x29
		bl remap_vectors

        mov     x0, x29

        cmp     x0, xzr
        b.ne    1f
        bl      BootMain                            //; save this in register for possible long jump
        b       .
1:
        bl      SubCPUBoot
        b       .

        .ltorg
//;==================================================================
//; PLL Change
//;==================================================================
        .align 6                                    //; below instruction number is 6, 2^6=64bytes

.global __pllchange
__pllchange:                                        //; r0:data r1:pll address r2:delay count
        mov     w3, #0x1000                         //; for icache prefetch
pllchangedelayloop:                                 //; this code will be already within i-cache. no bus transaction will make
        subs    w3, w3, #1                          //; wait for pll change done
        b.ne    notadapt
        str     w0, [x1]                            //; pll change start
        mov     w3, w2                              //; real delay time set
        cmp     w3, wzr
        b.ne    postloop
notadapt:
        cmp     w3, #0x1000
postloop:
        b.ne    pllchangedelayloop
        ret

        .ltorg

remap_vectors:
        adr     x3, vectors_el3
        msr     VBAR_EL3, x3                        //; reset exception vector
        ret

.Lpanic:
    stp     x27, x28, [sp, -16]!
    stp     x25, x26, [sp, -16]!
    stp     x23, x24, [sp, -16]!
    stp     x21, x22, [sp, -16]!
    stp     x19, x20, [sp, -16]!
    stp     x17, x18, [sp, -16]!
    stp     x15, x16, [sp, -16]!
    stp     x13, x14, [sp, -16]!
    stp     x11, x12, [sp, -16]!
    stp     x9,  x10, [sp, -16]!
    stp     x7,  x8,  [sp, -16]!
    stp     x5,  x6,  [sp, -16]!
    stp     x3,  x4,  [sp, -16]!
    stp     x1,  x2,  [sp, -16]!
    stp     xzr, x0,  [sp, -16]!
    mrs		x1, currentEl
    ubfx	x1, x1, 2, 2
    cmp		x1, 3
    b.eq    3f
    cmp		x1, 2
    b.eq    2f
    mrs		x1, esr_el1
    b       1f
3:  mrs     x1, esr_el3
    b       1f
2:  mrs     x1, esr_el2
1:  mov     x0, x29
    add     x2, sp, 8
    bl      sync_c_handler_EL3
    ldp     xzr, x0,  [sp], 16
    ldp     x1,  x2,  [sp], 16
    ldp     x3,  x4,  [sp], 16
    ldp     x5,  x6,  [sp], 16
    ldp     x7,  x8,  [sp], 16
    ldp     x9,  x10, [sp], 16
    ldp     x11, x12, [sp], 16
    ldp     x13, x14, [sp], 16
    ldp     x15, x16, [sp], 16
    ldp     x17, x18, [sp], 16
    ldp     x19, x20, [sp], 16
    ldp     x21, x22, [sp], 16
    ldp     x23, x24, [sp], 16
    ldp     x25, x26, [sp], 16
    ldp     x27, x28, [sp], 16
    ldp     x29, x30, [sp], 16
    eret

.macro panic
	.align	7
1:  stp     x29, x30, [sp, -16]!
    mov     x29, 1b - vectors_el3
    b .Lpanic
    ldp     x29, x30, [sp], 16
    eret
.endm

	.text
	.align 11
vectors_el3:
    panic /* 0x000 Current EL Synchronous Thread */
    panic /* 0x080 Current EL IRQ Thread */
	panic /* 0x100 Current EL FIQ Thread */
	panic /* 0x180 Current EL Error Thread */
	panic /* 0x200 Current EL Synchronous Handler */
	panic /* 0x280 Current EL IRQ Handler */
	panic /* 0x300 Current EL FIQ Handler */
	panic /* 0x380 Current EL Error Handler */
	panic /* 0x400 */
    panic /* 0x480 */
    panic /* 0x500 */
    panic /* 0x580 */
    panic /* 0x600 */
    panic /* 0x680 */
    panic /* 0x700 */
    panic /* 0x780 */

do_reset:
    ldr w0, =0xc0010224
    mov w1, #0x08
    str w1, [x0]
    mov w1, #0x1000
    str w1, [x0, 4]
1:  b 1b

