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

#include <stdlib.h>
#include <string.h>
#include "cmsis_os.h"
#include "sys_os_config.h"

#include "iot_data.h"
#include "blewifi_common.h"
#include "blewifi_configuration.h"
#include "blewifi_ctrl.h"
#include "blewifi_wifi_api.h"
#include "tcp_client.h"
#include "iot_rb_data.h"
#include "hal_vic.h"
#include "hal_uart.h"


uintptr_t g_tcp_handle;
int g_tcp_hdl_ID = 0;
int g_tcp_tx_ID = 0;
int g_tcp_rx_ID = 0;

#if (TCP_HEARTBEAT_TIMER_EN == 1)
static osTimerId g_tTcpHeartBeatTimerId;
#endif
static osSemaphoreId g_tTcpSemaphoreId;

static const char *pSocket=TCP_SERVER_ADDR;
static const int port = TCP_PORT;

static uint32_t g_ulRcvCount = 0;
static int g_iCntForPrint = 0;

#if (IOT_DEVICE_DATA_TX_EN == 1)
osThreadId g_tAppIotDataTxTaskId;
osMessageQId g_tAppIotDataTxQueueId;
#endif
#if (IOT_DEVICE_DATA_RX_EN == 1)
osThreadId g_tAppIotDataRxTaskId;
#endif

int Iot_Data_TxTask_MsgSend(int msg_type, uint8_t *data, int data_len);

#if (IOT_DEVICE_DATA_TX_EN == 1)
static void Iot_Data_TxTaskEvtHandler_DataPost(uint32_t evt_type, void *data, int len);
static void Iot_Data_TxTaskEvtHandler_Est_Tcp_Connection(uint32_t evt_type, void *data, int len);
static void Iot_Data_TxTaskEvtHandler_Send_Heartbeat(uint32_t evt_type, void *data, int len);
static T_IoT_Data_EvtHandlerTbl g_tAppIotDataTxTaskEvtHandlerTbl[] =
{
    {IOT_DATA_TX_MSG_DATA_POST,             Iot_Data_TxTaskEvtHandler_DataPost},
    {IOT_DATA_TX_MSG_EST_TCP_CONNECTION,    Iot_Data_TxTaskEvtHandler_Est_Tcp_Connection},
    {IOT_DATA_TX_MSG_SEND_HEARTBEAT,        Iot_Data_TxTaskEvtHandler_Send_Heartbeat},

    {0xFFFFFFFF,                            NULL}
};

static void Iot_Data_TxTaskEvtHandler_DataPost(uint32_t evt_type, void *data, int len)
{
    int ret = 0;

    // get the IoT data here, or the data are from *data and len parameters.
    // send the data to cloud

    if (true == BleWifi_EventStatusGet(g_tAppCtrlEventGroup, BLEWIFI_CTRL_EVENT_BIT_WIFI_GOT_IP))
    {
        if(true == BleWifi_EventStatusGet(g_tAppCtrlEventGroup, BLEWIFI_CTRL_EVENT_BIT_TCP_CONN))
        {
            // data from ring buffer
            IoT_Properity_t IoT_Properity;
            if (IOT_RB_DATA_OK != IoT_Ring_Buffer_CheckEmpty())
            {
                IoT_Ring_Buffer_Pop(&IoT_Properity);
                printf("TCP Wrt(%d)\n", IoT_Properity.ulLen);

                BleWifi_Wifi_SetDTIM(0);

                osSemaphoreWait(g_tTcpSemaphoreId, osWaitForever);
                if (g_tcp_tx_ID != g_tcp_hdl_ID)
                {
                    g_tcp_tx_ID = g_tcp_hdl_ID;
                }
                osSemaphoreRelease(g_tTcpSemaphoreId);

                ret = tcp_Send(g_tcp_handle, (const char *)(IoT_Properity.ubData), IoT_Properity.ulLen, TCP_TX_TIMEOUT);

                if (ret <= 0)
                {
                    printf("TCP Wrt failed (ret=%d)\n", ret);

#if (TCP_HEARTBEAT_TIMER_EN == 1)
                    osTimerStop(g_tTcpHeartBeatTimerId);
#endif
                    osSemaphoreWait(g_tTcpSemaphoreId, osWaitForever);
                    if ((g_tcp_handle != (uintptr_t)-1) && (g_tcp_tx_ID == g_tcp_hdl_ID))
                    {
                        ret = tcp_Disconnect(g_tcp_handle);
                        if (ret < 0)
                        {
                            printf("tcp dis connect 2 (ret=%d)\n", ret);
                        }
                        g_tcp_handle = (uintptr_t)-1;
                        g_tcp_hdl_ID = -1;
                        g_tcp_tx_ID = -1;
                        osSemaphoreRelease(g_tTcpSemaphoreId);
                        Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_EST_TCP_CONNECTION, NULL, 0);
                        BleWifi_EventStatusSet(g_tAppCtrlEventGroup, BLEWIFI_CTRL_EVENT_BIT_TCP_CONN, false);
                    }
                    else
                    {
                        osSemaphoreRelease(g_tTcpSemaphoreId);
                    }
                }
                else
                {
                    if (IoT_Properity.ubData != NULL)
                    {
                        free(IoT_Properity.ubData);
                    }
                    IoT_Ring_Buffer_ReadIdxUpdate();
                    Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_DATA_POST, NULL, 0);
                    //osDelay(10);
                }
            }
        }
    }
}

