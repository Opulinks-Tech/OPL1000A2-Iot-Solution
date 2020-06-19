/******************************************************************************
*  Copyright 2017 - 2018, Opulinks Technology Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2018
******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "at_cmd.h"
#include "at_cmd_common.h"
#include "at_cmd_data_process.h"
#include "hal_flash.h"
#include "at_cmd_task.h"
#include "at_cmd_func_patch.h"
#include "blewifi_configuration.h"
#include "blewifi_ctrl.h"
#include "blewifi_data.h"
#ifdef __BLEWIFI_TRANSPARENT__
#include "blewifi_server_app.h"
#endif

#include "agent_patch.h"
#include "mw_fim.h"
#include "at_cmd_data_process_patch.h"
#include "hal_flash_internal.h"
#include "hal_spi.h"
#include "svn_info.h"  //execute "pre-build.bat" to generate "svn_info.h"
#include "mw_ota.h"
#include "mw_ota_def.h"

//#define AT_LOG                      msg_print_uart1
#define AT_LOG(...)

/* For at_cmd_sys_write_fim */
#define AT_FIM_DATA_LENGTH 2 /* EX: 2 = FF */
#define AT_FIM_DATA_LENGTH_WITH_COMMA (AT_FIM_DATA_LENGTH + 1) /* EX: 3 = FF, */

extern RET_DATA int8_t g_bMwOtaImageIdx;
extern RET_DATA T_Hal_Flash_AddrRead g_tMwOtaReadFunc;
extern RET_DATA T_Hal_Flash_4KSectorAddrErase g_tMwOtaEraseFunc;
extern RET_DATA T_MwOtaLayoutInfo g_tMwOtaLayoutInfo;          // the layout information


typedef struct
{
    uint32_t u32Id;
    uint16_t u16Index;
    uint16_t u16DataTotalLen;

    uint32_t u32DataRecv;       // Calcuate the receive data
    uint32_t TotalSize;         // user need to input total bytes

    char     u8aReadBuf[8];
    uint8_t  *ResultBuf;
    uint32_t u32StringIndex;       // Indicate the location of reading string
    uint16_t u16Resultindex;       // Indicate the location of result string
    uint8_t  fIgnoreRestString;    // Set a flag for last one comma
    uint8_t  u8aTemp[1];
} T_AtFimParam;

int app_at_cmd_sys_read_fim(char *buf, int len, int mode)
{
    int iRet = 0;
    int argc = 0;
    char *argv[AT_MAX_CMD_ARGS] = {0};
    uint32_t i = 0;
    uint8_t *readBuf = NULL;
    uint32_t u32Id  = 0;
    uint16_t u16Index  = 0;
    uint16_t u16Size  = 0;

    if(!at_cmd_buf_to_argc_argv(buf, &argc, argv, AT_MAX_CMD_ARGS))
    {
        goto done;
    }

    if(argc != 4)
    {
        AT_LOG("invalid param number\r\n");
        goto done;
    }

    u32Id  = (uint32_t)strtoul(argv[1], NULL, 16);
    u16Index  = (uint16_t)strtoul(argv[2], NULL, 0);
    u16Size  = (uint16_t)strtoul(argv[3], NULL, 0);

    if((u16Size == 0) )
    {
        AT_LOG("invalid size[%d]\r\n", u16Size);
        goto done;
    }

    switch(mode)
    {
        case AT_CMD_MODE_SET:
        {
            readBuf = (uint8_t *)malloc(u16Size);

            if(MW_FIM_OK == MwFim_FileRead(u32Id, u16Index, u16Size, readBuf))
            {
                msg_print_uart1("%02X",readBuf[0]);
                for(i = 1 ; i < u16Size ; i++)
                {
                    msg_print_uart1(",%02X",readBuf[i]);
                }
            }
            else
            {
                goto done;
            }

            msg_print_uart1("\r\n");
            break;
        }

        default:
            goto done;
    }

    iRet = 1;
done:
    if(iRet)
    {
        msg_print_uart1("OK\r\n");
    }
    else
    {
        msg_print_uart1("ERROR\r\n");
    }

    if(readBuf != NULL)
        free(readBuf);
    return iRet;
}

