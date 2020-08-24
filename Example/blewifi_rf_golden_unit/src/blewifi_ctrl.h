/******************************************************************************
*  Copyright 2017 - 2018, Opulinks Technology Ltd.
*  ----------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2018
******************************************************************************/

/**
 * @file blewifi_ctrl.h
 * @author Vincent Chen, Michael Liao
 * @date 20 Feb 2018
 * @brief File includes the function declaration of blewifi ctrl task.
 *
 */

#ifndef __BLEWIFI_CTRL_H__
#define __BLEWIFI_CTRL_H__

#include <stdint.h>
#include <stdbool.h>
#include "blewifi_configuration.h"
#include "mw_fim_default_group11_project.h"

#define BLEWIFI_CTRL_QUEUE_SIZE         (20)

typedef enum blewifi_ctrl_msg_type
{
    /* BLE Trigger */
    BLEWIFI_CTRL_MSG_BLE_INIT_COMPLETE = 0,             //BLE report status
    BLEWIFI_CTRL_MSG_BLE_ADVERTISING_CFM,               //BLE report status
    BLEWIFI_CTRL_MSG_BLE_ADVERTISING_EXIT_CFM,          //BLE report status
    BLEWIFI_CTRL_MSG_BLE_ADVERTISING_TIME_CHANGE_CFM,   //BLE report status
    BLEWIFI_CTRL_MSG_BLE_CONNECTION_COMPLETE,           //BLE report status
    BLEWIFI_CTRL_MSG_BLE_CONNECTION_FAIL,               //BLE report status
    BLEWIFI_CTRL_MSG_BLE_DISCONNECT,                    //BLE report status
    BLEWIFI_CTRL_MSG_BLE_DATA_IND,                      //BLE receive the data from peer to device

    BLEWIFI_CTRL_MSG_BLE_NUM,

    /* Wi-Fi Trigger */
    BLEWIFI_CTRL_MSG_WIFI_INIT_COMPLETE = 0x80, //Wi-Fi report status
    BLEWIFI_CTRL_MSG_WIFI_RESET_DEFAULT_IND,    //Wi-Fi report status
    BLEWIFI_CTRL_MSG_WIFI_SCAN_DONE_IND,        //Wi-Fi report status
    BLEWIFI_CTRL_MSG_WIFI_CONNECTION_IND,       //Wi-Fi report status
    BLEWIFI_CTRL_MSG_WIFI_DISCONNECTION_IND,    //Wi-Fi report status
    BLEWIFI_CTRL_MSG_WIFI_GOT_IP_IND,           //Wi-Fi report status
    BLEWIFI_CTRL_MSG_WIFI_AUTO_CONNECT_IND,     //Wi-Fi the auto connection is triggered by timer

    BLEWIFI_CTRL_MSG_WIFI_NUM,

    /* Others Event */
    BLEWIFI_CTRL_MSG_OTHER_OTA_ON = 0x100,      //OTA
    BLEWIFI_CTRL_MSG_OTHER_OTA_OFF,             //OTA success
    BLEWIFI_CTRL_MSG_OTHER_OTA_OFF_FAIL,        //OTA fail
    BLEWIFI_CTRL_MSG_OTHER_SYS_TIMER,           //SYS timer
    BLEWIFI_CTRL_MSG_NETWORKING_START,          //Networking Start
    BLEWIFI_CTRL_MSG_NETWORKING_STOP,           //Networking Stop
    BLEWIFI_CTRL_MSG_NETWORKING_PARAM_CHANGE,   //Networking Interval Time Change
#if (BLEWIFI_CTRL_BUTTON_SENSOR_EN == 1)
    BLEWIFI_CTRL_MSG_BUTTON_STATE_CHANGE,           //Button Stage Change
    BLEWIFI_CTRL_MSG_BUTTON_DEBOUNCE_TIMEOUT,       //Button Debounce Time Out
    BLEWIFI_CTRL_MSG_BUTTON_RELEASE_TIMEOUT,        //Button Release Time Out
#endif
#if (BLEWIFI_CTRL_WAKEUP_IO_EN == 1)
    BLEWIFI_CTRL_MSG_PS_SMART_STATE_CHANGE,         //PS SMART Stage Change
    BLEWIFI_CTRL_MSG_PS_SMART_DEBOUNCE_TIMEOUT,     //PS SMART Debounce Time Out
#endif
    BLEWIFI_CTRL_MSG_OTHER__NUM
} blewifi_ctrl_msg_type_e;

