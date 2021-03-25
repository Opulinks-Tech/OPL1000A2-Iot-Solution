#include <stdlib.h>
#include <string.h>
#include "ssidpwd_proc.h"

#include "mw_fim.h"
#include "blewifi_common.h"

#if (BLEWIFI_CTRL_SSID_ROAMING_EN == 1)
/*
Enhancement (optional):
1. If SSID is same and PWD is different, add new one or only update password. (now : only update password)
2. If SSID is same and only update passwrd, update seqno and weight as max or not. (now : not update)
3. Roming connect sequence is by min > max, max > min or index. (now : index)
*/

extern const T_MwFim_GP11_Ssid_PWD g_tMwFimDefaultGp11SsidPwd;
extern const T_MwFim_GP11_Ssid_PWD g_tMwFimDefaultGp11FixedSsidPwd;

/* Queue structure */

apinfo_t g_apInfo[MW_FIM_GP11_FIXED_SSID_PWD_NUM+MW_FIM_GP11_SSID_PWD_NUM];
uint32_t g_u32MinSeqIdx;
uint32_t g_u32MaxSeqIdx;
uint32_t g_RoamingApInfoTotalCnt;

/*
Compare apinfo by weight and seq
If ap1 > ap2, return 0, else, return 1

If weight same, ap1.seqno > ap2.seqno, return 0
If weight diff, ap1.seqno < ap2.seqno, return 0
*/
static int entry_compare(T_MwFim_GP11_Ssid_PWD ap1,T_MwFim_GP11_Ssid_PWD ap2)
{
    //printf("[%s,%d]\n", __FUNCTION__, __LINE__);
    if ( ap1.weight == ap2.weight )
    {
        //printf("[%s,%d] weight same\n", __FUNCTION__, __LINE__);
        if (ap1.seqno > ap2.seqno)
        {
            //printf("[%s,%d] %d > %d\n", __FUNCTION__, __LINE__, t1.seq, t2.seq);
            return 0;
        }
        else
        {
            //printf("[%s,%d] %d < %d\n", __FUNCTION__, __LINE__, t1.seq, t2.seq);
            return 1;
        }
    }
    else
    {
        //printf("[%s,%d]weight diff\n", __FUNCTION__, __LINE__);
        if (ap1.seqno < ap2.seqno)
        {
            //printf("[%s,%d] %d > %d\n", __FUNCTION__, __LINE__, t1.seq, t2.seq);
            return 0;
        }
        else
        {
            //printf("[%s,%d] %d < %d\n", __FUNCTION__, __LINE__, t1.seq, t2.seq);
            return 1;
        }
    }
}

static void SortMinAndMax(void)
{
    int i;
    g_u32MinSeqIdx = 1;
    g_u32MaxSeqIdx = 1;

    if (g_RoamingApInfoTotalCnt > 0)
    {
        // Find min seqno, max seqno
        g_u32MinSeqIdx = 1;
        g_u32MaxSeqIdx = 1;
        for ( i=1; i<(MW_FIM_GP11_FIXED_SSID_PWD_NUM+MW_FIM_GP11_SSID_PWD_NUM) ; i++ )
        {
            // check min
            if (entry_compare(g_apInfo[g_u32MinSeqIdx].tSsidPwd, g_apInfo[i].tSsidPwd) == 0)
            {
                g_u32MinSeqIdx = i;
            }
            else
            {
                // check max
                if (entry_compare(g_apInfo[i].tSsidPwd, g_apInfo[g_u32MaxSeqIdx].tSsidPwd) == 0)
                {
                    g_u32MaxSeqIdx = i;
                }
            }
        }
    }
    printf("min[%d]:%d, max[%d]:%d\n", g_u32MinSeqIdx, g_apInfo[g_u32MinSeqIdx].tSsidPwd.seqno, g_u32MaxSeqIdx, g_apInfo[g_u32MaxSeqIdx].tSsidPwd.seqno);
    printf("\n\n");
}