int write_fim_handle(uint32_t u32Type, uint8_t *u8aData, uint32_t u32DataLen, void *pParam)
{
    T_AtFimParam *ptParam = (T_AtFimParam *)pParam;

    uint8_t  iRet = 0;
    uint8_t  u8acmp[] = ",\0";
    uint32_t i = 0;

    ptParam->u32DataRecv += u32DataLen;

    /* If previous segment is error then ignore the rest of segment */
    if(ptParam->fIgnoreRestString)
    {
        goto done;
    }

    for(i = 0 ; i < u32DataLen ; i++)
    {
        if(u8aData[i] != u8acmp[0])
        {
            if(ptParam->u32StringIndex >= AT_FIM_DATA_LENGTH)
            {
                ptParam->fIgnoreRestString = 1;
                goto done;
            }

            /* compare string. If not comma then store into array. */
            ptParam->u8aReadBuf[ptParam->u32StringIndex] = u8aData[i];
            ptParam->u32StringIndex++;
        }
        else
        {
            /* Convert string into Hex and store into array */
            ptParam->ResultBuf[ptParam->u16Resultindex] = (uint8_t)strtoul(ptParam->u8aReadBuf, NULL, 16);

            /* Result index add one */
            ptParam->u16Resultindex++;

            /* re-count when encounter comma */
            ptParam->u32StringIndex=0;
        }
    }

    /* If encounter the last one comma
       1. AT_FIM_DATA_LENGTH:
       Max character will pick up to compare.

       2. (ptParam->u16DataTotalLen - 1):
       If total length minus 1 is equal (ptParam->u16Resultindex) mean there is no comma at the rest of string.
    */
    if((ptParam->u16Resultindex == (ptParam->u16DataTotalLen - 1)) && (ptParam->u32StringIndex >= AT_FIM_DATA_LENGTH))
    {
        ptParam->ResultBuf[ptParam->u16Resultindex] = (uint8_t)strtoul(ptParam->u8aReadBuf, NULL, 16);

        /* Result index add one */
        ptParam->u16Resultindex++;
    }

    /* Collect array data is equal to total lengh then write data to fim. */
    if(ptParam->u16Resultindex == ptParam->u16DataTotalLen)
    {
       	if(MW_FIM_OK == MwFim_FileWrite(ptParam->u32Id, ptParam->u16Index, ptParam->u16DataTotalLen, ptParam->ResultBuf))
        {
            msg_print_uart1("OK\r\n");
        }
        else
        {
            ptParam->fIgnoreRestString = 1;
        }
    }
    else
    {
        goto done;
    }

done:
    if(ptParam->TotalSize >= ptParam->u32DataRecv)
    {
        if(ptParam->fIgnoreRestString)
        {
            msg_print_uart1("ERROR\r\n");
        }

        if(ptParam != NULL)
        {
            if (ptParam->ResultBuf != NULL)
            {
                free(ptParam->ResultBuf);
            }
            free(ptParam);
            ptParam = NULL;
        }
    }

    return iRet;
}