static void Iot_Data_TxTaskEvtHandler_Est_Tcp_Connection(uint32_t evt_type, void *data, int len)
{
    printf("Iot_Data_TxTaskEvtHandler_Est_Tcp_Connection\n");

    if (true == BleWifi_EventStatusGet(g_tAppCtrlEventGroup, BLEWIFI_CTRL_EVENT_BIT_WIFI_GOT_IP))
    {
        if(false == BleWifi_EventStatusGet(g_tAppCtrlEventGroup, BLEWIFI_CTRL_EVENT_BIT_TCP_CONN))
        {
            osSemaphoreWait(g_tTcpSemaphoreId, osWaitForever);
            BleWifi_Wifi_SetDTIM(0);
            g_tcp_handle = tcp_Establish(pSocket, port);
            if ((uintptr_t)-1 == g_tcp_handle)
            {
                osSemaphoreRelease(g_tTcpSemaphoreId);
                printf("... Failed to allocate socket. \r\n");
                //osDelay(5000);
                Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_EST_TCP_CONNECTION, NULL, 0);
            }
            else
            {
                g_tcp_hdl_ID++;
                g_tcp_hdl_ID = g_tcp_hdl_ID & (0xff);
                osSemaphoreRelease(g_tTcpSemaphoreId);

                printf("connect_tcp Success\n");
                BleWifi_Wifi_SetDTIM(BleWifi_Ctrl_DtimTimeGet());
                BleWifi_EventStatusSet(g_tAppCtrlEventGroup, BLEWIFI_CTRL_EVENT_BIT_TCP_CONN, true);
#if (TCP_HEARTBEAT_TIMER_EN == 1)
                osTimerStop(g_tTcpHeartBeatTimerId);
                osTimerStart(g_tTcpHeartBeatTimerId, TCP_HEARTBEAT_TIMEOUT);
#endif
                Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_DATA_POST, NULL, 0);
            }
        }
    }
}

static void Iot_Data_TxTaskEvtHandler_Send_Heartbeat(uint32_t evt_type, void *data, int len)
{
    BLEWIFI_INFO("Iot_Data_TxTaskEvtHandler_Send_HeartBeat - Send Heartbeat\n");

    if (true == BleWifi_EventStatusGet(g_tAppCtrlEventGroup, BLEWIFI_CTRL_EVENT_BIT_WIFI_GOT_IP))
    {
        if (true == BleWifi_EventStatusGet(g_tAppCtrlEventGroup, BLEWIFI_CTRL_EVENT_BIT_TCP_CONN))
        {
#if 0
            int ret;
            uint8_t ubLen = 5;
            uint8_t ubHeartBeatData[5] = {0x01};

            IoT_Properity_t IoT_Properity;
            IoT_Properity.ubData = malloc(sizeof(uint8_t)*ubLen);
            memset(IoT_Properity.ubData, 0, sizeof(uint8_t)*ubLen);
            memcpy(IoT_Properity.ubData, ubHeartBeatData, ubLen);
            IoT_Properity.ulLen = ubLen;
            if (IOT_RB_DATA_OK != IoT_Ring_Buffer_Push(&IoT_Properity))
            {
                BLEWIFI_ERROR("Uart0 - IoT_Ring_Buffer_Push failed!\n");
                free(IoT_Properity.ubData);
            }
            ret = Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_DATA_POST, NULL, 0);
            if (ret != 0)
            {
                BLEWIFI_ERROR("Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_DATA_POST, NULL, 0) failed!\n");
            }
#endif
        }
    }
}

