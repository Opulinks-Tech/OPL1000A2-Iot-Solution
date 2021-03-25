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
 * @file blewifi_ctrl.c
 * @author Vincent Chen, Michael Liao
 * @date 20 Feb 2018
 * @brief File creates the blewifi ctrl task architecture.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "cmsis_os.h"
#include "event_groups.h"
#include "sys_os_config.h"
#include "sys_os_config_patch.h"
#include "at_cmd_common.h"

#include "blewifi_common.h"
#include "blewifi_configuration.h"
#include "blewifi_ctrl.h"
#include "blewifi_wifi_api.h"
#include "blewifi_ble_api.h"
#include "blewifi_data.h"
#include "blewifi_app.h"
#include "mw_ota_def.h"
#include "mw_ota.h"
#include "hal_system.h"
#include "mw_fim_default_group03.h"
#include "mw_fim_default_group03_patch.h"
#include "mw_fim_default_group11_project.h"
#include "ps_public.h"
#include "wifi_api.h"

#if (BLEWIFI_CTRL_BUTTON_SENSOR_EN == 1)
#include "btn_press_ctrl.h"
#endif
#if (BLEWIFI_CTRL_WAKEUP_IO_EN == 1)
#include "smart_sleep.h"
#endif

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
#include "ssidpwd_proc.h"
#endif

#define BLEWIFI_CTRL_RESET_DELAY    (3000)  // ms

osThreadId   g_tAppCtrlTaskId;
osMessageQId g_tAppCtrlQueueId;
osTimerId    g_tAppCtrlAutoConnectTriggerTimer;
osTimerId    g_tAppCtrlSysTimer;
osTimerId    g_tAppCtrlNetworkTimerId;

EventGroupHandle_t g_tAppCtrlEventGroup;

uint8_t g_ulAppCtrlSysMode;
uint8_t g_ubAppCtrlSysStatus;

uint8_t g_ubAppCtrlRequestRetryTimes;
uint32_t g_ulAppCtrlAutoConnectInterval;
uint32_t g_ulAppCtrlWifiDtimTime;

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
uint8_t g_iIdxOfApInfo; //Record Idx of AP for doing connection if connction failed

bool g_bAutoconn_flag       = true;
bool g_bRoaming_flag        = false;

bool g_bConnectReqByUser_flag = false;
bool g_bScanReqByUser_flag    = false;

extern apinfo_t g_apInfo[MW_FIM_GP11_FIXED_SSID_PWD_NUM+MW_FIM_GP11_SSID_PWD_NUM];

extern uint8_t g_RoamingApInfoTotalCnt;
extern wifi_config_t wifi_config_req_connect;
#endif