int app_at_cmd_sys_write_fim(char *buf, int len, int mode)
{
    int iRet = 0;
    int argc = 0;
    char *argv[AT_MAX_CMD_ARGS] = {0};

    /* Initialization the value */
    T_AtFimParam *tAtFimParam = (T_AtFimParam*)malloc(sizeof(T_AtFimParam));
    if(tAtFimParam == NULL)
    {
        goto done;
    }
    memset(tAtFimParam, 0, sizeof(T_AtFimParam));

    if(!at_cmd_buf_to_argc_argv(buf, &argc, argv, AT_MAX_CMD_ARGS))
    {
        goto done;
    }

    if(argc != 4)
    {
        msg_print_uart1("invalid param number\r\n");
        goto done;
    }

    /* save parameters to process uart1 input */
    tAtFimParam->u32Id = (uint32_t)strtoul(argv[1], NULL, 16);
    tAtFimParam->u16Index = (uint16_t)strtoul(argv[2], NULL, 0);
    tAtFimParam->u16DataTotalLen = (uint16_t)strtoul(argv[3], NULL, 0);

    /* If user input data length is 0 then go to error.*/
    if(tAtFimParam->u16DataTotalLen == 0)
    {
        goto done;
    }

    switch(mode)
    {
        case AT_CMD_MODE_SET:
        {
            tAtFimParam->TotalSize = ((tAtFimParam->u16DataTotalLen * AT_FIM_DATA_LENGTH_WITH_COMMA) - 1);

            /* Memory allocate a memory block for pointer */
            tAtFimParam->ResultBuf = (uint8_t *)malloc(tAtFimParam->u16DataTotalLen);
            if(tAtFimParam->ResultBuf == NULL)
                goto done;

            // register callback to process uart1 input
            agent_data_handle_reg(write_fim_handle, tAtFimParam);

            // redirect uart1 input to callback
            data_process_lock_patch(LOCK_OTHERS, (tAtFimParam->TotalSize));

            break;
        }

        default:
            goto done;
    }

    iRet = 1;

done:
    if(iRet)
    {
        msg_print_uart1("OK\r\n");
    }
    else
    {
        msg_print_uart1("ERROR\r\n");
        if (tAtFimParam != NULL)
        {
            if (tAtFimParam->ResultBuf != NULL)
            {
		        free(tAtFimParam->ResultBuf);
            }
            free(tAtFimParam);
            tAtFimParam = NULL;
        }
    }

    return iRet;
}

int app_at_cmd_sys_dtim_time(char *buf, int len, int mode)
{
    int iRet = 0;
    int argc = 0;
    char *argv[AT_MAX_CMD_ARGS] = {0};

    int32_t s32DtimTime;

    if (!at_cmd_buf_to_argc_argv(buf, &argc, argv, AT_MAX_CMD_ARGS))
    {
        goto done;
    }

    switch (mode)
    {
        case AT_CMD_MODE_READ:
        {
            msg_print_uart1("DTIM Time: %d\r\n", BleWifi_Ctrl_DtimTimeGet());
            break;
        }

        case AT_CMD_MODE_SET:
        {
            // at+dtim=<value>
            if(argc != 2)
            {
                AT_LOG("invalid param number\r\n");
                goto done;
            }

            s32DtimTime = strtol(argv[1], NULL, 0);

            if ( s32DtimTime < 0 )
            {
                AT_LOG("invalid param value\r\n");
                goto done;
            }
            BleWifi_Ctrl_DtimTimeSet((uint32_t)s32DtimTime);
            break;
        }

        default:
            goto done;
    }

    iRet = 1;

done:
    if(iRet)
    {
        msg_print_uart1("OK\r\n");
    }
    else
    {
        msg_print_uart1("ERROR\r\n");
    }

    return iRet;
}

int app_at_cmd_sys_main_fw_version(char *buf, int len, int mode)
{
    int iRet = 0;
    int argc = 0;
    char *argv[AT_MAX_CMD_ARGS] = {0};

    T_MwOtaFlashHeader taHeader;

    if (!at_cmd_buf_to_argc_argv(buf, &argc, argv, AT_MAX_CMD_ARGS))
    {
        goto done;
    }

    switch (mode)
    {
        case AT_CMD_MODE_READ:
        {
            if(1 == g_bMwOtaImageIdx)
            {
                if (0 != g_tMwOtaReadFunc(SPI_IDX_0, g_tMwOtaLayoutInfo.ulaHeaderAddr[0], 0, sizeof(T_MwOtaFlashHeader), (uint8_t*)&taHeader))
                {
                    printf("To read the 1st header of MW_OTA is fail.\n");
                    return MW_OTA_FAIL;
                }
            }
            else
            {
                if (0 != g_tMwOtaReadFunc(SPI_IDX_0, g_tMwOtaLayoutInfo.ulaHeaderAddr[1], 0, sizeof(T_MwOtaFlashHeader), (uint8_t*)&taHeader))
                {
                    printf("To read the 2nd header of MW_OTA is fail.\n");
                    return MW_OTA_FAIL;
                }
            }
            msg_print_uart1("ProjectId : = %u\r\n",taHeader.uwProjectId);
            msg_print_uart1("ChipId    : = %u\r\n",taHeader.uwChipId);
            msg_print_uart1("FwId      : = %u\r\n",taHeader.uwFirmwareId);

            break;
        }
        default:
            goto done;
    }

    iRet = 1;

done:
    if(iRet)
    {
        msg_print_uart1("OK\r\n");
    }
    else
    {
        msg_print_uart1("ERROR\r\n");
    }

    return iRet;
}

