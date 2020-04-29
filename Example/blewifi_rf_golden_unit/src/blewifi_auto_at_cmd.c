/******************************************************************************
*  Copyright 2017 - 2020, Opulinks Technology Ltd.
*  ---------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2020
******************************************************************************/

#include "blewifi_configuration.h"
#include "blewifi_common.h"
#include "blewifi_app.h"
#include "blewifi_wifi_api.h"
#include "blewifi_ble_api.h"
#include "blewifi_ctrl.h"
#include "blewifi_ctrl_http_ota.h"
#include "sys_common_api.h"
#include "mw_fim_default_group14_project.h"
#include "app_at_cmd.h"
#include "cmsis_os.h"
#include "at_cmd_common_patch.h"
#include "at_cmd_task.h"

#define IOT_DEVICE_DATA_QUEUE_SIZE       (1024)
#define AT_CMD_TEST_NUM (4)

osThreadId g_tAppIotDataATCmdTaskID;

extern volatile T_MwFim_GP14_Boot_Status g_tBootStatus;

void ExecATCommand(void *args)
{
    xATMessage stATMsg;
    at_uart_buffer_t stUartBuffer;
    uint8_t u8Idx = 0;

    // clear boot cnt
    osDelay(5000);
    g_tBootStatus.u8Cnt = 0;
    if (MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP14_PROJECT_BOOT_STATUS, 0, MW_FIM_GP14_BOOT_STATUS_SIZE, (uint8_t *)&g_tBootStatus))
    {
        BLEWIFI_ERROR("[%s][%d] MwFim_FileWrite fail\n", __func__, __LINE__);
    }
    else
    {
        BLEWIFI_WARN("[%s][%d] clear boot_cnt\n", __func__, __LINE__);
    }

    memset(&stATMsg,0,sizeof(xATMessage));
    stATMsg.pcMessage = (char *)&stUartBuffer;
    stATMsg.length = sizeof(at_uart_buffer_t);
    stATMsg.event = AT_UART1_EVENT;

    for(u8Idx=0;u8Idx<AT_CMD_TEST_NUM;u8Idx++)
    {
        switch(u8Idx)
        {
            case 0:
                sprintf(stUartBuffer.buf,"at+mode=3\r\0");
                stUartBuffer.in = strlen(stUartBuffer.buf);
                break;
            case 1:
                sprintf(stUartBuffer.buf,"at+channel=7\r\0");
                stUartBuffer.in = strlen(stUartBuffer.buf);
                break;
            case 2:
                sprintf(stUartBuffer.buf,"at+go=1,6,30,1,0\r\0");
                stUartBuffer.in = strlen(stUartBuffer.buf);
                break;
            case 3:
                sprintf(stUartBuffer.buf,"at+tx=1\r\0");
                stUartBuffer.in = strlen(stUartBuffer.buf);
                break;
            default:
                BLEWIFI_ERROR("[%s][%d]Error u8Idx = %u\n",__func__,__LINE__,u8Idx);
                break;
        }
        msg_print_uart1("%s\n",stUartBuffer.buf);
        at_task_send( stATMsg );
        stUartBuffer.buf[0]='\n';
        stUartBuffer.buf[1]='\0';
        stUartBuffer.in = 1;
        at_task_send( stATMsg );
        osDelay(1000);
    }
    osThreadTerminate(NULL);
}
void ATCmdProc(void)
{
    osThreadDef_t task_def;

    /* Create task */
    task_def.name = "AT command test";
    task_def.stacksize = IOT_DEVICE_DATA_QUEUE_SIZE;
    task_def.tpriority = osPriorityNormal;
    task_def.pthread = ExecATCommand;

    g_tAppIotDataATCmdTaskID = osThreadCreate(&task_def, (void*)NULL);
    if(g_tAppIotDataATCmdTaskID == NULL)
    {
        BLEWIFI_INFO("BLEWIFI: AT command test task create fail \r\n");
    }
    else
    {
        BLEWIFI_INFO("BLEWIFI: AT command test create successful \r\n");
    }
}
