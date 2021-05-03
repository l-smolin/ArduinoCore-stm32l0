/*
 * Copyright (c) 2017-2018 Thomas Roell.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimers.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimers in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Thomas Roell, nor the names of its contributors
 *     may be used to endorse or promote products derived from this Software
 *     without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

#include "armv6m.h"
#include "stm32l0xx.h"

#include "stm32l0_optbyte.h"

#define FLASH_PEKEY1               (0x89ABCDEFU) // Flash program erase key1
#define FLASH_PEKEY2               (0x02030405U) // Flash program erase key: used with FLASH_PEKEY2
                                                 // to unlock the write access to the FLASH_PECR register and
                                                 // data EEPROM

#define FLASH_PRGKEY1              (0x8C9DAEBFU) // Flash program memory key1
#define FLASH_PRGKEY2              (0x13141516U) // Flash program memory key2: used with FLASH_PRGKEY2
                                                 // to unlock the program memory

#define FLASH_OPTKEY1              (0xFBEAD9C8U) // Flash option key1 */
#define FLASH_OPTKEY2              (0x24252627U) // Flash option key2: used with FLASH_OPTKEY1 to unlock the write access to the option byte block


static void __empty() { }

void stm32l0_eeprom_acquire(void) __attribute__ ((weak, alias("__empty")));
void stm32l0_eeprom_release(void) __attribute__ ((weak, alias("__empty")));

bool stm32l0_optbyte_unlock(void)
{
    uint32_t primask;

    stm32l0_eeprom_acquire();

    primask = __get_PRIMASK();

    __disable_irq();

    if (FLASH->PECR & FLASH_PECR_PELOCK)
    {
        FLASH->PEKEYR  = FLASH_PEKEY1;
        FLASH->PEKEYR  = FLASH_PEKEY2;
    }

    if (!(FLASH->PECR & FLASH_PECR_PELOCK))
    {
        if (FLASH->PECR & FLASH_PECR_OPTLOCK)
        {
            FLASH->OPTKEYR = FLASH_OPTKEY1;
            FLASH->OPTKEYR = FLASH_OPTKEY2;
        }
    }

    __set_PRIMASK(primask);

    if (FLASH->PECR & FLASH_PECR_OPTLOCK)
    {
        stm32l0_eeprom_release();

        return false;
    }

    return true;
}

void stm32l0_optbyte_lock(void)
{
  uint32_t primask;

  primask = __get_PRIMASK();

  __disable_irq();

  FLASH->PECR |= FLASH_PECR_PELOCK;
  FLASH->PECR |= FLASH_PECR_OPTLOCK;

  __set_PRIMASK(primask);

  stm32l0_eeprom_release();
}


static __attribute__((optimize("O3"), section(".ramfunc.stm32l0_optbyte_do_program"), long_call)) void stm32l0_optbyte_do_program(uint8_t index, uint16_t data)
{
  // Write a 32-bit word value at the option byte address, the 16-bit data is extended with its compemented value
	*(__IO uint32_t *)(OB_BASE + (index * 4)) = (uint32_t)((~data << 16) | data);

  __DMB();

  while (FLASH->SR & FLASH_SR_BSY)
  {
  }
}


// Flash an option byte
// This function actually flashes a word at a time as that is what is needed to ensure we are
// flashing the complemented upper word
// Index is the word that we are programming (see Table 25)
bool stm32l0_optbyte_program(uint8_t index, uint16_t data)
{
  if (index > 4)
  {
    // Too high no option bytes there
    return false;
  }

  stm32l0_optbyte_do_program(index, data);

	// Check the EOP flag in the FLASH_SR register
	if ((FLASH->SR & FLASH_SR_EOP) != 0)
	{
		// Clear EOP flag by software by writing EOP at 1
		FLASH->SR = FLASH_SR_EOP;
	}
	else
	{
		// Some error occurred
    return true;
	}
}
