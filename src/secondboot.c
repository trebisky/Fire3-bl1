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


#define __SET_GLOBAL_VARIABLES
#include "sysheader.h"
#include "nx_bootheader.h"

//#define SIMPLE_MEMTEST 			(1)

#define EMA_VALUE (1) // Manual setting - 1: 1.1V, 3: 1.0V, 4: 0.95V

extern void DMC_Delay(int milisecond);

// extern void     flushICache(void);
// extern void     enableICache(CBOOL enable);

extern void enterSelfRefresh(void);
extern void exitSelfRefresh(void);
extern void set_bus_config(void);
extern void set_drex_qos(void);

extern CBOOL iUSBBOOT(struct NX_SecondBootInfo *const pTBI);
extern CBOOL iUARTBOOT(struct NX_SecondBootInfo *const pTBI);
extern CBOOL iSPIBOOT(struct NX_SecondBootInfo *const pTBI);
extern CBOOL iSDXCBOOT(struct NX_SecondBootInfo *const pTBI);
extern CBOOL iNANDBOOTEC(struct NX_SecondBootInfo *const pTBI);
extern CBOOL iSDXCFSBOOT(struct NX_SecondBootInfo *const pTBI);
extern void initClock(void);
#ifdef MEM_TYPE_DDR3
extern CBOOL init_DDR3(U32);
#endif
#ifdef MEM_TYPE_LPDDR23
extern CBOOL init_LPDDR3(U32);
#endif
extern CBOOL buildinfo(void);

extern void printClkInfo(void);

extern void ResetCon(U32 devicenum, CBOOL en);
extern CBOOL SubCPUBringUp(U32 CPUID);

extern void initPMIC(void);
extern void dowakeup(void);
extern void RomUSBBoot(U32 RomJumpAddr);
extern void SetSecureState(void);
extern int memtester_main(unsigned int start, unsigned int end);

extern int CRC_Check(void* buf, unsigned int size, unsigned int ref_crc);

void simple_memtest(U32 *pStart, U32 *pEnd);

#if defined(RAPTOR)
/*
 * Raptor board - revision check. (H/W: GPIOE 4,5,6)
 * GPIOE4 (Least Bit), GPIOE6(Most Bit)
 */
unsigned int raptor_check_hw_revision(void)
{
	volatile unsigned int *reg = 0;
	unsigned int val = 0;

	/*
	 * Check to GPIOE PORT
	 * Read to GPIOPAD Status Register
	 */
	reg = (volatile unsigned int*)(0xC001E000 + 0x18);
	val = (*reg & (0x7 << 4)) >> 4;

        return val;
}
#endif

/*
 * PWM Module Reset
 * This function is temporary code.
 */

void pwm_reset(void)
{
	int reset_number = RESETINDEX_OF_PWM_MODULE_PRESETn;

	ResetCon(reset_number, CTRUE);	// reset on
	ResetCon(reset_number, CFALSE); // reset negate
}

/*
 * TIMER Module Reset
 * This function is temporary code.
 */

void timer_reset(void)
{
	int reset_number = RESETINDEX_OF_TIMER_MODULE_PRESETn;

	ResetCon(reset_number, CTRUE);	// reset on
	ResetCon(reset_number, CFALSE); // reset negate
}

void device_set_env(void)
{
	/* (Device Port Number) for U-BOOT  */
	unsigned int dev_portnum = pSBI->DBI.SDMMCBI.PortNumber;
	WriteIO32(&pReg_ClkPwr->SCRATCH[1], dev_portnum );
}

//------------------------------------------------------------------------------
#if (CCI400_COHERENCY_ENABLE == 1)
void initCCI400(void)
{
	// before set barrier instruction.
	SetIO32(&pReg_CCI400->SCR, 1 << 0); // static bus disable speculative fetches
	SetIO32(&pReg_CCI400->SCR, 1 << 1); // SFR bus disable speculative fetches

	WriteIO32(&pReg_CCI400->COR, (1UL << 3)); // protect to send barrier command to drex

	WriteIO32(&pReg_CCI400->CSI[BUSID_CS].SCR, 0); // snoop request disable
	WriteIO32(&pReg_CCI400->CSI[BUSID_CODA].SCR,
		  0);					// snoop request disable
	WriteIO32(&pReg_CCI400->CSI[BUSID_TOP].SCR, 0); // snoop request disable

#if (MULTICORE_BRING_UP == 1)
	WriteIO32(&pReg_CCI400->CSI[BUSID_CPUG0].SCR,
		  0x3); // cpu 0~3 Snoop & DVM Req
	while (ReadIO32(&pReg_CCI400->STSR) & 0x1);

	WriteIO32(&pReg_CCI400->CSI[BUSID_CPUG1].SCR, 0x3); // cpu 4~7 Snoop & DVM Req
	while (ReadIO32(&pReg_CCI400->STSR) & 0x1);
#else
	WriteIO32(&pReg_CCI400->CSI[BUSID_CPUG0].SCR, 0x0);
	WriteIO32(&pReg_CCI400->CSI[BUSID_CPUG1].SCR, 0x0);
#endif
}
#endif // #if (CCI400_COHERENCY_ENABLE == 1)

