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
#include "iot_rb_data.h"
#include "cmsis_os.h"

IoT_Ring_buffer_t g_tRBData;
static osSemaphoreId g_tAppSemaphoreId;

void IoT_Ring_Buffer_Init(void)
{
    osSemaphoreDef_t tSemaphoreDef;
        
    // create the semaphore
    tSemaphoreDef.dummy = 0;                            // reserved, it is no used
    g_tAppSemaphoreId = osSemaphoreCreate(&tSemaphoreDef, 1);
    if (g_tAppSemaphoreId == NULL)
    {
        printf("To create the semaphore for AppSemaphore is fail.\n");
    }
    osSemaphoreWait(g_tAppSemaphoreId, osWaitForever);
    g_tRBData.ulReadIdx = 0;
    g_tRBData.ulWriteIdx = 0;
    osSemaphoreRelease(g_tAppSemaphoreId);
//    IoT_Ring_Buffer_ResetBuffer();
}

uint8_t IoT_Ring_Buffer_Push(IoT_Properity_t *ptProperity)
{
    uint32_t ulWriteNext;
    
    osSemaphoreWait(g_tAppSemaphoreId, osWaitForever);
    
    if (ptProperity == NULL)
    {
        osSemaphoreRelease(g_tAppSemaphoreId);
        return IOT_RB_DATA_FAIL;
    }

    // full, ulWriteIdx + 1 == ulReadIdx
    ulWriteNext = (g_tRBData.ulWriteIdx + 1) % IOT_RB_COUNT;
    
    // Read index alway prior to write index
    if (ulWriteNext == g_tRBData.ulReadIdx)
    {
        // discard the oldest data, and read index move forware one step.
//        free(g_tRBData.taProperity[g_tRBData.ulReadIdx].ubData);
//        g_tRBData.ulReadIdx = (g_tRBData.ulReadIdx + 1) % IOT_RB_COUNT;
        osSemaphoreRelease(g_tAppSemaphoreId);
        return IOT_RB_DATA_FAIL;
        
    }

    // update the temperature data to write index
	memcpy(&(g_tRBData.taProperity[g_tRBData.ulWriteIdx]), ptProperity, sizeof(IoT_Properity_t));
    g_tRBData.ulWriteIdx = ulWriteNext;
    
    osSemaphoreRelease(g_tAppSemaphoreId);

    return IOT_RB_DATA_OK;
}

uint8_t IoT_Ring_Buffer_Pop(IoT_Properity_t *ptProperity)
{
    uint8_t bRet = IOT_RB_DATA_FAIL;
    
    osSemaphoreWait(g_tAppSemaphoreId, osWaitForever);
    if (ptProperity == NULL)
    {
        osSemaphoreRelease(g_tAppSemaphoreId);
        return IOT_RB_DATA_FAIL;
    }
    // empty, ulWriteIdx == ulReadIdx
    if (g_tRBData.ulWriteIdx == g_tRBData.ulReadIdx)
    {
        osSemaphoreRelease(g_tAppSemaphoreId);
        goto done;
    }
	memcpy(ptProperity, &(g_tRBData.taProperity[g_tRBData.ulReadIdx]), sizeof(IoT_Properity_t));
	
    bRet = IOT_RB_DATA_OK;
    
    osSemaphoreRelease(g_tAppSemaphoreId);
done:
    return bRet;
}

uint8_t IoT_Ring_Buffer_CheckEmpty(void)
{
    osSemaphoreWait(g_tAppSemaphoreId, osWaitForever);
    // empty, ulWriteIdx == ulReadIdx
    if (g_tRBData.ulWriteIdx == g_tRBData.ulReadIdx)
    {
        osSemaphoreRelease(g_tAppSemaphoreId);
        return IOT_RB_DATA_OK;
    }
    
    osSemaphoreRelease(g_tAppSemaphoreId);
    return IOT_RB_DATA_FAIL;
}

void IoT_Ring_Buffer_ResetBuffer(void)
{
    IoT_Properity_t ptProperity;
    
    osSemaphoreWait(g_tAppSemaphoreId, osWaitForever);
    while(IOT_RB_DATA_OK!=IoT_Ring_Buffer_CheckEmpty())
    {
        IoT_Ring_Buffer_Pop(&ptProperity);
        IoT_Ring_Buffer_ReadIdxUpdate();

#ifdef RB_POINTER_DATA_TYPE        
        if(ptProperity.ubData!=NULL)
            free(ptProperity.ubData);
#endif        
    }
    osSemaphoreRelease(g_tAppSemaphoreId);    
}

uint8_t IoT_Ring_Buffer_ReadIdxUpdate(void)
{
    osSemaphoreWait(g_tAppSemaphoreId, osWaitForever);
    g_tRBData.ulReadIdx = (g_tRBData.ulReadIdx + 1) % IOT_RB_COUNT;
    osSemaphoreRelease(g_tAppSemaphoreId); 
    return IOT_RB_DATA_OK;
}

uint8_t IoT_Ring_Buffer_CheckFull(void)
{
    uint32_t ulWriteNext;
    
    osSemaphoreWait(g_tAppSemaphoreId, osWaitForever);
    // full, ulWriteIdx + 1 == ulReadIdx
    ulWriteNext = (g_tRBData.ulWriteIdx + 1) % IOT_RB_COUNT;
    
    // Read index alway prior to write index
    if (ulWriteNext == g_tRBData.ulReadIdx)
    {
        osSemaphoreRelease(g_tAppSemaphoreId); 
        return IOT_RB_DATA_OK;
    }
    
    osSemaphoreRelease(g_tAppSemaphoreId); 
    return IOT_RB_DATA_FAIL;
}