#if (TCP_HEARTBEAT_TIMER_EN == 1)
static void Tcp_HeartBeat_TimerCallBack(void const *argu)
{
    Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_SEND_HEARTBEAT, NULL, 0);
}
#endif

void Iot_Data_TxTaskEvtHandler(uint32_t evt_type, void *data, int len)
{
    uint32_t i = 0;

    while (g_tAppIotDataTxTaskEvtHandlerTbl[i].ulEventId != 0xFFFFFFFF)
    {
        // match
        if (g_tAppIotDataTxTaskEvtHandlerTbl[i].ulEventId == evt_type)
        {
            g_tAppIotDataTxTaskEvtHandlerTbl[i].fpFunc(evt_type, data, len);
            break;
        }

        i++;
    }

    // not match
    if (g_tAppIotDataTxTaskEvtHandlerTbl[i].ulEventId == 0xFFFFFFFF)
    {
    }
}

void Iot_Data_TxTask(void *args)
{
    osEvent rxEvent;
    xIotDataMessage_t *rxMsg;

    // do the initialization
    while (1)
    {
        #if 1
        if (true == BleWifi_EventStatusWait(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_GOT_IP , 0xFFFFFFFF))
        {
            // init behavior
            BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_IOT_INIT , true);
            break;
        }
        // !!! if the IoT initialization is executed once by Tx or Rx, we could wait the behavior finish.
        #else
        if (true == BleWifi_EventStatusWait(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_IOT_INIT , 0xFFFFFFFF))
        {
            break;
        }
        #endif
    }

    while (1)
    {
        /* Wait event */
        rxEvent = osMessageGet(g_tAppIotDataTxQueueId, osWaitForever);
        if(rxEvent.status != osEventMessage)
            continue;

        rxMsg = (xIotDataMessage_t *)rxEvent.value.p;
        Iot_Data_TxTaskEvtHandler(rxMsg->event, rxMsg->ucaMessage, rxMsg->length);

        /* Release buffer */
        if (rxMsg != NULL)
            free(rxMsg);
    }
}