static void BleWifi_Ctrl_TaskEvtHandler_BleInitComplete(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingCfm(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingExitCfm(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingTimeChangeCfm(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleConnectionComplete(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleConnectionFail(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleDisconnect(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_BleDataInd(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiInitComplete(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiResetDefaultInd(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiScanDoneInd(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiConnectionInd(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiDisconnectionInd(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiGotIpInd(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_WifiAutoConnectInd(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_OtherOtaOn(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_OtherOtaOff(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_OtherOtaOffFail(uint32_t evt_type, void *data, int len);
#if (BLEWIFI_CTRL_BUTTON_SENSOR_EN == 1)
static void BleWifi_Ctrl_TaskEvtHandler_ButtonStateChange(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_ButtonDebounceTimeOut(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_ButtonReleaseTimeOut(uint32_t evt_type, void *data, int len);
#endif
static void BleWifi_Ctrl_TaskEvtHandler_OtherSysTimer(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_NetworkingStart(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_NetworkingStop(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_NetworkingParamChange(uint32_t evt_type, void *data, int len);
#if (BLEWIFI_CTRL_WAKEUP_IO_EN == 1)
static void BleWifi_Ctrl_TaskEvtHandler_PsSmartStateChange(uint32_t evt_type, void *data, int len);
static void BleWifi_Ctrl_TaskEvtHandler_PsSmartDebounceTimeOut(uint32_t evt_type, void *data, int len);
#endif
static T_BleWifi_Ctrl_EvtHandlerTbl g_tCtrlEvtHandlerTbl[] =
{
    {BLEWIFI_CTRL_MSG_BLE_INIT_COMPLETE,                BleWifi_Ctrl_TaskEvtHandler_BleInitComplete},
    {BLEWIFI_CTRL_MSG_BLE_ADVERTISING_CFM,              BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingCfm},
    {BLEWIFI_CTRL_MSG_BLE_ADVERTISING_EXIT_CFM,         BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingExitCfm},
    {BLEWIFI_CTRL_MSG_BLE_ADVERTISING_TIME_CHANGE_CFM,  BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingTimeChangeCfm},
    {BLEWIFI_CTRL_MSG_BLE_CONNECTION_COMPLETE,          BleWifi_Ctrl_TaskEvtHandler_BleConnectionComplete},
    {BLEWIFI_CTRL_MSG_BLE_CONNECTION_FAIL,              BleWifi_Ctrl_TaskEvtHandler_BleConnectionFail},
    {BLEWIFI_CTRL_MSG_BLE_DISCONNECT,                   BleWifi_Ctrl_TaskEvtHandler_BleDisconnect},
    {BLEWIFI_CTRL_MSG_BLE_DATA_IND,                     BleWifi_Ctrl_TaskEvtHandler_BleDataInd},

    {BLEWIFI_CTRL_MSG_WIFI_INIT_COMPLETE,               BleWifi_Ctrl_TaskEvtHandler_WifiInitComplete},
    {BLEWIFI_CTRL_MSG_WIFI_RESET_DEFAULT_IND,           BleWifi_Ctrl_TaskEvtHandler_WifiResetDefaultInd},
    {BLEWIFI_CTRL_MSG_WIFI_SCAN_DONE_IND,               BleWifi_Ctrl_TaskEvtHandler_WifiScanDoneInd},
    {BLEWIFI_CTRL_MSG_WIFI_CONNECTION_IND,              BleWifi_Ctrl_TaskEvtHandler_WifiConnectionInd},
    {BLEWIFI_CTRL_MSG_WIFI_DISCONNECTION_IND,           BleWifi_Ctrl_TaskEvtHandler_WifiDisconnectionInd},
    {BLEWIFI_CTRL_MSG_WIFI_GOT_IP_IND,                  BleWifi_Ctrl_TaskEvtHandler_WifiGotIpInd},
    {BLEWIFI_CTRL_MSG_WIFI_AUTO_CONNECT_IND,            BleWifi_Ctrl_TaskEvtHandler_WifiAutoConnectInd},

    {BLEWIFI_CTRL_MSG_OTHER_OTA_ON,                     BleWifi_Ctrl_TaskEvtHandler_OtherOtaOn},
    {BLEWIFI_CTRL_MSG_OTHER_OTA_OFF,                    BleWifi_Ctrl_TaskEvtHandler_OtherOtaOff},
    {BLEWIFI_CTRL_MSG_OTHER_OTA_OFF_FAIL,               BleWifi_Ctrl_TaskEvtHandler_OtherOtaOffFail},
#if (BLEWIFI_CTRL_BUTTON_SENSOR_EN==1)
    {BLEWIFI_CTRL_MSG_BUTTON_STATE_CHANGE,              BleWifi_Ctrl_TaskEvtHandler_ButtonStateChange},
    {BLEWIFI_CTRL_MSG_BUTTON_DEBOUNCE_TIMEOUT,          BleWifi_Ctrl_TaskEvtHandler_ButtonDebounceTimeOut},
    {BLEWIFI_CTRL_MSG_BUTTON_RELEASE_TIMEOUT,           BleWifi_Ctrl_TaskEvtHandler_ButtonReleaseTimeOut},
#endif
#if (BLEWIFI_CTRL_WAKEUP_IO_EN == 1)
    {BLEWIFI_CTRL_MSG_PS_SMART_STATE_CHANGE,            BleWifi_Ctrl_TaskEvtHandler_PsSmartStateChange},
    {BLEWIFI_CTRL_MSG_PS_SMART_DEBOUNCE_TIMEOUT,        BleWifi_Ctrl_TaskEvtHandler_PsSmartDebounceTimeOut},
#endif
    {BLEWIFI_CTRL_MSG_OTHER_SYS_TIMER,                  BleWifi_Ctrl_TaskEvtHandler_OtherSysTimer},
    {BLEWIFI_CTRL_MSG_NETWORKING_START,                 BleWifi_Ctrl_TaskEvtHandler_NetworkingStart},
    {BLEWIFI_CTRL_MSG_NETWORKING_STOP,                  BleWifi_Ctrl_TaskEvtHandler_NetworkingStop},
    {BLEWIFI_CTRL_MSG_NETWORKING_PARAM_CHANGE,          BleWifi_Ctrl_TaskEvtHandler_NetworkingParamChange},
    {0xFFFFFFFF,                                        NULL}
};

void BleWifi_Ctrl_SysModeSet(uint8_t mode)
{
    g_ulAppCtrlSysMode = mode;
}

uint8_t BleWifi_Ctrl_SysModeGet(void)
{
    return g_ulAppCtrlSysMode;
}

void BleWifi_Ctrl_DtimTimeSet(uint32_t value)
{
    g_ulAppCtrlWifiDtimTime = value;
    BleWifi_Wifi_SetDTIM(g_ulAppCtrlWifiDtimTime);
}

uint32_t BleWifi_Ctrl_DtimTimeGet(void)
{
    return g_ulAppCtrlWifiDtimTime;
}

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
/* According scan result, get bssid with MAX RSSI */
int BleWifi_Wifi_Get_BSsid(apinfo_t *tApInfo)
{
    int iRet = -1;
    wifi_scan_info_t *ap_list = NULL;
    uint16_t apCount = 0;
    int32_t i = 0;
    int32_t idx = 0;
    int32_t MaxRssiIdx=0;

    wifi_scan_get_ap_num(&apCount);

    if(apCount == 0)
    {
        printf("No AP found\r\n");
        goto err;
	}
	BLEWIFI_ERROR("ap num = %d\n", apCount);
	ap_list = (wifi_scan_info_t *)malloc(sizeof(wifi_scan_info_t) * apCount);

	if (!ap_list) {
        printf("Get_BSsid: malloc fail\n");
		goto err;
	}

    wifi_scan_get_ap_records(&apCount, ap_list);

    /* build blewifi ap list */
    for(i = 0; i < apCount; i++)
    {
        if(0 == memcmp(ap_list[i].ssid, tApInfo->tSsidPwd.ssid, sizeof(ap_list[i].ssid)))
        {
            iRet = 0;
            MaxRssiIdx = i;
            for(idx = i; idx < apCount; idx++)
            {
                //if there are 2 or more same ssid, find max rssi to connect.
                if(0 == memcmp(ap_list[idx].ssid, tApInfo->tSsidPwd.ssid, sizeof(ap_list[idx].ssid)))
                {
                    if( ap_list[idx].rssi > ap_list[i].rssi )
                    {
                        MaxRssiIdx = idx;
                    }
                }
            }
            break;
        }
    }

    if( 0 == iRet )
    {
        memcpy(tApInfo->bssid, ap_list[MaxRssiIdx].bssid, 6);
    }

err:
    if (ap_list)
        free(ap_list);

    return iRet;
}

int Wifi_Roaming_Connect(uint8_t *bssid, uint8_t bssid_len, uint8_t *password, uint8_t pwd_len)
{
    int ret=0;
	uint8_t *conn_data;
	uint8_t conn_data_len=bssid_len+pwd_len+2;

	conn_data = malloc(sizeof(uint8_t)*conn_data_len);
    if(conn_data == NULL)
    {
         BLEWIFI_ERROR("WiFi_Connect: malloc fail\n");
         return -1;
    }
	memset(conn_data, 0, sizeof(uint8_t)*conn_data_len);

	memcpy(conn_data, bssid, bssid_len);   // BSSID
	*(conn_data+bssid_len) = 0;            // connected or not
	*(conn_data+bssid_len+1) = pwd_len;    // password len
	memcpy((conn_data+bssid_len+2), password, pwd_len);

	ret = BleWifi_Wifi_DoConnect(conn_data, conn_data_len);

    printf("[%s %d]BleWifi_Wifi_DoConnect(ret=%d)\n", __func__, __LINE__, ret);

	free(conn_data);

    return ret;
}

int BleWifi_Ctrl_DoAutoConnect(void)
{
    wifi_scan_config_t scan_config = {0};
    int ret = -1;
    int api_ret;

    if ((true == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED)) || (true == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTED)))
    {
        return ret;
    }

    if( true == g_bAutoconn_flag )
    {
        // if the count of auto-connection list is empty,
        if (0 == BleWifi_Wifi_AutoConnectListNum())
        {
            g_bRoaming_flag = true;
            g_bAutoconn_flag = false;
        }
        else
        {
            //printf("\n\n!!! >>> %s,%d, call wifi_auto_connect_start!!!\n\n", __FUNCTION__, __LINE__);
            api_ret = wifi_auto_connect_start();
            if( api_ret == 0 )
            {
                ret = 0;
            }
            else
            {
                g_bRoaming_flag = true;
                g_bAutoconn_flag = false;
            }
        }
    }
    else
    {
        g_bRoaming_flag = true;
    }

    if (true == g_bRoaming_flag)
    {
        if (0 != g_RoamingApInfoTotalCnt)
        {
            printf("[%s %d]Roaming List NUM:%d\n", __FUNCTION__, __LINE__, g_RoamingApInfoTotalCnt);

            g_iIdxOfApInfo = 0; //initialize index for roaming connection

            scan_config.show_hidden = 1; // Enable to scan AP whose SSID is hidden
            scan_config.scan_type = WIFI_SCAN_TYPE_MIX;  // mixed mode

            api_ret = BleWifi_Wifi_DoScan(&scan_config);
            if (0 == api_ret)
            {
                ret = 0;
            }
            else
            {
                g_bRoaming_flag = false;
            }
        }
        else
        {
            g_bRoaming_flag = false;
        }
    }

    return ret;
}
#else
void BleWifi_Ctrl_DoAutoConnect(void)
{
    wifi_scan_config_t scan_config = {0};

    // if the count of auto-connection list is empty, don't do the auto-connect
    if (0 == BleWifi_Wifi_AutoConnectListNum())
        return;

    // if request connect by Peer side, don't do the auto-connection
    if (g_ubAppCtrlRequestRetryTimes <= BLEWIFI_WIFI_REQ_CONNECT_RETRY_TIMES)
        return;

    // BLE is disconnect and Wifi is disconnect, too.
    if ((false == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED)) && (false == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTED)))
    {
        // start to scan
        // after scan, do the auto-connect
        if (g_ubAppCtrlRequestRetryTimes == BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE)
        {
            scan_config.show_hidden = 1; // Enable to scan AP whose SSID is hidden
            scan_config.scan_type = WIFI_SCAN_TYPE_MIX;  // mixed mode

            BleWifi_Wifi_DoScan(&scan_config);

            g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_SCAN;
        }
    }
}
#endif

void BleWifi_Ctrl_AutoConnectTrigger(void const *argu)
{
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_WIFI_AUTO_CONNECT_IND, NULL, 0);
}

void BleWifi_Ctrl_SysStatusChange(void)
{
    T_MwFim_SysMode tSysMode;

    // get the settings of system mode
    if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP03_PATCH_SYS_MODE, 0, MW_FIM_SYS_MODE_SIZE, (uint8_t*)&tSysMode))
    {
        // if fail, get the default value
        memcpy(&tSysMode, &g_tMwFimDefaultSysMode, MW_FIM_SYS_MODE_SIZE);
    }

    // change from init to normal
    if (g_ubAppCtrlSysStatus == BLEWIFI_CTRL_SYS_INIT)
    {
        g_ubAppCtrlSysStatus = BLEWIFI_CTRL_SYS_NORMAL;

        /* Power saving settings */
        if (tSysMode.ubSysMode == MW_FIM_SYS_MODE_USER)
        {
#if (BLEWIFI_CTRL_WAKEUP_IO_EN == 1)
            BleWifi_Ps_Smart_Init(BLEWIFI_CTRL_WAKEUP_IO_PORT, tSysMode.ubSysMode, BLEWIFI_COM_POWER_SAVE_EN);
#else
            ps_smart_sleep(BLEWIFI_COM_POWER_SAVE_EN);
#endif
        }

//        // start the sys timer
//        osTimerStop(g_tAppCtrlSysTimer);
//        osTimerStart(g_tAppCtrlSysTimer, BLEWIFI_COM_SYS_TIME_NORMAL);
    }
    // change from normal to ble off
    else if (g_ubAppCtrlSysStatus == BLEWIFI_CTRL_SYS_NORMAL)
    {
        g_ubAppCtrlSysStatus = BLEWIFI_CTRL_SYS_BLE_OFF;

//        // change the advertising time
//        BleWifi_Ble_AdvertisingTimeChange(BLEWIFI_BLE_ADVERTISEMENT_INTERVAL_PS_MIN, BLEWIFI_BLE_ADVERTISEMENT_INTERVAL_PS_MAX);
    }
}

uint8_t BleWifi_Ctrl_SysStatusGet(void)
{
    return g_ubAppCtrlSysStatus;
}

void BleWifi_Ctrl_SysTimeout(void const *argu)
{
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_OTHER_SYS_TIMER, NULL, 0);
}

static void BleWifi_Ctrl_TaskEvtHandler_BleInitComplete(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_INIT_COMPLETE \r\n");
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_INIT_DONE , true);

    /* BLE Init Step 2: Do BLE Advertising*/
    BleWifi_Ble_StartAdvertising();
}

static void BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingCfm(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_ADVERTISING_CFM \r\n");

    /* BLE Init Step 3: BLE is ready for peer BLE device's connection trigger */
}

static void BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingExitCfm(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_ADVERTISING_EXIT_CFM \r\n");
}

static void BleWifi_Ctrl_TaskEvtHandler_BleAdvertisingTimeChangeCfm(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_ADVERTISING_TIME_CHANGE_CFM \r\n");
#ifdef __BLEWIFI_TRANSPARENT__
    if ( true == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_ADVTISING)) {
        msg_print_uart1("+BLECAST:BLE ENTER ADVERTISING\n");
    }
    else {
        msg_print_uart1("+BLECAST:BLE EXIT ADVERTISING\n");
    }
#endif
}

static void BleWifi_Ctrl_TaskEvtHandler_BleConnectionComplete(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_CONNECTION_COMPLETE \r\n");
#ifdef __BLEWIFI_TRANSPARENT__
    msg_print_uart1("+BLECONN:PEER CONNECTION\n");
#endif

    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED , true);

    /* BLE Init Step 4: BLE said it's connected with a peer BLE device */
}

static void BleWifi_Ctrl_TaskEvtHandler_BleConnectionFail(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_CONNECTION_FAIL \r\n");
    BleWifi_Ble_StartAdvertising();
}

static void BleWifi_Ctrl_TaskEvtHandler_BleDisconnect(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_DISCONNECT \r\n");
#ifdef __BLEWIFI_TRANSPARENT__
    msg_print_uart1("+BLECONN:PEER DISCONNECTION\n");
#endif

    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED , false);
    BleWifi_Ble_StartAdvertising();

    /* start to do auto-connection. */
    g_ulAppCtrlAutoConnectInterval = BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_INIT;
    BleWifi_Ctrl_DoAutoConnect();

    /* stop the OTA behavior */
    if (gTheOta)
    {
        MwOta_DataGiveUp();
        free(gTheOta);
        gTheOta = 0;

        BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_OTHER_OTA_OFF_FAIL, NULL, 0);
    }
}

static void BleWifi_Ctrl_TaskEvtHandler_BleDataInd(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BLE_DATA_IND \r\n");
    BleWifi_Ble_DataRecvHandler(data, len);
}

static void BleWifi_Ctrl_TaskEvtHandler_WifiInitComplete(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_INIT_COMPLETE \r\n");
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_INIT_DONE , true);

    /* When device power on, start to do auto-connection. */
    g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE;
    g_ulAppCtrlAutoConnectInterval = BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_INIT;

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
    g_bAutoconn_flag = true;
    g_bRoaming_flag = false;
#endif

    BleWifi_Ctrl_DoAutoConnect();
}

static void BleWifi_Ctrl_TaskEvtHandler_WifiResetDefaultInd(uint32_t evt_type, void *data, int len)
{
    //BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_RESET_DEFAULT_IND \r\n");
    printf("\033[1;32;40m");
    printf("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_RESET_DEFAULT_IND");
    printf("\033[0m");
    printf("\r\n");
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_INIT_DONE , true);
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_SCANNING , false);
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTING , false);
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTED , false);
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_GOT_IP , false);

    g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE;
    g_ulAppCtrlAutoConnectInterval = BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_INIT;

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
    g_bAutoconn_flag = true;
    g_bRoaming_flag = false;
#endif
    /* When device power on, start to do auto-connection. */
    BleWifi_Ctrl_DoAutoConnect();
}

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
static void BleWifi_Ctrl_TaskEvtHandler_WifiScanDoneInd(uint32_t evt_type, void *data, int len)
{
    int iIdx=0;

    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_SCAN_DONE_IND \r\n");
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_SCANNING, false);

    if (true == g_bScanReqByUser_flag)
    {
        BleWifi_Wifi_SendScanReport();
        BleWifi_Ble_SendResponse(BLEWIFI_RSP_SCAN_END, 0);
        g_bScanReqByUser_flag = false;

    }
    // scan by auto-connect (roaming)
    else
    {
        //BleWifi_Wifi_UpdateScanInfoToAutoConnList(&u8IsUpdate);
        //printf("[%s,%d] roaming cnt = %d, flag = %d\n\n", __FUNCTION__, __LINE__, g_RoamingApInfoTotalCnt, g_bRoaming_flag);

        if((0 != g_RoamingApInfoTotalCnt) && (true == g_bRoaming_flag))
        {
            //printf("\ng_iIdxOfApInfo=%d, g_RoamingApInfoCnt=%d\n", g_iIdxOfApInfo, g_RoamingApInfoTotalCnt);
            for( iIdx = g_iIdxOfApInfo; iIdx < (MW_FIM_GP11_FIXED_SSID_PWD_NUM+MW_FIM_GP11_SSID_PWD_NUM); iIdx++)
            {
                int ret;
                if (g_apInfo[iIdx].tSsidPwd.used == 0)
                {
                    continue;
                }

                ret = BleWifi_Wifi_Get_BSsid(&(g_apInfo[iIdx]));

                if( 0 == ret )
                {
                    printf("\nFound Roaming APInfo[%d]=%s\n\n", iIdx, g_apInfo[iIdx].tSsidPwd.ssid);
                    g_iIdxOfApInfo = iIdx;

                    ret = Wifi_Roaming_Connect(g_apInfo[iIdx].bssid,  6, g_apInfo[iIdx].tSsidPwd.password, strlen((char *)g_apInfo[iIdx].tSsidPwd.password));
                    if(0 == ret)
                    {
                        return;
                    }
                }
                else
                {
                    printf("\nNot found Roaming APInfo[%d]=%s\n\n", iIdx, g_apInfo[iIdx].tSsidPwd.ssid);
                }
            }

            if (iIdx == (MW_FIM_GP11_FIXED_SSID_PWD_NUM+MW_FIM_GP11_SSID_PWD_NUM))
            {
                printf("\nNo Roaming AP in Scan list!\n\n");
                g_bRoaming_flag = false;

                BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_WIFI_DISCONNECTION_IND, NULL, 0);
            }
        }
    }
}
#else
static void BleWifi_Ctrl_TaskEvtHandler_WifiScanDoneInd(uint32_t evt_type, void *data, int len)
{
    uint8_t u8IsUpdate = false;
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_SCAN_DONE_IND \r\n");
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_SCANNING , false);

    // scan by auto-connect
    if (g_ubAppCtrlRequestRetryTimes == BLEWIFI_CTRL_AUTO_CONN_STATE_SCAN)
    {
        BleWifi_Wifi_UpdateScanInfoToAutoConnList(&u8IsUpdate);
        if(u8IsUpdate == true)
        {
            BleWifi_Wifi_DoAutoConnect();
        }
        else
        {
            printf("\nAuto connect list ap is not found.\n\n");
            BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_WIFI_DISCONNECTION_IND, NULL, 0);
        }

        g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE;
    }
    // scan by user
    else
    {
        BleWifi_Wifi_SendScanReport();
        BleWifi_Ble_SendResponse(BLEWIFI_RSP_SCAN_END, 0);
    }
}
#endif

