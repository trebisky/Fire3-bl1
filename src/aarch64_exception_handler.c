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

U32 GetCurrentSMode(void);
U32 GetCPUID(void);
void psciHandler(unsigned long*);
void boardReset(void);

void sync_c_handler_EL3(unsigned exc, unsigned esr, unsigned long *regs)
{
    unsigned excClass = esr >> 26, excSyndrome = esr & 0x1ffffff;

    switch( exc ) {
    case 0x280:     // IRQ
        WriteIO32(0xc0009f10, -1);  // clear GIC interrupt
        return;
    case 0x400:
        if( excClass == 0x17 ) {   // smc
            if( excSyndrome == 0 ) {
                psciHandler(regs);
                return;
            }else if( excSyndrome == 0x123 ) {
                putchar(regs[0]);
                return;
            }
        }
    }
    printf("\r\n>> PANIC! exception 0x%x on CPU%d level %d, class=0x%x syndrome=0x%x\r\n",
            exc, GetCPUID(), GetCurrentSMode(), esr >> 26, esr & 0x1ffffff);
    boardReset();
}
