#ifndef __SSIDPWD_PROC_H__
#define __SSIDPWD_PROC_H__

#include  "cmsis_os.h"
#include "mw_fim_default_group11_project.h"

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
typedef struct {
    T_MwFim_GP11_Ssid_PWD tSsidPwd;
    uint8_t     bssid[6];
    uint8_t     dummy[2];
} apinfo_t;

void SsidPwdInit(void);
int SsidPwdAdd(T_MwFim_GP11_Ssid_PWD tNewSsidPwd);
void SsidPwdClear(void);
#endif

#endif /* __SSIDPWD_PROC_H__ */