static void BleWifi_Ctrl_TaskEvtHandler_WifiConnectionInd(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_CONNECTION_IND \r\n");
#ifdef __BLEWIFI_TRANSPARENT__
    msg_print_uart1("WIFI CONNECTED\n");
#endif
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTING , false);
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTED , true);

    // return to the idle of the connection retry
    g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE;
    g_ulAppCtrlAutoConnectInterval = BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_INIT;
    BleWifi_Ble_SendResponse(BLEWIFI_RSP_CONNECT, BLEWIFI_WIFI_CONNECTED_DONE);

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
    if( true == g_bConnectReqByUser_flag )
    {
        //save ssid and password to FIM
        wifi_scan_info_t tInfo;

        wifi_sta_get_ap_info(&tInfo);

        //printf("Save AP info:SSID:%s, PWD:%s\n", tInfo.ssid, wifi_config_req_connect.sta_config.password);
        T_MwFim_GP11_Ssid_PWD tNewSsidPwd;

        memset((void *)&tNewSsidPwd, 0, sizeof(T_MwFim_GP11_Ssid_PWD));
        memcpy((void *)tNewSsidPwd.ssid, (void *)tInfo.ssid, sizeof(tInfo.ssid));
        memcpy((void *)tNewSsidPwd.password, (void *)wifi_config_req_connect.sta_config.password, sizeof(wifi_config_req_connect.sta_config.password));

        SsidPwdAdd(tNewSsidPwd);

        g_bConnectReqByUser_flag = false;
    }
