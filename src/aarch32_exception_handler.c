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

#include "sysheader.h"

U32 GetCPUID(void);
U32 GetSMCCode(void *);
U32 GetCurrentSMode(void);
void boardReset(void);

static void panic(const char *excName)
{
    printf("\r\n>> PANIC! <%s> exception on CPU%d level %d\r\n",
            excName, GetCPUID(), GetCurrentSMode());
    boardReset();
}

void undef_instr_handler(void)
{
    panic("undefined instruction");
}

void smc_handler(void)
{
    panic("smc");
}

void prefetch_abort_handler(void)
{
    panic("prefetch abort");
}

void data_abort_handler(void)
{
    panic("data abort");
}

void hyp_trap_handler(void)
{
    panic("hyp trap");
}

void irq_handler(void)
{
    panic("irq");
}

void fiq_handler(void)
{
    panic("fiq");
}

