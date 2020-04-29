/******************************************************************************
*  Copyright 2019 - 2019, Opulinks Technology Ltd.
*  ----------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2018
******************************************************************************/
#ifndef __BTN_PRESS_CTRL_H__
#define __BTN_PRESS_CTRL_H__

#include "blewifi_configuration.h"
#if (BLEWIFI_CTRL_BUTTON_SENSOR_EN == 1)

#include "cmsis_os.h"
#include "hal_vic.h"

typedef enum
{
    BLEWIFI_BUTTON_STATE_INIT = 0,
    BLEWIFI_BUTTON_STATE_CHECK_SHORT_PRESS,
    BLEWIFI_BUTTON_STATE_CHECK_LONG_PRESS,

    BLEWIFI_BUTTON_STATE_MAX
} T_BleWifiButtonState;

typedef enum
{
    BLEWIFI_BUTTON_EVENT_PRESS = 0,
    BLEWIFI_BUTTON_EVENT_RELEASE,
    BLEWIFI_BUTTON_EVENT_TIMEOUT,       // used in the count of short press or timeout of long press

    BLEWIFI_BUTTON_EVENT_MAX
} T_BleWifiButtonEvent;


extern osTimerId g_tAppButtonDebounceTimerId;
extern osTimerId g_tAppButtonReleaseTimerId;

void BleWifi_ButtonPress_Init(void);
void BleWifi_ButtonFsm_Run(uint8_t u8Evt);

#endif // end of (BLEWIFI_CTRL_BUTTON_SENSOR_EN == 1)

#endif // end of __BTN_PRESS_CTRL_H__

