

#include "smart_sleep.h"
#include "mw_fim_default_group03.h"
#include "mw_fim_default_group03_patch.h"
#include "mw_fim_default_group11_project.h"
#include "ps_public.h"
#include "blewifi_common.h"

#if (BLEWIFI_CTRL_WAKEUP_IO_EN == 1)
osTimerId g_tAppPsSmartDebounceTimerId;

static void BleWifi_Ps_Smart_DebounceTimerCallBack(void const *argu)
{
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_PS_SMART_DEBOUNCE_TIMEOUT, NULL, 0);
}

void BleWifi_Ps_Smart_TimerInit(void)
{
    osTimerDef_t tTimerPsSmartDef;

    // create the timer
    tTimerPsSmartDef.ptimer = BleWifi_Ps_Smart_DebounceTimerCallBack;
    g_tAppPsSmartDebounceTimerId = osTimerCreate(&tTimerPsSmartDef, osTimerOnce, NULL);

    if (g_tAppPsSmartDebounceTimerId == NULL)
    {
        printf("To create the timer for AppTimer is fail.\n");
    }
}

static void Ps_Smart_GPIOCallBack(E_GpioIdx_t tGpioIdx)
{
    // ISR level dont do printf 
    Hal_Vic_GpioIntEn(tGpioIdx, 0);
    // send the result to the task of blewifi control.
    BleWifi_Ctrl_MsgSend(BLEWIFI_CTRL_MSG_PS_SMART_STATE_CHANGE, NULL, 0);
}

void BleWifi_Ps_Smart_Pin_Config(E_GpioIdx_t tGpioIdx, uint32_t u32PinLevel)
{
 
    BLEWIFI_INFO("BleWifi_Ps_Smart_Pin_Config - u32PinLevel=%d\r\n", u32PinLevel);

    if(GPIO_LEVEL_HIGH == u32PinLevel)
    {
        Hal_Vic_GpioIntInv(tGpioIdx, 1);
        Hal_Vic_GpioIntEn(tGpioIdx, 1);
    }
    else
    {
        Hal_Vic_GpioIntInv(tGpioIdx, 0);
        Hal_Vic_GpioIntEn(tGpioIdx, 1);
    }
}

/*************************************************************************
* FUNCTION:
*   BleWifi_Ps_Smart_Init
*
* DESCRIPTION:
*   an example that generate the output pulses from one gpio to another,
*   then the input side will be triggered the interrupt.
*
* PARAMETERS
*   tGpioIdx    : Index of call-back GPIO
*
* RETURNS
*   none
*
*************************************************************************/
void BleWifi_Ps_Smart_Init(E_GpioIdx_t tGpioIdx, uint8_t ubSysMode, uint8_t ubPowerSaving)
{
    uint32_t u32PinLevel = 0;
    u32PinLevel = Hal_Vic_GpioInput(tGpioIdx);   
    BLEWIFI_INFO("u32PinLevel=%d\r\n", u32PinLevel);
    
    Hal_Vic_GpioCallBackFuncSet(tGpioIdx, Ps_Smart_GPIOCallBack);
    Hal_Vic_GpioIntTypeSel(tGpioIdx, INT_TYPE_LEVEL);
    Hal_Vic_GpioIntMask(tGpioIdx, 0);
    BleWifi_Ps_Smart_Pin_Config(tGpioIdx, u32PinLevel);

    BleWifi_Ps_Smart_TimerInit();

    if(BLEWIFI_CTRL_WAKEUP_IO_LEVEL != u32PinLevel)
    {
        /* Power saving settings */ 
        ps_smart_sleep(ubPowerSaving);
    }
}

#endif  // end of (BLEWIFI_CTRL_WAKEUP_IO_EN == 1)