struct NX_CLKPWR_RegisterSet *const clkpwr;

//------------------------------------------------------------------------------
#if (MULTICORE_BRING_UP == 1)
void BootMain ( U32 CPUID )
#else
void BootMain ( void )
#endif
{
	struct NX_SecondBootInfo TBI;
	struct NX_SecondBootInfo *pTBI = &TBI; // third boot info

	CBOOL Result = CFALSE;
	register volatile U32 temp;
	U32 signature, isResume = 0;
	U32 debugCH = 0;

#ifdef RAPTOR_PMIC_INIT
	debugCH = 3;
#endif

	//--------------------------------------------------------------------------
	// Set EMA
	//--------------------------------------------------------------------------

	// Set EMA for CPU Cluster0
	temp = ReadIO32(&pReg_Tieoff->TIEOFFREG[94]) &
	       ~((0x7 << 23) | (0x7 << 17));
	temp |= ((EMA_VALUE << 23) | (EMA_VALUE << 17));
	WriteIO32(&pReg_Tieoff->TIEOFFREG[94], temp);

	// Set EMA for CPU Cluster1
	temp = ReadIO32(&pReg_Tieoff->TIEOFFREG[111]) &
	       ~((0x7 << 23) | (0x7 << 17));
	temp |= ((EMA_VALUE << 23) | (EMA_VALUE << 17));
	WriteIO32(&pReg_Tieoff->TIEOFFREG[111], temp);

	//--------------------------------------------------------------------------
	// Set Affinity ID
	//--------------------------------------------------------------------------
#if (CONFIG_RESET_AFFINITY_ID == 1)
	// Set Affinity level1 for CPU Cluster1
	temp = ReadIO32(&pReg_Tieoff->TIEOFFREG[95]) & 0x00FFFFFF;
	temp |= (1 << 24);
	WriteIO32(&pReg_Tieoff->TIEOFFREG[95], temp);

	// Set Affinity level2 for CPU Cluster1
	temp = ReadIO32(&pReg_Tieoff->TIEOFFREG[96]) & 0xF0;
	//    temp |= (1 << 0);
	WriteIO32(&pReg_Tieoff->TIEOFFREG[96], temp);
#endif

	//--------------------------------------------------------------------------
	// Init debug
	//--------------------------------------------------------------------------
	DebugInit(debugCH);

	WriteIO32(&pReg_Alive->ALIVEPWRGATEREG, 1);
	WriteIO32(&pReg_Alive->VDDCTRLSETREG, 0x000003FC); //; Retention off (Pad hold off)

	if (USBREBOOT_SIGNATURE == ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE5))
		RomUSBBoot((U32)0x0000009C);
#if !defined(LOAD_FROM_USB)
		SetIO32(&pReg_RstCon->REGRST[RESETINDEX_OF_WDT_MODULE_PRESETn >> 5], 1 << (RESETINDEX_OF_WDT_MODULE_PRESETn & 0x1F));
		SetIO32(&pReg_RstCon->REGRST[RESETINDEX_OF_WDT_MODULE_nPOR >> 5], 1 << (RESETINDEX_OF_WDT_MODULE_nPOR & 0x1F));
		WriteIO32(&pReg_WDT->WTCON,
				0xFF << 8 |		// prescaler value
				0x03 << 3 |		// division factor (3:128)
				0x01 << 2);		// watchdog reset enable
		WriteIO32(&pReg_WDT->WTCNT, 0xFFFF);	// 200MHz/256/128 = 6103.515625, 65536/6103.5 = 10.74 sec
//		SetIO32  ( &pReg_WDT->WTCON, 0x01<<5);          // watchdog timer enable
#endif
	//--------------------------------------------------------------------------
	// Get resume information.
	//--------------------------------------------------------------------------
	signature = ReadIO32(&pReg_Alive->ALIVESCRATCHREADREG);
	if ((SUSPEND_SIGNATURE == (signature & 0xFFFFFF00)) && ReadIO32(&pReg_Alive->WAKEUPSTATUS)) {
		isResume = 1;
	}

	/* Arm Trusted Firmware */
	signature = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE4);
	if ((ATF_SUSPEND_SIGNATURE == (signature & 0xFFFFFF00)) && ReadIO32(&pReg_Alive->WAKEUPSTATUS)) {
		isResume = 1;
	}

	/*
	 * SD/MMC,SPI - port number stored for u-boot.
	 */
	device_set_env();

