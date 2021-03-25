

#ifndef __SMART_SLEEP_H__
#define __SMART_SLEEP_H__

#include "blewifi_configuration.h"
#if (BLEWIFI_CTRL_WAKEUP_IO_EN == 1)

#include "cmsis_os.h"
#include "hal_vic.h"
#include "blewifi_ctrl.h"

extern osTimerId g_tAppPsSmartDebounceTimerId;

void BleWifi_Ps_Smart_Init(E_GpioIdx_t tGpioIdx, uint8_t ubSysMode, uint8_t ubPowerSaving);
void BleWifi_Ps_Smart_Pin_Config(E_GpioIdx_t tGpioIdx, uint32_t u32PinLevel);

#endif  // end of (BLEWIFI_CTRL_WAKEUP_IO_EN == 1)

#endif  // end of __SMART_SLEEP_H__
