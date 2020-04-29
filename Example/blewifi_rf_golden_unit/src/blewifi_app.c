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
 * @file blewifi_app.c
 * @author Vincent Chen
 * @date 12 Feb 2018
 * @brief File creates the wifible app task architecture.
 *
 */
#include "blewifi_configuration.h"
#include "blewifi_common.h"
#include "blewifi_app.h"
#include "blewifi_wifi_api.h"
#include "blewifi_ble_api.h"
#include "blewifi_ctrl.h"
#include "blewifi_ctrl_http_ota.h"
#include "iot_data.h"
#include "sys_common_api.h"
#include "ps_public.h"
#include "mw_fim_default_group03.h"
#include "mw_fim_default_group03_patch.h"
#include "mw_fim_default_group11_project.h"
#include "mw_fim_default_group14_project.h"
#include "app_at_cmd.h"
#include "blewifi_auto_at_cmd.h"

blewifi_ota_t *gTheOta = 0;

volatile T_MwFim_GP14_Boot_Status g_tBootStatus = {0};

void BleWifi_BootCntUpdate(void)
{
    if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP14_PROJECT_BOOT_STATUS, 0, MW_FIM_GP14_BOOT_STATUS_SIZE, (uint8_t *)&g_tBootStatus))
    {
        BLEWIFI_ERROR("[%s][%d] MwFim_FileRead fail\n", __func__, __LINE__);
        // if fail, get the default value
        memcpy((void *)&g_tBootStatus, &g_tMwFimDefaultGp14BootStatus, MW_FIM_GP14_BOOT_STATUS_SIZE);
    }
    BLEWIFI_INFO("[%s][%d] read boot_cnt[%u]\n", __func__, __LINE__, g_tBootStatus.u8Cnt);
    
    if(g_tBootStatus.u8Cnt < BLEWIFI_CTRL_BOOT_CNT_FOR_RESET)
    {
        g_tBootStatus.u8Cnt += 1;
        if (MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP14_PROJECT_BOOT_STATUS, 0, MW_FIM_GP14_BOOT_STATUS_SIZE, (uint8_t *)&g_tBootStatus))
        {
            BLEWIFI_ERROR("[%s][%d] MwFim_FileWrite fail\n", __func__, __LINE__);
        }
    }
    BLEWIFI_WARN("[%s][%d] current boot_cnt[%u]\n", __func__, __LINE__, g_tBootStatus.u8Cnt);
    
    return;
}

void BleWifiAppInit(void)
{
    T_MwFim_SysMode tSysMode;
    
	gTheOta = 0;

#if (SNTP_FUNCTION_EN == 1)
    g_ulSntpSecondInit = SNTP_SEC_2019;     // Initialize the Sntp Value
    g_ulSystemSecondInit = 0;               // Initialize System Clock Time
#endif

    BleWifi_BootCntUpdate();

    if(g_tBootStatus.u8Cnt < BLEWIFI_CTRL_BOOT_CNT_FOR_RESET)
    {
        ATCmdProc(); 
    }
    else
    {
        //clear boot cnt
        g_tBootStatus.u8Cnt = 0;
        if (MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP14_PROJECT_BOOT_STATUS, 0, MW_FIM_GP14_BOOT_STATUS_SIZE, (uint8_t *)&g_tBootStatus))
        {
            BLEWIFI_ERROR("[%s][%d] MwFim_FileWrite fail\n", __func__, __LINE__);
        }
        else
        {
            BLEWIFI_WARN("[%s][%d] clear boot_cnt\n", __func__, __LINE__);
        }

        // get the settings of system mode
    	if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP03_PATCH_SYS_MODE, 0, MW_FIM_SYS_MODE_SIZE, (uint8_t*)&tSysMode))
        {
            // if fail, get the default value
            memcpy(&tSysMode, &g_tMwFimDefaultSysMode, MW_FIM_SYS_MODE_SIZE);
        }

        // only for the user mode
        if ((tSysMode.ubSysMode == MW_FIM_SYS_MODE_INIT) || (tSysMode.ubSysMode == MW_FIM_SYS_MODE_USER))
        {
            /* Wi-Fi Initialization */
            BleWifi_Wifi_Init();

            /* BLE Stack Initialization */
            BleWifi_Ble_Init();

            /* blewifi "control" task Initialization */
            BleWifi_Ctrl_Init();

            /* blewifi HTTP OTA */
            #if (WIFI_OTA_FUNCTION_EN == 1)
            blewifi_ctrl_http_ota_task_create();
            #endif

            /* IoT device Initialization */
            #if (IOT_DEVICE_DATA_TX_EN == 1) || (IOT_DEVICE_DATA_RX_EN == 1)
            Iot_Data_Init();
            #endif

            // move the settings to blewifi_ctrl, when the sys status is changed from Init to Noraml
            /* Power saving settings */
            //if (tSysMode.ubSysMode == MW_FIM_SYS_MODE_USER)
            //    ps_smart_sleep(BLEWIFI_COM_POWER_SAVE_EN);
            
            /* RF Power settings */
            BleWifi_RFPowerSetting(BLEWIFI_COM_RF_POWER_SETTINGS);
            
            //set TCA mode
#if (BLEWIFI_WIFI_SET_TCA_MODE == 1)
            sys_set_rf_temp_cal_mode(true);
#endif
        }

        // update the system mode
        BleWifi_Ctrl_SysModeSet(tSysMode.ubSysMode);

        // add app cmd
        app_at_cmd_add(); 
    }

}