#endif
}

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
static void BleWifi_Ctrl_TaskEvtHandler_WifiDisconnectionInd(uint32_t evt_type, void *data, int len)
{
    int ret;

    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_DISCONNECTION_IND \r\n");
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTING, false);
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTED, false);
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_GOT_IP, false);
    BleWifi_Wifi_SetDTIM(0);

    wifi_scan_config_t scan_config = {0};

    scan_config.show_hidden = 1; // Enable to scan AP whose SSID is hidden
    scan_config.scan_type = WIFI_SCAN_TYPE_MIX;  // mixed mode

    if ( len > 0 )
    {
        uint8_t *reason = (uint8_t*)(data);

        printf("[%s,%d] WIFI DISCONNECT reason(%d)...\n", __FUNCTION__, __LINE__, *reason);

        if (WIFI_REASON_CODE_AUTO_CONNECT_FAILED == (*reason))
        {
            //printf("[%s,%d] auto connect fail...\n", __FUNCTION__, __LINE__);

            if (true == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED))
            {
                g_bAutoconn_flag = false;
                g_bRoaming_flag = false;
                return;
            }

            g_bAutoconn_flag = false;
            g_bRoaming_flag = true;

            //printf("\n[%s,%d]g_iIdxOfApInfo=%d, g_RoamingApInfoCnt=%d\n", __FUNCTION__, __LINE__, g_iIdxOfApInfo, g_RoamingApInfoTotalCnt);
            if (0 != g_RoamingApInfoTotalCnt)
            {
                g_iIdxOfApInfo = 0;

                ret = BleWifi_Wifi_DoScan(&scan_config);
                if (0 == ret)
                {
                    return;
                }
                else
                {
                    g_bRoaming_flag = false;
                }
            }
            else
            {
                g_bRoaming_flag = false;
            }
        }
    }

    // continue the connection retry
    if (g_ubAppCtrlRequestRetryTimes < BLEWIFI_WIFI_REQ_CONNECT_RETRY_TIMES)
    {
        if ((true == g_bRoaming_flag) && (true == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED)))
        {
            g_bRoaming_flag = false;
            g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE;
            return;
        }
        ret = BleWifi_Wifi_ReqConnectRetry();

        g_ubAppCtrlRequestRetryTimes++;
        return;
    }
    // stop the connection retry
    else if (g_ubAppCtrlRequestRetryTimes == BLEWIFI_WIFI_REQ_CONNECT_RETRY_TIMES)
    {
        // by roaming trigger
        if (true == g_bRoaming_flag)
        {
            g_iIdxOfApInfo++;
            if ( g_iIdxOfApInfo < (MW_FIM_GP11_FIXED_SSID_PWD_NUM+MW_FIM_GP11_SSID_PWD_NUM) )
            {
                if (false == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED))
                {
                    ret = BleWifi_Wifi_DoScan(&scan_config);
                    if(0 == ret)
                    {
                        return;
                    }
                }
            }

            g_bRoaming_flag = false;
        }
        // by user trigger
        else
        {
            g_bConnectReqByUser_flag = false;
            BleWifi_Ble_SendResponse(BLEWIFI_RSP_CONNECT, BLEWIFI_WIFI_CONNECTED_FAIL);
        }

        // return to the idle of the connection retry
        g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE;
        g_ulAppCtrlAutoConnectInterval = BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_INIT;

        /* do auto-connection. */
        if (false == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED))
        {
            if ((0 != BleWifi_Wifi_AutoConnectListNum()) || (g_RoamingApInfoTotalCnt != 0))
            {
                osTimerStop(g_tAppCtrlAutoConnectTriggerTimer);
                osTimerStart(g_tAppCtrlAutoConnectTriggerTimer, g_ulAppCtrlAutoConnectInterval);

                g_ulAppCtrlAutoConnectInterval = g_ulAppCtrlAutoConnectInterval + BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_DIFF;
                if (g_ulAppCtrlAutoConnectInterval > BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_MAX)
                    g_ulAppCtrlAutoConnectInterval = BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_MAX;
            }
        }
    }
    else
    {
        BleWifi_Ble_SendResponse(BLEWIFI_RSP_DISCONNECT, BLEWIFI_WIFI_DISCONNECTED_DONE);

        /* do auto-connection. */
        if (false == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED))
        {
            if ((0 != BleWifi_Wifi_AutoConnectListNum()) || (g_RoamingApInfoTotalCnt != 0))
            {
                osTimerStop(g_tAppCtrlAutoConnectTriggerTimer);
                osTimerStart(g_tAppCtrlAutoConnectTriggerTimer, g_ulAppCtrlAutoConnectInterval);

                g_ulAppCtrlAutoConnectInterval = g_ulAppCtrlAutoConnectInterval + BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_DIFF;
                if (g_ulAppCtrlAutoConnectInterval > BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_MAX)
                    g_ulAppCtrlAutoConnectInterval = BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_MAX;
            }
        }
    }
}
#else
static void BleWifi_Ctrl_TaskEvtHandler_WifiDisconnectionInd(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_DISCONNECTION_IND \r\n");
#ifdef __BLEWIFI_TRANSPARENT__
    msg_print_uart1("WIFI DISCONNECT\n");
#endif

    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTING , false);
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTED , false);
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_GOT_IP , false);
    BleWifi_Wifi_SetDTIM(0);

    // continue the connection retry
    if (g_ubAppCtrlRequestRetryTimes < BLEWIFI_WIFI_REQ_CONNECT_RETRY_TIMES)
    {
        BleWifi_Wifi_ReqConnectRetry();
        g_ubAppCtrlRequestRetryTimes++;
    }
    // stop the connection retry
    else if (g_ubAppCtrlRequestRetryTimes == BLEWIFI_WIFI_REQ_CONNECT_RETRY_TIMES)
    {
        // return to the idle of the connection retry
        g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE;
        g_ulAppCtrlAutoConnectInterval = BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_INIT;
        BleWifi_Ble_SendResponse(BLEWIFI_RSP_CONNECT, BLEWIFI_WIFI_CONNECTED_FAIL);

        /* do auto-connection. */
        if (false == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED))
        {
            osTimerStop(g_tAppCtrlAutoConnectTriggerTimer);
            osTimerStart(g_tAppCtrlAutoConnectTriggerTimer, g_ulAppCtrlAutoConnectInterval);

            g_ulAppCtrlAutoConnectInterval = g_ulAppCtrlAutoConnectInterval + BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_DIFF;
            if (g_ulAppCtrlAutoConnectInterval > BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_MAX)
                g_ulAppCtrlAutoConnectInterval = BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_MAX;
        }
    }
    else
    {
        BleWifi_Ble_SendResponse(BLEWIFI_RSP_DISCONNECT, BLEWIFI_WIFI_DISCONNECTED_DONE);

        /* do auto-connection. */
        if (false == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED))
        {
            osTimerStop(g_tAppCtrlAutoConnectTriggerTimer);
            osTimerStart(g_tAppCtrlAutoConnectTriggerTimer, g_ulAppCtrlAutoConnectInterval);

            g_ulAppCtrlAutoConnectInterval = g_ulAppCtrlAutoConnectInterval + BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_DIFF;
            if (g_ulAppCtrlAutoConnectInterval > BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_MAX)
                g_ulAppCtrlAutoConnectInterval = BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_MAX;

        }
    }
}
#endif