//--------------------------------------------------------------------------
// Initialize PMIC device.
//--------------------------------------------------------------------------
#if defined(INITPMIC_YES)
	initPMIC();
#endif

	SYSMSG("EMA is %s\r\n", (EMA_VALUE == 1) ? "1.1V" : (EMA_VALUE == 3) ? "1.0V" : "0.95V");
	SYSMSG("\r\n\nWorking to aarch%d\r\nwaiting for pll change..\r\n", sizeof(void *) * 8);

	while (!DebugIsUartTxDone());

	//--------------------------------------------------------------------------
	// Change to PLL.
	//--------------------------------------------------------------------------
	initClock();

	//--------------------------------------------------------------------------
	// Debug Console
	//--------------------------------------------------------------------------
	DebugInit(debugCH);

	//--------------------------------------------------------------------------
	// build information. version, build time and date
	//--------------------------------------------------------------------------
#if 1
	buildinfo();
#else
	if (buildinfo() == CFALSE) {
		printf("WARNING : NSIH mismatch...!!!\r\n");
		while (1);
	}
#endif

	//--------------------------------------------------------------------------
	// print clock information
	//--------------------------------------------------------------------------
	printClkInfo();

	SYSMSG("\r\nDDR3 POR Init Start %d\r\n", isResume);
#if 0
	if( (pSBI->DII.WRITEDELAY & 0xff) == 0 ) {
		void udelay(U32);
		SetIO32(&pReg_GPIO[GPIO_GROUP_E]->GPIOx_PULLENB, 1<<12);
		SetIO32(&pReg_GPIO[GPIO_GROUP_E]->GPIOx_PULLSEL_DISABLE_DEFAULT, 1<<12);
		SetIO32(&pReg_GPIO[GPIO_GROUP_E]->GPIOx_PULLENB_DISABLE_DEFAULT, 1<<12);
		udelay(0x4000);
		if( (ReadIO32(&pReg_GPIO[GPIO_GROUP_E]->GPIOxPAD) &  1<<12) == 0 ) {
			pSBI->DII.WRITEDELAY |= 1;
		}else{
			pSBI->DII.WRITEDELAY |= 2;
		}
	}
#endif
#ifdef MEM_TYPE_DDR3
#if 0
	if (init_DDR3(isResume) == CFALSE)
		init_DDR3(isResume);
#else
	/*
	 * DDR initialization fails, a temporary code
	 * code for the defense.
	 */
	int ddr_retry = 0;
	while (init_DDR3(0) == CFALSE) {
		ddr_retry++;
		if (ddr_retry > 3) {
			printf("Memory Initialize Retry : %d \r\n", ddr_retry);
			printf("Memory Initializa or Calibration Failed! \r\n");
			break;
		}
	}
#endif
#endif
#ifdef MEM_TYPE_LPDDR23
	if (init_LPDDR3(0) == CFALSE)
		init_LPDDR3(0);
#endif

	if (isResume)
		exitSelfRefresh();

	SYSMSG("DDR3 Init Done!\r\n");

	set_bus_config();
	set_drex_qos();
	/* Temporary Code - PWM Reset */
	pwm_reset();
	/* Temporary Code - Timer Reset */
	timer_reset();

#if (CCI400_COHERENCY_ENABLE == 1)
	SYSMSG("CCI Init!\r\n");
	initCCI400();
#endif

#ifdef aarch64
	SetSecureState();
#endif

	SYSMSG("Wakeup CPU ");

#if (MULTICORE_BRING_UP == 1)
	SubCPUBringUp(CPUID);
#endif

	if (isResume) {
		SYSMSG(" DDR3 SelfRefresh exit Done!\r\n0x%08X\r\n", 
			ReadIO32(&pReg_Alive->WAKEUPSTATUS));
		dowakeup();
	}
	WriteIO32(&pReg_Alive->ALIVEPWRGATEREG, 0);

	if (pSBI->SIGNATURE != HEADER_ID)
		printf("2nd Boot Header is invalid, Please check it out!\r\n");

#ifdef SIMPLE_MEMTEST
	simple_memtest((U32 *)0x40000000UL, (U32 *)0xBFFF0000);