int Iot_Data_TxTask_MsgSend(int msg_type, uint8_t *data, int data_len)
{
    xIotDataMessage_t *pMsg = 0;

	if (NULL == g_tAppIotDataTxQueueId)
	{
        BLEWIFI_ERROR("BLEWIFI: No IoT Tx task queue \r\n");
        return -1;
	}

    /* Mem allocate */
    pMsg = malloc(sizeof(xIotDataMessage_t) + data_len);
    if (pMsg == NULL)
	{
        BLEWIFI_ERROR("BLEWIFI: IoT Tx task pmsg allocate fail \r\n");
	    goto error;
    }

    pMsg->event = msg_type;
    pMsg->length = data_len;
    if (data_len > 0)
    {
        memcpy(pMsg->ucaMessage, data, data_len);
    }

    if (osMessagePut(g_tAppIotDataTxQueueId, (uint32_t)pMsg, 0) != osOK)
    {
        printf("BLEWIFI: IoT Tx task message send fail \r\n");
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

void Iot_Data_TxInit(void)
{
    osThreadDef_t task_def;
    osMessageQDef_t queue_def;

    /* Create IoT Tx task */
    task_def.name = "iot tx";
    task_def.stacksize = IOT_DEVICE_DATA_TASK_STACK_SIZE_TX;
    task_def.tpriority = TCP_OS_TASK_PRIORITY;
    task_def.pthread = Iot_Data_TxTask;
    g_tAppIotDataTxTaskId = osThreadCreate(&task_def, (void*)NULL);
    if(g_tAppIotDataTxTaskId == NULL)
    {
        BLEWIFI_INFO("BLEWIFI: IoT Tx task create fail \r\n");
    }
    else
    {
        BLEWIFI_INFO("BLEWIFI: IoT Tx task create successful \r\n");
    }

    /* Create IoT Tx message queue*/
    queue_def.item_sz = sizeof(xIotDataMessage_t);
    queue_def.queue_sz = IOT_DEVICE_DATA_QUEUE_SIZE_TX;
    g_tAppIotDataTxQueueId = osMessageCreate(&queue_def, NULL);
    if(g_tAppIotDataTxQueueId == NULL)
    {
        BLEWIFI_ERROR("BLEWIFI: IoT Tx create queue fail \r\n");
    }
}
#endif  // end of #if (IOT_DEVICE_DATA_TX_EN == 1)

#if (IOT_DEVICE_DATA_RX_EN == 1)
void Iot_Data_RxTask(void *args)
{
    int ret = 0;
    char szBuf[TCP_RX_BUFFER_SIZE] = {0};

    // do the initialization
    while (0)
    {
        #if 1
        if (true == BleWifi_EventStatusWait(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_GOT_IP , 0xFFFFFFFF))
        {
            // init behavior
            BleWifi_EventStatusSet(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_IOT_INIT , true);
            break;
        }
        // !!! if the IoT initialization is executed once by Tx or Rx, we could wait the behavior finish.
        #else
        if (true == BleWifi_EventStatusWait(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_IOT_INIT , 0xFFFFFFFF))
        {
            break;
        }
        #endif
    }

    // do the rx behavior
    while (1)
    {
        if (true == BleWifi_EventStatusWait(g_tAppCtrlEventGroup , BLEWIFI_CTRL_EVENT_BIT_WIFI_GOT_IP , 0xFFFFFFFF))
        {
            if(true == BleWifi_EventStatusWait(g_tAppCtrlEventGroup, BLEWIFI_CTRL_EVENT_BIT_TCP_CONN, 0xFFFFFFFF))
            {
                memset(szBuf, 0, sizeof(szBuf));

                osSemaphoreWait(g_tTcpSemaphoreId, osWaitForever);
                if (g_tcp_rx_ID != g_tcp_hdl_ID)
                {
                    g_tcp_rx_ID = g_tcp_hdl_ID;
                }
                osSemaphoreRelease(g_tTcpSemaphoreId);
                ret = tcp_Recv(g_tcp_handle, szBuf, TCP_RX_BUFFER_SIZE, TCP_RX_TIMEOUT);
                if (ret > 0)
                {
                    g_ulRcvCount += ret;
                    g_iCntForPrint++;
                    printf("%d:Rcv %u\n", g_iCntForPrint, g_ulRcvCount);

#if (TCP_LOOPBACK_TEST_MODE_EN == 1)
                    IoT_Properity_t IoT_Properity;
                    IoT_Properity.ubData = malloc(sizeof(uint8_t)*ret);
                    memset(IoT_Properity.ubData, 0, sizeof(uint8_t)*ret);
                    memcpy(IoT_Properity.ubData, szBuf, ret);
                    IoT_Properity.ulLen = ret;
                    if (IOT_RB_DATA_OK != IoT_Ring_Buffer_Push(&IoT_Properity))
                    {
                        BLEWIFI_ERROR("Uart0 - IoT_Ring_Buffer_Push failed!\n");
                        free(IoT_Properity.ubData);
                    }
                    ret = Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_DATA_POST, NULL, 0);
                    if (ret != 0)
                    {
                        BLEWIFI_ERROR("Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_DATA_POST, NULL, 0) failed!\n");
                    }
#endif  // end of #if (TCP_LOOPBACK_TEST_MODE_EN == 1)

#if 0
                    for (int i=0; i<ret; i++)
                    {
                       Hal_Uart_DataSend(UART_IDX_0, szBuf[i]);
                    }
#endif
                    BleWifi_Wifi_SetDTIM(BleWifi_Ctrl_DtimTimeGet());
                }
                else if (ret < 0)
                {
                    printf("TCP Rcv failed (ret=%d)\n", ret);

#if (TCP_HEARTBEAT_TIMER_EN == 1)
                    osTimerStop(g_tTcpHeartBeatTimerId);
#endif
                    osSemaphoreWait(g_tTcpSemaphoreId, osWaitForever);
                    if ((g_tcp_handle != (uintptr_t)-1) && (g_tcp_rx_ID == g_tcp_hdl_ID))
                    {
                        ret = tcp_Disconnect(g_tcp_handle);
                        if (ret < 0)
                        {
                            printf("tcp dis connect 3 (ret=%d)\n", ret);
                        }
                        g_tcp_handle = (uintptr_t)-1;
                        g_tcp_hdl_ID = -1;
                        g_tcp_rx_ID = -1;
                        osSemaphoreRelease(g_tTcpSemaphoreId);

                        BleWifi_EventStatusSet(g_tAppCtrlEventGroup, BLEWIFI_CTRL_EVENT_BIT_TCP_CONN, false);
                        Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_EST_TCP_CONNECTION, NULL, 0);
                    }
                    else
                    {
                        osSemaphoreRelease(g_tTcpSemaphoreId);
                    }
                }
            }
//            osDelay(3000); // if do nothing for rx behavior, the delay must be exist.
                            // if do something for rx behavior, the delay could be removed.
        }
    }
}