static void BleWifi_Ctrl_TaskEvtHandler_WifiGotIpInd(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_GOT_IP_IND \r\n");
#ifdef __BLEWIFI_TRANSPARENT__
    msg_print_uart1("WIFI GOT IP\n");
#endif

    BleWifi_Wifi_SendStatusInfo(BLEWIFI_IND_IP_STATUS_NOTIFY);
#if (SNTP_FUNCTION_EN == 1)
    BleWifi_SntpInit();
#endif
    BleWifi_Wifi_UpdateBeaconInfo();
    BleWifi_Wifi_SetDTIM(BleWifi_Ctrl_DtimTimeGet());

    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_GOT_IP , true);
}

static void BleWifi_Ctrl_TaskEvtHandler_WifiAutoConnectInd(uint32_t evt_type, void *data, int len)
{
#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
        g_bAutoconn_flag = true;
        g_bRoaming_flag = false;
#endif
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_WIFI_AUTO_CONNECT_IND \r\n");
    BleWifi_Ctrl_DoAutoConnect();
}

static void BleWifi_Ctrl_TaskEvtHandler_OtherOtaOn(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_OTHER_OTA_ON \r\n");
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_OTA , true);
}

static void BleWifi_Ctrl_TaskEvtHandler_OtherOtaOff(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_OTHER_OTA_OFF \r\n");
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_OTA , false);
    msg_print_uart1("OK\r\n");

    // restart the system
    osDelay(BLEWIFI_CTRL_RESET_DELAY);
    Hal_Sys_SwResetAll();
}