int app_at_cmd_sys_erase_mp_fw(char *buf, int len, int mode)
{
    int iRet = 0;
    int argc = 0;
    char *argv[AT_MAX_CMD_ARGS] = {0};

    if (!at_cmd_buf_to_argc_argv(buf, &argc, argv, AT_MAX_CMD_ARGS))
    {
        goto done;
    }

    switch (mode)
    {
        case AT_CMD_MODE_EXECUTION:
        {
            if (0 != g_tMwOtaEraseFunc(SPI_IDX_0, g_tMwOtaLayoutInfo.ulaHeaderAddr[g_bMwOtaImageIdx]))
            {
                printf("To erase the header [0x%x] of MW_OTA is fail.\n", g_tMwOtaLayoutInfo.ulaHeaderAddr[g_bMwOtaImageIdx]);
                goto done;
            }
            //msg_print_uart1("erase the header [0x%x] is successful\n",g_tMwOtaLayoutInfo.ulaHeaderAddr[g_bMwOtaImageIdx]);

            break;
        }
        default:
            goto done;
    }

    iRet = 1;

done:
    if(iRet)
    {
        msg_print_uart1("OK\r\n");
    }
    else
    {
        msg_print_uart1("ERROR\r\n");
    }

    return iRet;
}


int app_at_cmd_sys_mp_version(char *buf, int len, int mode)
{
    int iRet = 0;
    int argc = 0;
    char *argv[AT_MAX_CMD_ARGS] = {0};

    if (!at_cmd_buf_to_argc_argv(buf, &argc, argv, AT_MAX_CMD_ARGS))
    {
        goto done;
    }

    switch (mode)
    {
        case AT_CMD_MODE_READ:
        {
            msg_print_uart1("mp version: %u\r\n", SVN_REVISION);
            break;
        }
        default:
            goto done;
    }

    iRet = 1;

done:
    if(iRet)
    {
        msg_print_uart1("OK\r\n");
    }
    else
    {
        msg_print_uart1("ERROR\r\n");
    }

    return iRet;
}

int app_at_cmd_sys_FlashID(char *buf, int len, int mode)
{
    int iRet = 0;
    int argc = 0;
    char *argv[AT_MAX_CMD_ARGS] = {0};

    uint32_t u32Manufacturer = 0 , u32MemoryType = 0 , u32MemoryDensity = 0;

    if (!at_cmd_buf_to_argc_argv(buf, &argc, argv, AT_MAX_CMD_ARGS))
    {
        goto done;
    }

    switch (mode)
    {
        case AT_CMD_MODE_READ:
        {
            _Hal_Flash_ManufDeviceId(SPI_IDX_0, &u32Manufacturer, &u32MemoryType, &u32MemoryDensity);
            msg_print_uart1("Manufacturer: %u\r\n", u32Manufacturer);
            msg_print_uart1("MemoryType: %u\r\n", u32MemoryType);
            msg_print_uart1("MemoryDensity: %u\r\n", u32MemoryDensity);
            break;
        }
        default:
            goto done;
    }

    iRet = 1;

done:
    if(iRet)
    {
        msg_print_uart1("OK\r\n");
    }
    else
    {
        msg_print_uart1("ERROR\r\n");
    }

    return iRet;
}


