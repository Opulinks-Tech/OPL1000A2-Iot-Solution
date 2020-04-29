#ifndef __UART_H__
#define __UART_H__


#include <stdint.h>
#include <stdbool.h>


typedef enum iot_uart0_msg_type
{
    IOT_UART0_MSG_DATA_IN = 0,

    IOT_UART0_MSG_NUM
} iot_uart0_msg_type_e;

typedef struct
{
    uint32_t event;
    uint32_t length;
    uint8_t ucaMessage[];
} xIotUart0Message_t;

typedef void (*T_IoT_Uart0_EvtHandler_Fp)(uint32_t evt_type, uint8_t *data, int len);
typedef struct
{
    uint32_t ulEventId;
    T_IoT_Uart0_EvtHandler_Fp fpFunc;
} T_IoT_Uart0_EvtHandlerTbl;


#if (IOT_UART0_EN == 1)
void Iot_Uart0_Init(void);
#if (IOT_UART0_TASK_EN == 1)
void Iot_Uart0_Task_Init(void);
#endif  // end of #if (IOT_UART0_TASK_EN == 1)
#endif  // end of #if (IOT_UART0_EN == 1)

#endif  // end of #ifndef __UART_H__
