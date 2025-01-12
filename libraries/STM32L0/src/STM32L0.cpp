/*
 * Copyright (c) 2017-2020 Thomas Roell.  All rights reserved.
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

#include "Arduino.h"
#include "STM32L0.h"
#include "wiring_private.h"

uint64_t STM32L0Class::getSerial()
{
    return stm32l0_system_serial();
}

void STM32L0Class::getUID(uint32_t uid[3])
{
    stm32l0_system_uid(uid);
}

float STM32L0Class::getVBAT()
{
#if defined(STM32L0_CONFIG_PIN_VBAT)
    int32_t vrefint_data, vbat_data;
    float vdda;

    vrefint_data = __analogReadInternal(STM32L0_ADC_CHANNEL_VREFINT, STM32L0_ADC_VREFINT_PERIOD);
    vbat_data = __analogReadInternal(STM32L0_CONFIG_CHANNEL_VBAT, STM32L0_CONFIG_VBAT_PERIOD);

    vdda = (3.0 * STM32L0_ADC_VREFINT_CAL) / vrefint_data;

    return (STM32L0_CONFIG_VBAT_SCALE * vdda * vbat_data) / 4095.0;

#else /* defined(STM32L0_CONFIG_PIN_VBAT) */

    return -1;

#endif /* defined(STM32L0_CONFIG_PIN_VBAT) */
}

float STM32L0Class::getVDDA()
{
    int32_t vrefint_data;

    vrefint_data = __analogReadInternal(STM32L0_ADC_CHANNEL_VREFINT, STM32L0_ADC_VREFINT_PERIOD);

    return (3.0 * STM32L0_ADC_VREFINT_CAL) / vrefint_data;
}

float STM32L0Class::getTemperature()
{
    int32_t vrefint_data, tsense_data;

    vrefint_data = __analogReadInternal(STM32L0_ADC_CHANNEL_VREFINT, STM32L0_ADC_VREFINT_PERIOD);
    tsense_data = __analogReadInternal(STM32L0_ADC_CHANNEL_TSENSE, STM32L0_ADC_TSENSE_PERIOD);

    /* Compensate TSENSE_DATA for VDDA vs. 3.0 */
    tsense_data = (tsense_data * STM32L0_ADC_VREFINT_CAL) / vrefint_data;

    return (30.0 + (100.0 * (float)(tsense_data - STM32L0_ADC_TSENSE_CAL1)) / (float)(STM32L0_ADC_TSENSE_CAL2 - STM32L0_ADC_TSENSE_CAL1));
}

uint32_t STM32L0Class::resetCause()
{
    return stm32l0_system_reset_cause();
}

uint32_t STM32L0Class::wakeupReason()
{
    return stm32l0_system_wakeup_reason();
}

bool STM32L0Class::setClocks(uint32_t hclk, uint32_t pclk1, uint32_t pclk2)
{
    return stm32l0_system_sysclk_configure(hclk, pclk1, pclk2);
}

void STM32L0Class::setClocks(uint32_t &hclk, uint32_t &pclk1, uint32_t &pclk2)
{
    hclk = stm32l0_system_hclk();
    pclk1 = stm32l0_system_pclk1();
    pclk2 = stm32l0_system_pclk2();
}

void STM32L0Class::enablePowerSave()
{
    g_defaultPolicy = STM32L0_SYSTEM_POLICY_SLEEP;
}

void STM32L0Class::disablePowerSave()
{
    g_defaultPolicy = STM32L0_SYSTEM_POLICY_RUN;
}

void STM32L0Class::wakeup()
{
    stm32l0_system_wakeup(STM32L0_SYSTEM_EVENT_APPLICATION);
}

void STM32L0Class::sleep(uint32_t timeout)
{
    if (g_swdStatus == 0) {
        stm32l0_system_swd_disable();

        g_swdStatus = 2;
    }

    stm32l0_system_sleep(STM32L0_SYSTEM_POLICY_SLEEP, STM32L0_SYSTEM_EVENT_APPLICATION, timeout);
}

