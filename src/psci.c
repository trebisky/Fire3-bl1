#include "printf.h"
#include <nx_alive.h>
#include <nx_clkpwr.h>


void boardReset(void)
{
    printf("\r\nBoard reset.\r\n");
    WriteIO32(0xc0010224, 0x8);
    WriteIO32(0xc0010228, 0x1000);
    while( 1 );
}


enum PsciErrorCodes {
    PSCI_SUCCESS            =  0,
    PSCI_NOT_SUPPORTED      = -1,
    PSCI_INVALID_PARAMETERS = -2,
    PSCI_DENIED             = -3,
    PSCI_ALREADY_ON         = -4,
    PSCI_ON_PENDING         = -5,
    PSCI_INTERNAL_FAILURE   = -6,
    PSCI_NOT_PRESENT        = -7,
    PSCI_DISABLED           = -8,
    PSCI_INVALID_ADDRESS    = -9
};

static unsigned psciFeatures(unsigned function)
{
    unsigned result;

    switch( function ) {
    case 0x84000001:    // CPU_SUSPEND
    case 0xC4000001:
        result = 0x3;   // extended StateID format && OS-initiated mode support
        break;
    default:
        result = 0;
        break;
    }
    return result;
}

static unsigned bringCpuOn(unsigned long targetCpu, unsigned long entryPoint,
        unsigned long contextId)
{
    unsigned cpuId = (targetCpu & 3) | (targetCpu & 0xff00 ? 0x4 : 0);
    void SubCPUJumpTo(U32 cpuId, unsigned jumpAddr);

	(void)contextId;
    SubCPUJumpTo(cpuId, entryPoint);
    return PSCI_SUCCESS;
}

static void systemReset()
{
	/* writel((-1UL), 0xC00108DC); ALIVESCRATCHRSTREG5 - Android recovery/fastboot */
    WriteIO32(0xc0010800, 0);      // ALIVEPWRGATEREG, close alive gate
    SetIO32(0xc0010224, 1<<3);     // PWRCONT, Software reset enable
    SetIO32(0xc0010228, 1<<12);    // PWRMODE, Software reset
	while(1);
}

void psciHandler(unsigned long *regs)
{
    unsigned long result = 0;

    switch( regs[0] ) {
	case 0x80000000:		// SMC calling convention version
		result = 0;			// version 1.0
		break;
    case 0x84000000:        // PSCI_VERSION
        result = 0x10001;   // version: 1.1
        break;
    case 0x84000003:        // CPU_ON
    case 0xc4000003:
        result = bringCpuOn(regs[1], regs[2], regs[3]);
        break;
    case 0x84000005:        // MIGRATE
        result = PSCI_NOT_SUPPORTED;
        break;
    case 0x84000006:        // MIGRATE_INFO_TYPE
        result = 2;
        break;
    case 0x84000009:        // SYSTEM_RESET
        systemReset();
        break;
    case 0x8400000a:        //  PSCI_FEATURES
        result = psciFeatures(regs[1]);
        break;
    default:
        printf("psciHandler FATAL: unknown function %x\r\n", regs[0]);
        boardReset();
    }
    regs[0] = result;
}