#endif

	switch (pSBI->DBI.SPIBI.LoadDevice) {
#if defined(SUPPORT_USB_BOOT)
	case BOOT_FROM_USB:
		SYSMSG("Loading from usb...\r\n");
		Result = iUSBBOOT(pTBI); // for USB boot
		break;
#endif

#if defined(SUPPORT_SPI_BOOT)
	case BOOT_FROM_SPI:
		SYSMSG("Loading from spi...\r\n");
		Result = iSPIBOOT(pTBI); // for SPI boot
		break;
#endif

#if defined(SUPPORT_NAND_BOOT)
	case BOOT_FROM_NAND:
		SYSMSG( "Loading from nand...\r\n" );
		Result = iNANDBOOTEC(pTBI);     // for NAND boot
		break;
#endif

#if defined(SUPPORT_SDMMC_BOOT)
	case BOOT_FROM_SDMMC:
		SYSMSG("Loading from sdmmc...\r\n");
		Result = iSDXCBOOT(pTBI); // for SD boot
		break;
#endif

#if defined(SUPPORT_SDFS_BOOT)
	case BOOT_FROM_SDFS:
		SYSMSG("Loading from sd FATFS...\r\n");
		Result = iSDXCFSBOOT(pTBI); // for SDFS boot
		break;
#endif

#if defined(SUPPORT_UART_BOOT)
	case BOOT_FROM_UART:
		SYSMSG("Loading from uart...\r\n");
		Result = iUARTBOOT(pTBI);       // for UART boot
		break;
#endif
	}

#ifdef CRC_CHECK_ON
	//Result = CRC_Check((void*)pTBI->LOADADDR, (unsigned int)pTBI->LOADSIZE,
	//		   (unsigned int)pTBI->DBI.SDMMCBI.CRC32);
#endif
	if (Result) {
		void SwitchToEL2(void);

#ifdef notdef
		/* tjt */
		void my_test ( int * );
		void my_dump ( unsigned long *, int );
		void my_print ( char * );
		static long lbuf[7];
		int *ip;
		void (*xLaunch)();
		unsigned long *xptr;
#endif

		/* This is the location of the header from the file we are booting. */
		struct nx_tbbinfo *tbi = (struct nx_tbbinfo *)&TBI;
		void (*pLaunch)() = (void (*)())((long)tbi->startaddr);
		// xLaunch = pLaunch;
		// xptr = (unsigned long *) pLaunch;

		SYSMSG(" Image Loading Done!\n");

		printf ( "bl1, image loaded, launching to 0x%08x\n", (MPTRS)pLaunch);

#ifdef aarch64
		SwitchToEL2();
#endif

		SYSMSG("Launch to 0x%08X\n", (MPTRS)pLaunch);

		/* Wait for output to clear uart */
		temp = 0x10000000;
		while (!DebugIsUartTxDone() && temp--)
		    ;

#ifdef notdef
		printf ( "internal buffer for launch at: %08x\n", &lbuf[0] );
		my_print ( "Launch 1\n" );

		lbuf[0] = 0xd65f03c0;
		my_print ( "Launch 1b\n" );
		pLaunch = (void (*)()) &lbuf[0];
		my_print ( "Launch 1c\n" );
		pLaunch ();

		my_print ( "Launch 2\n" );

		ip = (int *) 0x50000000;
		*ip = 0xd65f03c0;
		pLaunch = (void (*)()) ip;
		pLaunch ();

		my_dump ( xptr, 4 );

		my_print ( "Launch for real ...\n" );
#endif

		// Heaven knows what these arguments are for */
		// xLaunch(0, 4330);
		// pLaunch(0, 4330);
		pLaunch ();

		printf ( "Back in the bootloader (bl1)\n" );
#ifdef notdef
		my_print ( "Launch DONE\n" );
		my_test ( (int *) pLaunch );
		for ( ;; ) ;
#endif
	}

	printf(" Image Loading Failure Try to USB boot\n");

	temp = 0x10000000;
	while (!DebugIsUartTxDone() && temp--)
	    ;

	RomUSBBoot((U32)0x0000009C);
	while (1)
	    ;
}
/* End of BootMain */

#ifdef notdef
void
my_print ( char *msg )
{
	volatile int tmo;

	printf ( msg );
	tmo = 0x10000000;
	while (!DebugIsUartTxDone() && tmo--)
	    ;

}

void
my_dump ( unsigned long *lp, int count )
{
	int i;

	for ( i=0; i< count; i++ ) {
	    printf ( "%08x = %08x\n\r", lp, *lp );
	    lp++;
	}
}

static void mdelay ( int n )
{
	volatile int i;
	volatile int k;

	for ( i=0; i<n; i++ )
	    for ( k=0; k<2400; k++ )
		;
}

/* TJT */
void
my_test ( int *p )
{
	// unsigned long *p = (unsigned long *) 0x40000000;
	unsigned long val;
	int t;

	printf ( "My test at address: %08x\n", p );

	val = *p;
	printf ( "My test reads: %08x\n", val );
	*p = 0;
	val = *p;
	printf ( "My test reads: %08x\n", val );
	*p = 0xffffffff;
	val = *p;
	printf ( "My test reads: %08x\n", val );

	for ( t=0;; t++ ) {
	    printf ( "Tick %d\n", t );
	    mdelay ( 1000 );
	}
}
#endif

/* THE END */