void Iot_Data_RxInit(void)
{
    osThreadDef_t task_def;

    /* Create IoT Rx task */
    task_def.name = "iot rx";
    task_def.stacksize = IOT_DEVICE_DATA_TASK_STACK_SIZE_RX;
    task_def.tpriority = TCP_OS_TASK_PRIORITY;
    task_def.pthread = Iot_Data_RxTask;
    g_tAppIotDataRxTaskId = osThreadCreate(&task_def, (void*)NULL);
    if(g_tAppIotDataRxTaskId == NULL)
    {
        BLEWIFI_INFO("BLEWIFI: IoT Rx task create fail \r\n");
    }
    else
    {
        BLEWIFI_INFO("BLEWIFI: IoT Rx task create successful \r\n");
    }
}
#endif  // end of #if (IOT_DEVICE_DATA_RX_EN == 1)

#if (IOT_DEVICE_DATA_TX_EN == 1) || (IOT_DEVICE_DATA_RX_EN == 1)
void Iot_Data_Init(void)
{
    osSemaphoreDef_t tSemaphoreDef;
#if (TCP_HEARTBEAT_TIMER_EN == 1)
    osTimerDef_t tTimerHeartBeatDef;
#endif

#if (IOT_DEVICE_DATA_TX_EN == 1)
    Iot_Data_TxInit();
#endif

#if (IOT_DEVICE_DATA_RX_EN == 1)
    Iot_Data_RxInit();
#endif

    IoT_Ring_Buffer_Init();

    // create the semaphore
    tSemaphoreDef.dummy = 0;                            // reserved, it is no used
    g_tTcpSemaphoreId = osSemaphoreCreate(&tSemaphoreDef, 1);
    if (g_tTcpSemaphoreId == NULL)
    {
        BLEWIFI_ERROR("To create the IoT semaphore is fail.\n");
    }

#if (TCP_HEARTBEAT_TIMER_EN == 1)
    // create Heartbeat timer
    tTimerHeartBeatDef.ptimer = Tcp_HeartBeat_TimerCallBack;
    g_tTcpHeartBeatTimerId = osTimerCreate(&tTimerHeartBeatDef, osTimerPeriodic, NULL);
    if (g_tTcpHeartBeatTimerId == NULL)
    {
        BLEWIFI_ERROR("To create the timer for TCP heartbeat is fail.\n");
    }
#endif
}
#endif  // end of #if (IOT_DEVICE_DATA_TX_EN == 1) || (IOT_DEVICE_DATA_RX_EN == 1)