void SsidPwdInit()
{
    int i;
    g_RoamingApInfoTotalCnt = 0;
    memset(g_apInfo, 0, sizeof(g_apInfo));

    //get Fixed SSID PASSWORD
    if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP11_PROJECT_FIXED_SSID_PASSWORD, 0, MW_FIM_GP11_FIXED_SSID_PWD_SIZE, (uint8_t*)&(g_apInfo[0].tSsidPwd)))
    {
        // if fail, get the default value
        BLEWIFI_ERROR("SsidPwdInit: MwFim_FileRead fail\n");
        memcpy(&(g_apInfo[0].tSsidPwd), &g_tMwFimDefaultGp11FixedSsidPwd, MW_FIM_GP11_SSID_PWD_SIZE);
    }
    //make sure fixed ssid is not default value
    if (1 == g_apInfo[0].tSsidPwd.used)
    {
        g_RoamingApInfoTotalCnt += 1;
        printf("APInfo[0]=%s\n", g_apInfo[0].tSsidPwd.ssid);
    }


    for(i = 0; i < MW_FIM_GP11_SSID_PWD_NUM; i++)
    {
        if (MW_FIM_OK != MwFim_FileRead(MW_FIM_IDX_GP11_PROJECT_SSID_PASSWORD, i, MW_FIM_GP11_SSID_PWD_SIZE, (uint8_t*)&(g_apInfo[i+1].tSsidPwd)))
        {
            // if fail, get the default value
            BLEWIFI_ERROR("SsidPwdInit: MwFim_FileRead fail\n");
            memcpy(&(g_apInfo[i+1].tSsidPwd), &g_tMwFimDefaultGp11SsidPwd, MW_FIM_GP11_SSID_PWD_SIZE);
        }

        //make sure ssid is not default value
        if(1 == g_apInfo[i+1].tSsidPwd.used)
        {
            //printf("APInfo[%d]=%s, w[%d], s[%d]\n", i+1, g_apInfo[i+1].tSsidPwd.ssid, g_apInfo[i+1].tSsidPwd.weight, g_apInfo[i+1].tSsidPwd.seqno);
            printf("APInfo[%d]=%s, w[%d], s[%d], u[%d]\n", i+1, g_apInfo[i+1].tSsidPwd.ssid, g_apInfo[i+1].tSsidPwd.weight, g_apInfo[i+1].tSsidPwd.seqno, g_apInfo[i+1].tSsidPwd.used);
            g_RoamingApInfoTotalCnt+=1;
        }
    }

    SortMinAndMax();
}