#if (WIFI_OTA_FUNCTION_EN == 1)
int app_at_cmd_sys_do_wifi_ota(char *buf, int len, int mode)
{
    int argc = 0;
    char *argv[AT_MAX_CMD_ARGS] = {0};

    if (AT_CMD_MODE_EXECUTION == mode)
    {
        BleWifi_Wifi_OtaTrigReq(WIFI_OTA_HTTP_URL);
        //msg_print_uart1("OK\r\n");
    }
    else if (AT_CMD_MODE_SET == mode)
    {
        if (!at_cmd_buf_to_argc_argv(buf, &argc, argv, AT_MAX_CMD_ARGS))
        {
            return false;
        }

        BleWifi_Wifi_OtaTrigReq((uint8_t*)(argv[1]));
        //msg_print_uart1("OK\r\n");
    }

    return true;
}
#endif

#ifdef __BLEWIFI_TRANSPARENT__
int app_at_cmd_sys_ble_cast(char *buf, int len, int mode)
{
    int iRet = 0;
    int argc = 0;
    char *argv[AT_MAX_CMD_ARGS] = {0};

    uint8_t u8BleCaseEnable = 0;
    uint32_t u32ExpireTime = 0;

    if (!at_cmd_buf_to_argc_argv(buf, &argc, argv, AT_MAX_CMD_ARGS))
    {
        goto done;
    }

    switch (mode)
    {
        case AT_CMD_MODE_SET:
        {
            if(argc != 2 && argc != 3)
            {
                AT_LOG("invalid param number\r\n");
                msg_print_uart1("+BLECAST: Invalid param number\n\n");
                goto done;
            }

            switch(argc){
                case 2:
                {
                    u8BleCaseEnable = (uint8_t)strtoul(argv[1], NULL, 0);
                    break;
                }
                case 3:
                {
                    u8BleCaseEnable = (uint8_t)strtoul(argv[1], NULL, 0);
                    u32ExpireTime = (uint32_t)strtoul(argv[2], NULL, 0);
                    break;
                }
                default:
                    break;
            }

            if ( BleWifi_Ctrl_BleCastWithExpire(u8BleCaseEnable, u32ExpireTime) < 0){
                msg_print_uart1("+BLECAST:Invalid value\n\n");
                goto done;
            }
            msg_print_uart1("+BLECAST:%d,%d\n\n", u8BleCaseEnable, u32ExpireTime);
            break;
        }

        default:
            goto done;
    }

    iRet = 1;

done:
    if(iRet)
    {
        msg_print_uart1("OK\n");
    }
    else
    {
        msg_print_uart1("ERROR\n");
    }

    return iRet;
}

int app_at_cmd_sys_ble_cast_param(char *buf, int len, int mode)
{
    int iRet = 0;
    int argc = 0;
    char *argv[AT_MAX_CMD_ARGS] = {0};

    uint32_t u32CastInterval = 0;

    if (!at_cmd_buf_to_argc_argv(buf, &argc, argv, AT_MAX_CMD_ARGS))
    {
        goto done;
    }

    switch (mode)
    {
        case AT_CMD_MODE_SET:
        {
            if(argc != 2)
            {
                AT_LOG("invalid param number\r\n");
                msg_print_uart1("+BLECASTPARAM: Invalid param number\n\n");
                goto done;
            }

            u32CastInterval = (uint32_t)strtoul(argv[1], NULL, 0);

            if ( BleWifi_Ctrl_BleCastParamSet(u32CastInterval) < 0){
                msg_print_uart1("+BLECASTPARAM:Invalid value\n\n");
                goto done;
            }
            msg_print_uart1("+BLECASTPARAM:%d\n\n", u32CastInterval);
            break;
        }
        default:
            goto done;
    }

    iRet = 1;

done:
    if(iRet)
    {
        msg_print_uart1("OK\n");
    }
    else
    {
        msg_print_uart1("ERROR\n");
    }

    return iRet;
}