static void BleWifi_Ctrl_TaskEvtHandler_OtherOtaOffFail(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_OTHER_OTA_OFF_FAIL \r\n");
    BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_OTA , false);
    msg_print_uart1("ERROR\r\n");
}

#if (BLEWIFI_CTRL_BUTTON_SENSOR_EN == 1)
static void BleWifi_Ctrl_TaskEvtHandler_ButtonStateChange(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BUTTON_STATE_CHANGE \r\n");

    /* Start timer to debounce */
    osTimerStop(g_tAppButtonDebounceTimerId);
    osTimerStart(g_tAppButtonDebounceTimerId, BLEWIFI_CTRL_BUTTON_TIMEOUT_DEBOUNCE_TIME);
}

static void BleWifi_Ctrl_TaskEvtHandler_ButtonDebounceTimeOut(uint32_t evt_type, void *data, int len)
{
    uint32_t u32PinLevel = 0;

    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_BUTTON_DEBOUNCE_TIMEOUT \r\n");

    // Get the status of GPIO (Low / High)
    u32PinLevel = Hal_Vic_GpioInput(BLEWIFI_CTRL_BUTTON_IO_PORT);
    BLEWIFI_INFO("BLEWIFI_CTRL_BUTTON_IO_PORT pin level = %s\r\n", u32PinLevel ? "GPIO_LEVEL_HIGH" : "GPIO_LEVEL_LOW");

    if(GPIO_LEVEL_LOW == u32PinLevel)
    {
        /* Disable - Invert gpio interrupt signal */
        Hal_Vic_GpioIntInv(BLEWIFI_CTRL_BUTTON_IO_PORT, 0);
        // Enable Interrupt
        Hal_Vic_GpioIntEn(BLEWIFI_CTRL_BUTTON_IO_PORT, 1);
    }
    else
    {
        /* Enable - Invert gpio interrupt signal */
        Hal_Vic_GpioIntInv(BLEWIFI_CTRL_BUTTON_IO_PORT, 1);
        // Enable Interrupt
        Hal_Vic_GpioIntEn(BLEWIFI_CTRL_BUTTON_IO_PORT, 1);
    }

    if(BLEWIFI_CTRL_BUTTON_PRESS_LEVEL == u32PinLevel)
    {
        BleWifi_ButtonFsm_Run(BLEWIFI_BUTTON_EVENT_PRESS);
    }
    else
    {
        BleWifi_ButtonFsm_Run(BLEWIFI_BUTTON_EVENT_RELEASE);
    }
}

static void BleWifi_Ctrl_TaskEvtHandler_ButtonReleaseTimeOut(uint32_t evt_type, void *data, int len)
{
    // used in short/long press
    BleWifi_ButtonFsm_Run(BLEWIFI_BUTTON_EVENT_TIMEOUT);
}