int SsidPwdAdd(T_MwFim_GP11_Ssid_PWD tNewSsidPwd)
{
    int idx;
    if ( MW_FIM_GP11_SSID_PWD_NUM <= 0 )
        return 0;

    if (1 == g_apInfo[0].tSsidPwd.used)
    {
        if((0 == strcmp((const char*)tNewSsidPwd.ssid, (const char*)g_apInfo[0].tSsidPwd.ssid) ) &&
                (0 == strcmp((const char*)tNewSsidPwd.password, (const char*)g_apInfo[0].tSsidPwd.password)))
        {
            printf("[SsidPwdAdd]ssid is the same as fixed ssid, no Save!\n");
            return 0;
        }
    }

    for(idx = 0; idx < MW_FIM_GP11_SSID_PWD_NUM; idx++)
    {
        if (1 == g_apInfo[idx+1].tSsidPwd.used)
        {
           if (0 == strcmp((const char*)g_apInfo[idx+1].tSsidPwd.ssid, (const char*)tNewSsidPwd.ssid))
           {
               if (0 != strcmp((const char*)g_apInfo[idx+1].tSsidPwd.password, (const char*)tNewSsidPwd.password))
               {
                   //printf("\n[SsidPwdAdd]ssid is the same, but password is different\n");
                   memcpy((void*)&g_apInfo[idx+1].tSsidPwd.password, (void*)&tNewSsidPwd.password, sizeof(tNewSsidPwd.password));
                   printf("\n[SsidPwdAdd]update PWD, SSID(Index=%d):%s\n", idx+1, tNewSsidPwd.ssid);
                   if (MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP11_PROJECT_SSID_PASSWORD, idx, MW_FIM_GP11_SSID_PWD_SIZE, (uint8_t*)&(g_apInfo[idx+1].tSsidPwd)))
                   {
                       BLEWIFI_ERROR("SsidPwdAdd: MwFim_FileWrite fail\n");
                   }
               }
               return 0;
           }
        }
    }

    if ((g_apInfo[g_u32MaxSeqIdx].tSsidPwd.seqno + 1) == (MW_FIM_GP11_SSID_PWD_NUM))
    {
        tNewSsidPwd.weight = (g_apInfo[g_u32MinSeqIdx].tSsidPwd.weight + 1) % 2;
        tNewSsidPwd.seqno = (g_apInfo[g_u32MaxSeqIdx].tSsidPwd.seqno + 1) % (MW_FIM_GP11_SSID_PWD_NUM);
    }
    else
    {
        tNewSsidPwd.weight = g_apInfo[g_u32MaxSeqIdx].tSsidPwd.weight;
        tNewSsidPwd.seqno = g_apInfo[g_u32MaxSeqIdx].tSsidPwd.seqno + 1;
    }
    tNewSsidPwd.used = 1;
    
    memset((void*)&g_apInfo[g_u32MinSeqIdx].tSsidPwd, 0, sizeof(T_MwFim_GP11_Ssid_PWD));
    memcpy((void*)&g_apInfo[g_u32MinSeqIdx].tSsidPwd, (void*)&tNewSsidPwd, sizeof(T_MwFim_GP11_Ssid_PWD));

    printf("\n[SsidPwdAdd]add SSID(Index=%d):%s, w[%d], s[%d]\n", g_u32MinSeqIdx, g_apInfo[g_u32MinSeqIdx].tSsidPwd.ssid, g_apInfo[g_u32MinSeqIdx].tSsidPwd.weight, g_apInfo[g_u32MinSeqIdx].tSsidPwd.seqno);
    if (MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP11_PROJECT_SSID_PASSWORD, g_u32MinSeqIdx-1, MW_FIM_GP11_SSID_PWD_SIZE, (uint8_t*)&(g_apInfo[g_u32MinSeqIdx].tSsidPwd)))
    {
        BLEWIFI_ERROR("SsidPwdAdd: MwFim_FileWrite fail\n");
    }

    if (1 == g_apInfo[0].tSsidPwd.used)
    {
       if ( g_RoamingApInfoTotalCnt < (MW_FIM_GP11_FIXED_SSID_PWD_NUM+MW_FIM_GP11_SSID_PWD_NUM))
       {
           g_RoamingApInfoTotalCnt = g_RoamingApInfoTotalCnt + 1;
       }
    }
    else
    {
        if ( g_RoamingApInfoTotalCnt < MW_FIM_GP11_SSID_PWD_NUM)
        {
            g_RoamingApInfoTotalCnt = g_RoamingApInfoTotalCnt + 1;
        }
    }

    SortMinAndMax();

    return 0; // No errors
}

void SsidPwdClear()
{
    int idx;
    for(idx = 0; idx < MW_FIM_GP11_SSID_PWD_NUM; idx++)
    {
        memset((void *)&g_apInfo[idx+1], 0, sizeof(apinfo_t));
        if (MW_FIM_OK != MwFim_FileWrite(MW_FIM_IDX_GP11_PROJECT_SSID_PASSWORD, idx, MW_FIM_GP11_SSID_PWD_SIZE, (uint8_t*)&(g_apInfo[idx+1].tSsidPwd)))
        {
            BLEWIFI_ERROR("SsidPwdAdd: MwFim_FileWrite fail\n");
        }        
    }
    if (1 == g_apInfo[0].tSsidPwd.used)
    {
        g_RoamingApInfoTotalCnt = 1;
    }
    else
    {
        g_RoamingApInfoTotalCnt = 0;
    }
}
#endif
