#include <string.h>
#include <stdlib.h>
#include "uart.h"
#include "cmsis_os.h"
#include "sys_os_config.h"
#include "blewifi_common.h"
#include "hal_uart.h"
#include "mw_fim_default_group01.h"
#include "boot_sequence.h"
#include "iot_data.h"
#include "iot_rb_data.h"

#define UART0_MESSAGE_Q_SIZE    16      // the number of elements in the message queue
#define UART0_RCV_TIMEOUT       100     // ms
#define UART0_RCV_MAX_LEN       2048    // bytes

#if (IOT_UART0_EN == 1)
#if (IOT_UART0_RECV_WITH_TIMER_METHO_EN == 1)
static osTimerId g_tIotUart0TimerId;
#endif
static uint8_t g_ucIotUart0RcvData[UART0_RCV_MAX_LEN];     // Receiving Data content
static uint16_t g_uwIotUart0Cnt = 0;

#if (IOT_UART0_TASK_EN == 1)
static osThreadId g_tIotUart0Thread;
static osMessageQId g_tIotUart0MessageQ;

// the message table of UART0
static void Iot_Uart0_EvtHandler_Data_In(uint32_t evt_type, uint8_t *data, int len);
static T_IoT_Uart0_EvtHandlerTbl g_tIotUart0EvtHandlerTbl[] =
{
    {IOT_UART0_MSG_DATA_IN,             Iot_Uart0_EvtHandler_Data_In},

    {0xFFFFFFFF,                        NULL}
};

/*************************************************************************
* FUNCTION:
*   Iot_Uart0_EvtHandler_Data_In
*
* DESCRIPTION:
*   the data in hanlder of Iot Uart0
*
* PARAMETERS
*   1. evt_type : [In] the event type
*   2. data     : [In] the pointer of data
*   3. len      : [In] the length of data
*
* RETURNS
*   none
*
*************************************************************************/
int g_lRcvCnt=0;
static void Iot_Uart0_EvtHandler_Data_In(uint32_t evt_type, uint8_t *data, int len)
{
    // output the content of message
    g_lRcvCnt += len;
    printf("uart0 total read(%d)\n", g_lRcvCnt);

#if 0
    // Send the message into Iot Tx MessageQ
    if (len > 0)
    {
        int ret;
        IoT_Properity_t IoT_Properity;

        IoT_Properity.ubData = malloc(sizeof(uint8_t)*len);
        memset(IoT_Properity.ubData, 0, sizeof(uint8_t)*len);
        memcpy(IoT_Properity.ubData, data, len);
        IoT_Properity.ulLen = len;
        if (IOT_RB_DATA_OK != IoT_Ring_Buffer_Push(&IoT_Properity))
        {
            BLEWIFI_INFO("Uart0 - IoT_Ring_Buffer_Push failed!\n");
            free(IoT_Properity.ubData);
            goto done;
        }

        ret = Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_DATA_POST, NULL, 0);
        if (0 != ret)
        {
            BLEWIFI_INFO("Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_DATA_POST, NULL, 0) failed!\n");
        }

done:
        return;
    }
#endif
}