void BleWifi_Ctrl_ButtonReleaseHandle(uint8_t u8ReleaseCount)
{
    if(u8ReleaseCount == 1)
    {
        BLEWIFI_INFO("[%s %d] release once\n", __func__, __LINE__);
    }
    else if(u8ReleaseCount >= 2)
    {
        BLEWIFI_INFO("[%s %d] release %u times\n", __func__, __LINE__, u8ReleaseCount);
    }
}
#endif
#if (BLEWIFI_CTRL_WAKEUP_IO_EN == 1)
static void BleWifi_Ctrl_TaskEvtHandler_PsSmartStateChange(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_PS_SMART_STATE_CHANGE \r\n");

    /* Start timer to debounce */
    osTimerStop(g_tAppPsSmartDebounceTimerId);
    osTimerStart(g_tAppPsSmartDebounceTimerId, BLEWIFI_CTRL_WAKEUP_IO_TIMEOUT_DEBOUNCE_TIME);

}
static void BleWifi_Ctrl_TaskEvtHandler_PsSmartDebounceTimeOut(uint32_t evt_type, void *data, int len)
{
    uint32_t u32PinLevel = 0;
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_PS_PMART_DEBOUNCE_TIMEOUT \r\n");

    u32PinLevel = Hal_Vic_GpioInput(BLEWIFI_CTRL_WAKEUP_IO_PORT);
    if(BLEWIFI_CTRL_WAKEUP_IO_LEVEL == u32PinLevel)
    {
        BLEWIFI_INFO("Ps_Smart_Off_callback, Wakeup !!!!!!\r\n");
        ps_smart_sleep(0);
    }
    else
    {
        /* Power saving settings */
        BLEWIFI_INFO("Ps_Smart_Off_callback, Sleep !!!!!!\r\n");
        ps_smart_sleep(BLEWIFI_COM_POWER_SAVE_EN);
    }
    BleWifi_Ps_Smart_Pin_Config(BLEWIFI_CTRL_WAKEUP_IO_PORT, u32PinLevel);
}
#endif
static void BleWifi_Ctrl_TaskEvtHandler_OtherSysTimer(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("BLEWIFI: MSG BLEWIFI_CTRL_MSG_OTHER_SYS_TIMER \r\n");
    BleWifi_Ctrl_SysStatusChange();
}

void BleWifi_Ctrl_NetworkingStart(uint32_t u32ExpireTime)
{
    if (false == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_NETWORK))
    {
        BLEWIFI_INFO("[%s %d] start\n", __func__, __LINE__);

        BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_NETWORK , true);
        BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_ADVTISING , true);

        BleWifi_Ble_AdvertisingTimeChange(BLEWIFI_BLE_ADVERTISEMENT_INTERVAL_MIN, BLEWIFI_BLE_ADVERTISEMENT_INTERVAL_MAX);

        osTimerStop(g_tAppCtrlNetworkTimerId);
        if ( u32ExpireTime > 0 ){
            osTimerStart(g_tAppCtrlNetworkTimerId, u32ExpireTime);
        }
    }
    else
    {
        BLEWIFI_WARN("[%s %d] BLEWIFI_CTRL_EVENT_BIT_NETWORK already true\n", __func__, __LINE__);
    }
}

void BleWifi_Ctrl_NetworkingStop(void)
{
    if (true == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_NETWORK))
    {
        BLEWIFI_INFO("[%s %d] start\n", __func__, __LINE__);

        osTimerStop(g_tAppCtrlNetworkTimerId);
        BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_NETWORK , false);
        BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_ADVTISING , false);

        // stop adv or disconnect
        if(false == BleWifi_EventStatusGet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED))
        {
            BleWifi_Ble_AdvertisingTimeChange(BLEWIFI_BLE_ADVERTISEMENT_INTERVAL_PS_MIN, BLEWIFI_BLE_ADVERTISEMENT_INTERVAL_PS_MAX);
        }
        else
        {
            //BleWifi_Ble_Disconnect();
            BleWifi_Ble_AdvertisingTimeChange(BLEWIFI_BLE_ADVERTISEMENT_INTERVAL_PS_MIN, BLEWIFI_BLE_ADVERTISEMENT_INTERVAL_PS_MAX);
        }
    }
    else
    {
        BLEWIFI_WARN("[%s %d] BLEWIFI_CTRL_EVENT_BIT_NETWORK already false\n", __func__, __LINE__);
    }
}

void BleWifi_Ctrl_NetworkingParamChange(uint32_t u32IntervalTime)
{
    BleWifi_Ble_AdvertisingTimeChange(u32IntervalTime, u32IntervalTime);
}

void BleWifi_Ctrl_NetworkingTimeout(void const *argu)
{
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_NETWORKING_STOP, NULL, 0);
}

#ifdef __BLEWIFI_TRANSPARENT__
int BleWifi_Ctrl_BleCastWithExpire(uint8_t u8BleCastEnable, uint32_t u32ExpireTime){
    if ( u8BleCastEnable != 0 && u8BleCastEnable != 1) {
        return -1;
    }
    if ( ( 0 < u32ExpireTime && u32ExpireTime < 1000 ) || u32ExpireTime > 3600000) {
        return -1;
    }

    if ( u8BleCastEnable == 1 ) {
        BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_NETWORKING_START, (void *)&u32ExpireTime, sizeof(u32ExpireTime));
    }
    else {
        BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_NETWORKING_STOP, NULL, 0);
    }
    return 0;
}

int BleWifi_Ctrl_BleCastParamSet(uint32_t u32CastInteval){
    if ( u32CastInteval < 32 || u32CastInteval > 10240) {
        return -1;
    }
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_NETWORKING_PARAM_CHANGE, (void *)&u32CastInteval, sizeof(u32CastInteval));

    return 0;
}
#endif

static void BleWifi_Ctrl_TaskEvtHandler_NetworkingStart(uint32_t evt_type, void *data, int len)
{
    uint32_t *fpExpireTime;

    fpExpireTime = (uint32_t *)data;
    BleWifi_Ctrl_NetworkingStart(*fpExpireTime);
}

static void BleWifi_Ctrl_TaskEvtHandler_NetworkingStop(uint32_t evt_type, void *data, int len)
{
    BleWifi_Ctrl_NetworkingStop();
}

static void BleWifi_Ctrl_TaskEvtHandler_NetworkingParamChange(uint32_t evt_type, void *data, int len)
{
    if ( data != NULL ) {
        BleWifi_Ctrl_NetworkingParamChange(*((uint32_t *)data));
    }
}