void STM32L0Class::deepsleep(uint32_t timeout)
{
    if (g_swdStatus == 0) {
        stm32l0_system_swd_disable();

        g_swdStatus = 2;
    }

    stm32l0_system_sleep(STM32L0_SYSTEM_POLICY_DEEPSLEEP, STM32L0_SYSTEM_EVENT_APPLICATION, timeout);
}

void STM32L0Class::standby(uint32_t timeout)
{
    stm32l0_system_standby(g_standbyControl, timeout);
}

void STM32L0Class::reset()
{
    stm32l0_system_reset();
}

void STM32L0Class::dfu()
{
    stm32l0_system_dfu();
}

void STM32L0Class::swdEnable()
{
    if (g_swdStatus != 3) {
        stm32l0_system_swd_enable();

        g_swdStatus = 1;
    }
}

void STM32L0Class::swdDisable()
{
    if (g_swdStatus != 3) {
        stm32l0_system_swd_disable();

        g_swdStatus = 2;
    }
}

void STM32L0Class::wdtEnable(uint32_t timeout)
{
    stm32l0_iwdg_enable(timeout);
}

void STM32L0Class::wdtReset()
{
    stm32l0_iwdg_reset();
}

bool STM32L0Class::flashErase(uint32_t address, uint32_t count)
{
    if (address & 127) {
        return false;
    }

    count = (count + 127) & ~127;

    if ((address < FLASHSTART) || ((address + count) > FLASHEND)) {
        return false;
    }

    stm32l0_flash_unlock();
    stm32l0_flash_erase(address, count);
    stm32l0_flash_lock();

    return true;
}

bool STM32L0Class::flashProgram(uint32_t address, const void *data, uint32_t count)
{
    if ((address & 3) || (count & 3)) {
        return false;
    }

    if ((address < FLASHSTART) || ((address + count) > FLASHEND)) {
        return false;
    }

    if (count) {
        stm32l0_flash_unlock();
        stm32l0_flash_program(address, (const uint8_t*)data, count);
        stm32l0_flash_lock();
    }

    return true;
}

void STM32L0Class::goToDFU()
{
  if (stm32l0_flash_unlock())
  {
    if (stm32l0_flash_erase(0x08018000, 128) && stm32l0_flash_erase(0x08000000, 128))
    {
      STM32L0.reset();
    }
  }
}

bool STM32L0Class::setBFB2()
{
  // Check to see if the BFB2 bit is set
  if ((FLASH->OPTR & FLASH_OPTR_BFB2) == 0)
  {
    if (stm32l0_optbyte_unlock())
    {
      if (stm32l0_optbyte_program(0x01, 0x80F0))
      {
        FLASH->PECR |= FLASH_PECR_OBL_LAUNCH;
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }
  else
  {
    return true;
  }
}

void STM32L0Class::waitForDFU(Uart NRF)
{
  // Used for timeouts
  uint32_t ms = 0;

  // Look for the first character informing us we need to flush everything out of the buffer
  if(NRF.available())
  {
    if(NRF.read() != 0x1B)
    {
      return;
    }
  }
  else
  {
    return;
  }

  // Flush everything out of the buffer, then send a response
  NRF.flush();
  NRF.write(0x1B);

  // Now everything has to be pretty precise
  // Give them a little more time here to clear the buffer
  // In case we were sending a lot of stuff when
  // asked to go to DFU
  ms = millis();
  while(millis() - ms < 1000)
  {
    if(NRF.available())
    {
      if(NRF.read() == 0x1B)
      {
        // Continue
        break;
      }
      else
      {
        // Wrong character, abort
        return;
      }
    }
  }

  // Ask to confirm
  NRF.write('b');
  NRF.flush();

  // Need to get response within 100ms
  ms = millis();
  while(millis() - ms < 100)
  {
    if(NRF.available())
    {
      // Got confirm
      if(NRF.read() == 'c')
      {
        // Clear the buffer
        while (NRF.available())
        {
          NRF.read();
        }
        // Do it
        STM32L0.goToDFU();
      }
      else
      {
        // Wrong character, abort
        return;
      }
    }
  }
}

STM32L0Class STM32L0;