/*************************************************************************
* FUNCTION:
*   IoT_Uart0_MsgSend
*
* DESCRIPTION:
*   send the message into Iot Uart0 MessageQ
*
* PARAMETERS
*   1. evt_type : [In] the event type
*   2. data     : [In] the pointer of data
*   3. data_len : [In] the length of data
*
* RETURNS
*   0  : success
*   -1 : fail
*
*************************************************************************/
int IoT_Uart0_MsgSend(int msg_type, uint8_t *data, int data_len)
{
    xIotUart0Message_t *pMsg = 0;

	if (NULL == g_tIotUart0MessageQ)
	{
        BLEWIFI_ERROR("No queue \n");
        return -1;
	}

    /* Mem allocate */
    pMsg = malloc(sizeof(xIotUart0Message_t) + data_len);
    if (pMsg == NULL)
	{
        BLEWIFI_ERROR("UART0 task pmsg allocate fail \n");
	    goto error;
    }

    pMsg->event = msg_type;
    pMsg->length = data_len;
    if (data_len > 0)
    {
        memcpy(pMsg->ucaMessage, data, data_len);
    }

    if (osMessagePut(g_tIotUart0MessageQ, (uint32_t)pMsg, 0) != osOK)
    {
        printf("IoT_Uart0_MsgSend fail \n");
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

/*************************************************************************
* FUNCTION:
*   Iot_Uart0_TxTaskEvtHandler
*
* DESCRIPTION:
*   the event hanlder of Iot Uart0
*
* PARAMETERS
*   1. evt_type : [In] the event type
*   2. data     : [In] the pointer of data
*   3. len      : [In] the length of data
*
* RETURNS
*   none
*
*************************************************************************/
void Iot_Uart0_TxTaskEvtHandler(uint32_t evt_type, void *data, int len)
{
    uint32_t i = 0;

    while (g_tIotUart0EvtHandlerTbl[i].ulEventId != 0xFFFFFFFF)
    {
        // match
        if (g_tIotUart0EvtHandlerTbl[i].ulEventId == evt_type)
        {
            g_tIotUart0EvtHandlerTbl[i].fpFunc(evt_type, data, len);
            break;
        }

        i++;
    }

    // not match
    if (g_tIotUart0EvtHandlerTbl[i].ulEventId == 0xFFFFFFFF)
    {
    }
}

/*************************************************************************
* FUNCTION:
*   Iot_Uart0_ProcessingFunction
*
* DESCRIPTION:
*   the task of Iot Uart0
*
* PARAMETERS
*   1. argu   : [In] it is not used
*
* RETURNS
*   none
*
*************************************************************************/
static void Iot_Uart0_ProcessingFunction(void *argu)
{
    osEvent tEvent;
    xIotUart0Message_t *ptMsg;

    while (1)
    {
        // receive the message from Uart0MessageQ
        tEvent = osMessageGet(g_tIotUart0MessageQ, osWaitForever);
        if (tEvent.status != osEventMessage)
        {
            BLEWIFI_ERROR("To receive the message from Uart0MessageQ is fail.\n");
            continue;
        }

        // get the content of message
        ptMsg = (xIotUart0Message_t *)tEvent.value.p;

        Iot_Uart0_TxTaskEvtHandler(ptMsg->event, ptMsg->ucaMessage, ptMsg->length);

        /* Release buffer */
        if (ptMsg != NULL)
            free(ptMsg);
    }
}

/*************************************************************************
* FUNCTION:
*   Iot_Uart0_Task_Init
*
* DESCRIPTION:
*   an example that read and write data on UART0, and send the rx data to
*   thread when the rx data is received in UART0 interrupt.
*
* PARAMETERS
*   none
*
* RETURNS
*   none
*
*************************************************************************/
void Iot_Uart0_Task_Init(void)
{
    osThreadDef_t tThreadDef;
    osMessageQDef_t tMessageDef;

    // create the thread for Uart0Thread
    tThreadDef.name      = "UART0_task";
    tThreadDef.pthread   = Iot_Uart0_ProcessingFunction;
    tThreadDef.tpriority = OS_TASK_PRIORITY_APP;            // osPriorityNormal
    tThreadDef.instances = 0;                               // reserved, it is no used
    tThreadDef.stacksize = OS_TASK_STACK_SIZE_APP;          // (512), unit: 4-byte, the size is 512*4 bytes
    g_tIotUart0Thread = osThreadCreate(&tThreadDef, NULL);
    if (g_tIotUart0Thread == NULL)
    {
        BLEWIFI_INFO("To create the thread for Uart0Thread is fail.\n");
    }

    // create the message queue for Uart0MessageQ
    tMessageDef.queue_sz = UART0_MESSAGE_Q_SIZE;            // number of elements in the queue
    tMessageDef.item_sz = sizeof(xIotUart0Message_t);       // size of an item
    tMessageDef.pool = NULL;                                // reserved, it is no used
    g_tIotUart0MessageQ = osMessageCreate(&tMessageDef, g_tIotUart0Thread);
    if (g_tIotUart0MessageQ == NULL)
    {
        BLEWIFI_INFO("To create the message queue for Uart0MessageQ is fail.\n");
    }
}
#endif  // end of #if (IOT_UART0_TASK_EN == 1)

/*************************************************************************
* FUNCTION:
*   Iot_Uart0_RxDataSend
*
* DESCRIPTION:
*   send the rx data to Uart0MessageQ or Iot Tx MessageQ
*
* PARAMETERS
*   none
*
* RETURNS
*   none
*
*************************************************************************/
static void Iot_Uart0_RxDataSend(void)
{
    // error handle
    if (g_uwIotUart0Cnt <= 0)
        return;

#if (IOT_UART0_TASK_EN == 1)
    // Send the message into Uart0MessageQ
    IoT_Uart0_MsgSend(IOT_UART0_MSG_DATA_IN, g_ucIotUart0RcvData, g_uwIotUart0Cnt);

    memset(g_ucIotUart0RcvData, 0, sizeof(uint8_t)*UART0_RCV_MAX_LEN);
    g_uwIotUart0Cnt = 0;
    return;
#else
    // Send the message into Iot Tx MessageQ
    int ret;
    IoT_Properity_t IoT_Properity;

    IoT_Properity.ubData = malloc(sizeof(uint8_t)*g_uwIotUart0Cnt);
    memset(IoT_Properity.ubData, 0, sizeof(uint8_t)*g_uwIotUart0Cnt);
    memcpy(IoT_Properity.ubData, g_ucIotUart0RcvData, g_uwIotUart0Cnt);
    IoT_Properity.ulLen = g_uwIotUart0Cnt;
    if (IOT_RB_DATA_OK != IoT_Ring_Buffer_Push(&IoT_Properity))
    {
        BLEWIFI_ERROR("Uart0 - IoT_Ring_Buffer_Push failed!\n");
        free(IoT_Properity.ubData);
        goto done;
    }

    ret = Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_DATA_POST, NULL, 0);
    if (0 != ret)
    {
        BLEWIFI_ERROR("Iot_Data_TxTask_MsgSend(IOT_DATA_TX_MSG_DATA_POST, NULL, 0) failed!\n");
    }

done:
    memset(g_ucIotUart0RcvData, 0, sizeof(uint8_t)*UART0_RCV_MAX_LEN);
    g_uwIotUart0Cnt = 0;
    return;
#endif  // end of #if (IOT_UART0_TASK_EN == 1)
}

/*************************************************************************
* FUNCTION:
*   Iot_Uart0_RcvTimerCallBack
*
* DESCRIPTION:
*   the timer callback function of Iot Uart0
*
* PARAMETERS
*   1. argu   : [In] it is not used
*
* RETURNS
*   none
*
*************************************************************************/
#if (IOT_UART0_RECV_WITH_TIMER_METHO_EN == 1)
static void Iot_Uart0_RcvTimerCallBack(void const *argu)
{
    //printf("Timer expired~!\n");
    Iot_Uart0_RxDataSend();
}
#endif

/*************************************************************************
* FUNCTION:
*   Iot_Uart0_ReceiveMessage
*
* DESCRIPTION:
*   1. receive the data from uart rx
*   2. send the message into Uart0MessageQ
*
* PARAMETERS
*   1. ulData   : [In] the data from uart rx
*
* RETURNS
*   none
*
*************************************************************************/
static void Iot_Uart0_ReceiveMessage(uint32_t ulData)
{
    //BLEWIFI_INFO("Enter Iot_Uart0_ReceiveMessage\n");

    g_ucIotUart0RcvData[g_uwIotUart0Cnt] = ulData;
    g_uwIotUart0Cnt++;

    // Check Length error handling
    if (g_uwIotUart0Cnt == UART0_RCV_MAX_LEN)
    {
        //printf("Iot_Uart0_ReceiveMessage - Rcv MAX LENGTH\n");

#if (IOT_UART0_RECV_WITH_TIMER_METHO_EN == 1)
        osTimerStop(g_tIotUart0TimerId);
#endif
        Iot_Uart0_RxDataSend();
    }
    else
    {
#if (IOT_UART0_RECV_WITH_TIMER_METHO_EN == 1)
        osTimerStop(g_tIotUart0TimerId);
        osTimerStart(g_tIotUart0TimerId, UART0_RCV_TIMEOUT);
#endif
    }
}

/*************************************************************************
* FUNCTION:
*   Iot_Uart0_Init
*
* DESCRIPTION:
*   the initial of Iot Uart0
*
* PARAMETERS
*   none
*
* RETURNS
*   none
*
*************************************************************************/
void Iot_Uart0_Init(void)
{
    T_HalUartConfig tUartConfig;

#if (IOT_UART0_RECV_WITH_TIMER_METHO_EN == 1)
    osTimerDef_t tTimerUart0Def;
#endif

    // Init UART0
    // cold boot
    if (0 == Boot_CheckWarmBoot())
    {
        if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP01_UART_CFG, 0, MW_FIM_UART_CFG_SIZE, (uint8_t*)&tUartConfig))
        {
            // if fail, get the default value
            memcpy(&tUartConfig, &g_tMwFimDefaultUartConfig, MW_FIM_UART_CFG_SIZE);
        }

        // Initialize Receiving Data
        memset(g_ucIotUart0RcvData, 0, sizeof(uint8_t)*UART0_RCV_MAX_LEN);
        g_uwIotUart0Cnt = 0;

#if (IOT_UART0_RECV_WITH_TIMER_METHO_EN == 1)
        // create the timer
        tTimerUart0Def.ptimer = Iot_Uart0_RcvTimerCallBack;
        g_tIotUart0TimerId = osTimerCreate(&tTimerUart0Def, osTimerOnce, NULL);
        if (g_tIotUart0TimerId == NULL)
        {
            BLEWIFI_INFO("To create the timer for Uart0 Timer is fail.\n");
        }
        else
        {
            BLEWIFI_INFO("To create the timer for Uart0 Timer is successful.\n");
        }
#endif
    }
    // warm boot
    else
    {
        if (0 != Hal_Uart_ConfigGet(UART_IDX_0, &tUartConfig))
        {
            // if fail, get the default value
            memcpy(&tUartConfig, &g_tMwFimDefaultUartConfig, MW_FIM_UART_CFG_SIZE);
        }
    }

    Hal_Uart_Init(UART_IDX_0,
                  tUartConfig.ulBuadrate,
                  (E_UartDataBit_t)(tUartConfig.ubDataBit),
                  (E_UartParity_t)(tUartConfig.ubParity),
                  (E_UartStopBit_t)(tUartConfig.ubStopBit),
                  tUartConfig.ubFlowCtrl);

    // set the rx callback function and enable the rx interrupt
    Hal_Uart_RxCallBackFuncSet(UART_IDX_0, Iot_Uart0_ReceiveMessage);
    Hal_Uart_RxIntEn(UART_IDX_0, 1);
}
#endif  // end of #if (IOT_UART0_EN == 1)