typedef struct
{
    uint32_t event;
    uint32_t length;
    uint8_t ucaMessage[];
} xBleWifiCtrlMessage_t;

typedef enum blewifi_ctrl_sys_state
{
    BLEWIFI_CTRL_SYS_INIT = 0x00,       // PS(0), Wifi(1), Ble(1)
    BLEWIFI_CTRL_SYS_NORMAL,            // PS(1), Wifi(1), Ble(1)
    BLEWIFI_CTRL_SYS_BLE_OFF,           // PS(1), Wifi(1), Ble(0)

    BLEWIFI_CTRL_SYS_NUM
} blewifi_ctrl_sys_state_e;

// event group bit (0 ~ 23 bits)
#define BLEWIFI_CTRL_EVENT_BIT_BLE_INIT_DONE    0x00000001U
#define BLEWIFI_CTRL_EVENT_BIT_BLE_ADVTISING    0x00000002U
#define BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED    0x00000004U
#define BLEWIFI_CTRL_EVENT_BIT_WIFI_INIT_DONE   0x00000008U
#define BLEWIFI_CTRL_EVENT_BIT_WIFI_SCANNING    0x00000010U
#define BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTING  0x00000020U
#define BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTED   0x00000040U
#define BLEWIFI_CTRL_EVENT_BIT_WIFI_GOT_IP      0x00000080U
#define BLEWIFI_CTRL_EVENT_BIT_NETWORK          0x00000100U
#define BLEWIFI_CTRL_EVENT_BIT_OTA              0x00000200U
#define BLEWIFI_CTRL_EVENT_BIT_IOT_INIT         0x00000400U

typedef void (*T_BleWifi_Ctrl_EvtHandler_Fp)(uint32_t evt_type, void *data, int len);
typedef struct
{
    uint32_t ulEventId;
    T_BleWifi_Ctrl_EvtHandler_Fp fpFunc;
} T_BleWifi_Ctrl_EvtHandlerTbl;

#define BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE   (BLEWIFI_WIFI_REQ_CONNECT_RETRY_TIMES + 1)
#define BLEWIFI_CTRL_AUTO_CONN_STATE_SCAN   (BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE + 1)


void BleWifi_Ctrl_SysModeSet(uint8_t mode);
uint8_t BleWifi_Ctrl_SysModeGet(void);
void BleWifi_Ctrl_DtimTimeSet(uint32_t value);
uint32_t BleWifi_Ctrl_DtimTimeGet(void);
int BleWifi_Ctrl_MsgSend(int msg_type, uint8_t *data, int data_len);
void BleWifi_Ctrl_Init(void);

void BleWifi_Ctrl_NetworkingStart(uint32_t u32ExpireTime);
void BleWifi_Ctrl_NetworkingStop(void);
#if (BLEWIFI_CTRL_BUTTON_SENSOR_EN == 1)
void BleWifi_Ctrl_ButtonReleaseHandle(uint8_t u8ReleaseCount);
#endif
void BleWifi_Ctrl_NetworkingParamChange(uint32_t u32IntervalTime);

#ifdef __BLEWIFI_TRANSPARENT__
#define BLE_PS_MODE     (1)
#define BLE_ADV_MODE    (2)

int BleWifi_Ctrl_BleCastWithExpire(uint8_t u8BleCastEnable, uint32_t u32ExpireTime);
int BleWifi_Ctrl_BleCastParamSet(uint32_t u32CastInteval);
#endif

#endif /* __BLEWIFI_CTRL_H__ */