void BleWifi_Ctrl_TaskEvtHandler(uint32_t evt_type, void *data, int len)
{
    uint32_t i = 0;

    while (g_tCtrlEvtHandlerTbl[i].ulEventId != 0xFFFFFFFF)
    {
        // match
        if (g_tCtrlEvtHandlerTbl[i].ulEventId == evt_type)
        {
            g_tCtrlEvtHandlerTbl[i].fpFunc(evt_type, data, len);
            break;
        }

        i++;
    }

    // not match
    if (g_tCtrlEvtHandlerTbl[i].ulEventId == 0xFFFFFFFF)
    {
    }
}

void BleWifi_Ctrl_Task(void *args)
{
    osEvent rxEvent;
    xBleWifiCtrlMessage_t *rxMsg;

    for(;;)
    {
        /* Wait event */
        rxEvent = osMessageGet(g_tAppCtrlQueueId, osWaitForever);
        if(rxEvent.status != osEventMessage)
            continue;

        rxMsg = (xBleWifiCtrlMessage_t *)rxEvent.value.p;
        BleWifi_Ctrl_TaskEvtHandler(rxMsg->event, rxMsg->ucaMessage, rxMsg->length);

        /* Release buffer */
        if (rxMsg != NULL)
            free(rxMsg);
    }
}

int BleWifi_Ctrl_MsgSend(int msg_type, uint8_t *data, int data_len)
{
    xBleWifiCtrlMessage_t *pMsg = 0;

	if (NULL == g_tAppCtrlQueueId)
	{
        BLEWIFI_ERROR("BLEWIFI: No queue \r\n");
        return -1;
	}

    /* Mem allocate */
    pMsg = malloc(sizeof(xBleWifiCtrlMessage_t) + data_len);
    if (pMsg == NULL)
	{
        BLEWIFI_ERROR("BLEWIFI: ctrl task pmsg allocate fail \r\n");
	    goto error;
    }

    pMsg->event = msg_type;
    pMsg->length = data_len;
    if (data_len > 0)
    {
        memcpy(pMsg->ucaMessage, data, data_len);
    }

    if (osMessagePut(g_tAppCtrlQueueId, (uint32_t)pMsg, 0) != osOK)
    {
        printf("BLEWIFI: ctrl task message send fail \r\n");
        goto error;
    }

    return 0;

error:
	if (pMsg != NULL)
	{
	    free(pMsg);
    }

	return -1;
}

void BleWifi_Ctrl_Init(void)
{
    osThreadDef_t task_def;
    osMessageQDef_t blewifi_queue_def;
    osTimerDef_t timer_auto_connect_def;
    osTimerDef_t timer_sys_def;
    osTimerDef_t timer_network_def;

    /* Create ble-wifi task */
    task_def.name = "blewifi ctrl";
    task_def.stacksize = OS_TASK_STACK_SIZE_BLEWIFI_CTRL;
    task_def.tpriority = OS_TASK_PRIORITY_APP;
    task_def.pthread = BleWifi_Ctrl_Task;
    g_tAppCtrlTaskId = osThreadCreate(&task_def, (void*)NULL);
    if(g_tAppCtrlTaskId == NULL)
    {
        BLEWIFI_INFO("BLEWIFI: ctrl task create fail \r\n");
    }
    else
    {
        BLEWIFI_INFO("BLEWIFI: ctrl task create successful \r\n");
    }

    /* Create message queue*/
    blewifi_queue_def.item_sz = sizeof(xBleWifiCtrlMessage_t);
    blewifi_queue_def.queue_sz = BLEWIFI_CTRL_QUEUE_SIZE;
    g_tAppCtrlQueueId = osMessageCreate(&blewifi_queue_def, NULL);
    if(g_tAppCtrlQueueId == NULL)
    {
        BLEWIFI_ERROR("BLEWIFI: ctrl task create queue fail \r\n");
    }

    /* create timer to trig auto connect */
    timer_auto_connect_def.ptimer = BleWifi_Ctrl_AutoConnectTrigger;
    g_tAppCtrlAutoConnectTriggerTimer = osTimerCreate(&timer_auto_connect_def, osTimerOnce, NULL);
    if(g_tAppCtrlAutoConnectTriggerTimer == NULL)
    {
        BLEWIFI_ERROR("BLEWIFI: ctrl task create auto-connection timer fail \r\n");
    }

    /* create timer to trig the sys state */
    timer_sys_def.ptimer = BleWifi_Ctrl_SysTimeout;
    g_tAppCtrlSysTimer = osTimerCreate(&timer_sys_def, osTimerOnce, NULL);
    if(g_tAppCtrlSysTimer == NULL)
    {
        BLEWIFI_ERROR("BLEWIFI: ctrl task create SYS timer fail \r\n");
    }

    /* create timer to trig the ble adv timeout */
    timer_network_def.ptimer = BleWifi_Ctrl_NetworkingTimeout;
    g_tAppCtrlNetworkTimerId = osTimerCreate(&timer_network_def, osTimerOnce, NULL);
    if(g_tAppCtrlNetworkTimerId == NULL)
    {
        BLEWIFI_ERROR("BLEWIFI: ctrl task create Network timeout timer fail \r\n");
    }

    /* Create the event group */
    if(false == BleWifi_EventCreate(&g_tAppCtrlEventGroup))
    {
        BLEWIFI_ERROR("BLEWIFI: create event group fail \r\n");
    }

    /* the init state of system mode is init */
    g_ulAppCtrlSysMode = MW_FIM_SYS_MODE_INIT;

    /* the init state of SYS is init */
    g_ubAppCtrlSysStatus = BLEWIFI_CTRL_SYS_INIT;
    // start the sys timer
    osTimerStop(g_tAppCtrlSysTimer);
    osTimerStart(g_tAppCtrlSysTimer, BLEWIFI_COM_SYS_TIME_INIT);

    /* Init the DTIM time (ms) */
    g_ulAppCtrlWifiDtimTime = BLEWIFI_WIFI_DTIM_INTERVAL;

    // the idle of the connection retry
    g_ubAppCtrlRequestRetryTimes = BLEWIFI_CTRL_AUTO_CONN_STATE_IDLE;
    g_ulAppCtrlAutoConnectInterval = BLEWIFI_WIFI_AUTO_CONNECT_INTERVAL_INIT;
#if (BLEWIFI_CTRL_BUTTON_SENSOR_EN == 1)
    BleWifi_ButtonPress_Init();
#endif
}