int app_at_cmd_sys_ble_sts(char *buf, int len, int mode)
{
    int iRet = 0;

    switch (mode)
    {
        case AT_CMD_MODE_READ:
        {
            if ( true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_BLE_CONNECTED)) {
                msg_print_uart1("+BLESTS:CONNECTED\n\n");
            }
            else if ( true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_BLE_ADVTISING)) {
                msg_print_uart1("+BLESTS:ADVERTISING\n\n");
            }
            else if ( true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_BLE_INIT_DONE)) {
                msg_print_uart1("+BLESTS:IDLE\n\n");
            }
            else {
                msg_print_uart1("+BLESTS:INIT\n\n");
            }
            break;
       	}
        default:
            goto done;
    }

    iRet = 1;
done:
    if(iRet)
    {
        msg_print_uart1("OK\n");
    }
    else
    {
        msg_print_uart1("ERROR\n");
    }

    return iRet;
}

int app_at_cmd_sys_wifi_sts(char *buf, int len, int mode)
{
    int iRet = 0;

    switch (mode)
    {
        case AT_CMD_MODE_READ:
        {
            if ( true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_WIFI_SCANNING) ) {
                msg_print_uart1("+WIFISTS:SCANNING\n\n");
            }
            else if ( true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_WIFI_GOT_IP)) {
                msg_print_uart1("+WIFISTS:GOT IP\n\n");
            }
            else if ( true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTED)) {
                msg_print_uart1("+WIFISTS:CONNECTED\n\n");
            }
            else if ( true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_WIFI_CONNECTING) ) {
                msg_print_uart1("+WIFISTS:CONNECTNG\n\n");
            }
            else if ( true == BleWifi_Ctrl_EventStatusGet(BLEWIFI_CTRL_EVENT_BIT_WIFI_INIT_DONE)) {
                msg_print_uart1("+WIFISTS:IDLE\n\n");
            }
            else {
                msg_print_uart1("+WIFISTS:INIT\n\n");
            }
            break;
       	}
        default:
            goto done;
    }

    iRet = 1;
done:
    if(iRet)
    {
        msg_print_uart1("OK\n");
    }
    else
    {
        msg_print_uart1("ERROR\n");
    }

    return iRet;
}
#endif

at_command_t g_taAppAtCmd[] =
{
    { "at+readfim",         app_at_cmd_sys_read_fim,            "Read FIM data" },
    { "at+writefim",        app_at_cmd_sys_write_fim,           "Write FIM data" },
    { "at+dtim",            app_at_cmd_sys_dtim_time,           "Wifi DTIM" },
    { "at+mainfwver",       app_at_cmd_sys_main_fw_version,     "Main FW Version" },
    { "at+erasempfw",       app_at_cmd_sys_erase_mp_fw,         "Erase MP FW" },
    { "at+mpver",           app_at_cmd_sys_mp_version,          "MP Version" },
    { "at+flashid",         app_at_cmd_sys_FlashID,             "Flash ID" },
#if (WIFI_OTA_FUNCTION_EN == 1)
    { "at+ota",             app_at_cmd_sys_do_wifi_ota,         "Do Wifi OTA" },
#endif
#ifdef __BLEWIFI_TRANSPARENT__
    { "at+blecast",         app_at_cmd_sys_ble_cast,           "Ble Advertise" },
    { "at+blecastparam",    app_at_cmd_sys_ble_cast_param,     "Ble Advertise Param" },
    { "at+blests",          app_at_cmd_sys_ble_sts,            "Ble Status" },
    { "at+wifists",         app_at_cmd_sys_wifi_sts,           "WiFi Status" },
#endif
    { NULL,                 NULL,                               NULL},
};

void app_at_cmd_add(void)
{
    at_cmd_ext_tbl_reg(g_taAppAtCmd);
    return;
}
