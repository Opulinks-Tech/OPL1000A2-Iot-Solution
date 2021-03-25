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

/***********************
Head Block of The File
***********************/
#ifndef _MW_FIM_DEFAULT_GROUP11_PROJECT_H_
#define _MW_FIM_DEFAULT_GROUP11_PROJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

// Sec 0: Comment block of the file


// Sec 1: Include File
#include "mw_fim.h"
#include "blewifi_configuration.h"

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
#include "wifi_types.h"
#endif

// Sec 2: Constant Definitions, Imported Symbols, miscellaneous
// the file ID
// xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx
// ^^^^ ^^^^ Zone (0~3)
//           ^^^^ ^^^^ Group (0~8), 0 is reserved for swap
//                     ^^^^ ^^^^ ^^^^ ^^^^ File ID, start from 0
typedef enum
{
#ifdef __BLEWIFI_TRANSPARENT__
    MW_FIM_IDX_GP11_PROJECT_START = 0x00080000,             // the start IDX of group 08
#else
    MW_FIM_IDX_GP11_PROJECT_START = 0x01010000,             // the start IDX of group 11
#endif

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
    MW_FIM_IDX_GP11_PROJECT_SSID_PASSWORD,

    MW_FIM_IDX_GP11_PROJECT_FIXED_SSID_PASSWORD,
#endif

    MW_FIM_IDX_GP11_PROJECT_MAX
} E_MwFimIdxGroup11_Project;


/******************************
Declaration of data structure
******************************/
// Sec 3: structure, uniou, enum, linked list

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
typedef struct
{
    uint8_t password[WIFI_LENGTH_PASSPHRASE]; //WIFI_LENGTH_PASSPHRASE=64    /**< The password of the target AP. >**/
    uint8_t ssid[WIFI_MAX_LENGTH_OF_SSID];    //WIFI_MAX_LENGTH_OF_SSID=32+1 /**< The SSID of the target AP. >**/
    uint8_t weight;                                                          /**< The flag of seqno overflow >**/
    uint8_t seqno;
    uint8_t used;
} T_MwFim_GP11_Ssid_PWD;

#define MW_FIM_GP11_SSID_PWD_SIZE  sizeof(T_MwFim_GP11_Ssid_PWD)
#define MW_FIM_GP11_SSID_PWD_NUM   1

#define MW_FIM_GP11_FIXED_SSID_PWD_SIZE  sizeof(T_MwFim_GP11_Ssid_PWD)
#define MW_FIM_GP11_FIXED_SSID_PWD_NUM   1
#endif

/********************************************
Declaration of Global Variables & Functions
********************************************/
// Sec 4: declaration of global variable
extern const T_MwFimFileInfo g_taMwFimGroupTable11_project[];

// Sec 5: declaration of global function prototype


/***************************************************
Declaration of static Global Variables & Functions
***************************************************/
// Sec 6: declaration of static global variable


// Sec 7: declaration of static function prototype


#ifdef __cplusplus
}
#endif

#endif // _MW_FIM_DEFAULT_GROUP11_PROJECT_H_
