/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2005
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE. 
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

/*******************************************************************************
 *
 * Filename:
 * ---------
 * MTSOC.c
 *
 * Project:
 * --------
 *   MAUI
 *
 * Description:
 * ------------
 *
 *      FM Radio Driver (for soc chip)
 *
 * Author:
 * -------
 * -------
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * removed!
 * removed!
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 *******************************************************************************/
#if defined(MT6256FM)||defined(MT6255FM)
#define FM_RADIO_SOC
#endif
#if defined(FM_RADIO_SOC)
#include "MTSOC.h"
#include "us_timer.h"
#include "drv_comm.h"
#include "drv_hisr.h"
#include "intrCtrl.h"
#include "dcl.h"
#include "kal_trace.h"
#include "kal_public_api.h"
#include "l1_interface.h"
#include "reg_base.h"
#include "app_ltlcom.h"
#include "stack_ltlcom.h"
#include "stack_config.h"
#include "stack_msgs.h"
#include "stack_common.h"
#include "fmr_trc.h"
#include "fm_radio.h"
#include "fs_gprot.h"//for FS API

extern void L1SM_SleepDisable( kal_uint8 handle );
extern void L1SM_SleepEnable( kal_uint8 handle );
extern kal_uint8 L1SM_GetHandle ( void );

static kal_uint32 _drv_fm_l1sm_handle = 0xffffffff;

extern FMR_data *pstFMR_data;
extern kal_uint16 FMR_RSSI_THRESHOLD_LANT;
extern kal_uint16 FMR_RSSI_THRESHOLD_SANT;
extern kal_uint16 FMR_CQI_TH;
extern kal_uint16 FMR_SCAN_BAND_UP; 
extern kal_uint16 FMR_SCAN_BAND_DN; 
DCL_HANDLE dcl_handle = DCL_HANDLE_INVALID;
/// Global variables for current FM status
kal_int16 _current_frequency = -1;
kal_int16 _backup_frequency = -1;
static kal_bool  _is_fm_on = KAL_FALSE;
static kal_uint16 Chip_ID = 0;
kal_uint8 HWSearch_flag = 0;
static kal_uint8 Valid_flag = 0;
static kal_uint16 FreqKHz = 0;
kal_uint16 FreqSt = 987;
kal_bool FM_Data_debug = KAL_TRUE;
static kal_bool TuneAgainFlag = KAL_FALSE;

#if defined (__FM_RADIO_RDS_SUPPORT__)
extern kal_bool isRdsOn;
#endif

#if defined(MT6256FM)
#define POWER_ON_COMMAND_COUNT 163
#elif defined(MT6255FM)
#define POWER_ON_COMMAND_COUNT 165
#else/*synch latest chip*/
#define POWER_ON_COMMAND_COUNT 165
#endif
static const ctrl_word_operation PowerOnSetting[POWER_ON_COMMAND_COUNT] =
{
///RFANA OFF-->RX (Short Antenna)
   { 0x01, 0, 0x4A00 }, //A01:Turn on the bandgap and central biasing core
   { 0xFFFA, 0, 0x001E }, //wait 30us
   { 0x01, 0, 0x6A00 },
   { 0xFFFA, 0, 0x0032 }, //wait 50us
   { 0x02, 0, 0x299C },//A02:Initialise the LDO's Output
   { 0x01, 0, 0x6B82 },//A03:Enable RX, ADC and ADPLL LDO
#ifdef FM_PowerOn_with_ShortAntenna   
   { 0x04, 0, 0x0145 },//A04:Update FMRF optimized register settings /*Short Antenna*/
   { 0x05, 0, 0x00FF }, /*Short Antenna*/
#else
   { 0x04, 0, 0x0142 },//A04:Update FMRF optimized register settings
   { 0x05, 0, 0x00E7 },
#endif
   { 0x0A, 0, 0x0060 },
   { 0x0C, 0, 0xAC8C },
   { 0x0D, 0, 0x0888 },
   { 0x10, 0, 0x0E8D },
   { 0x27, 0, 0x0004 },
   { 0x0E, 0, 0x0040 },
   { 0x03, 0, 0x9860 },
   { 0x3F, 0, 0xAD16 },
   { 0x3E, 0, 0x3280 },
   { 0x06, 0, 0x0125 },   
   { 0x08, 0, 0x15B8 },
   { 0x28, 0, 0x0000 },//A05:Enable RX related blocks
   { 0x00, 0, 0x0166 },
   { 0x3A, 0, 0x0004 },      
   { 0x37, 0, 0x0590 }, //Set FMCR_POW_RX_LODIV = '1'    
/*FM ADPLL Power Up*/
#if defined(APLL_Cal_Open_Loop_Mode)
#else
#if defined(MT6256FM)
   { 0x25, 0, 0x0403 },
   { 0x20, 0, 0x2320 },//C0:Remove the Reset_N
   { 0x22, 0, 0x7780 },//C1:Change DLF loop gain
   { 0x25, 0, 0x0803 },//C2:Configure initial I_CODE for calibration
#elif defined(MT6255FM)
	{0x38, 0, 0x0002},//C0.a:adpll ref_clk to 128K
	{0x39, 0, 0x0000},//C0.a:setup design robustness preferences
	{0x20, 0, 0x2320},//C0.b:Remove the Reset_N
	{0x1E, 0, 0x0860},//C0.c:select the frequency option:61.44M or 65.536M
	{0x22, 0, 0x7780},//C1:Change DLF loop gain
	{0x25, 0, 0x0802},//C2:Configure initial I_CODE for calibration
#else//synch latest chip
	{0x38, 0, 0x0000},//C0.a:setup design robustness preferences
	{0x39, 0, 0x0000},//C0.a:setup design robustness preferences
	{0x20, 0, 0x2320},//C0.b:Remove the Reset_N
	{0x1E, 0, 0x0860},//C0.c:select the frequency option:61.44M or 65.536M
	{0x22, 0, 0x7780},//C1:Change DLF loop gain
	{0x25, 0, 0x0802},//C2:Configure initial I_CODE for calibration
#endif
   { 0x1E, 0, 0x0863 },//C3:Enable ADPLL DCO, C4:Turn on Coarse calibration
   { 0xFFFB, 0, 0x000a }, // delay 5ms
   { 0xFFFF, 0x23, 0x8000 }, //C5: Polling FMR2D_DCO_CAL_STATUS = "1"
   { 0x1E, 0, 0x0865 },//C6:Turn on Fine_A calibration
   { 0xFFFB, 0, 0x000a }, // delay 5ms
   { 0xFFFF, 0x23, 0x8000 }, //C7: Polling FMR2D_DCO_CAL_STATUS = "1"
   { 0x1E, 0, 0x0871 },//C8:Enable Close-loop mode
   { 0xFFFB, 0, 0x0064 }, // delay 100ms
   { 0x2A, 0, 0x1022 },//C9:Disable fm adc ck top clock gating, Set rgfrf_en_top_cg ="1"
#endif  
/*FM RC Calibration*/
   { 0x00, 0, 0x01E6 }, //Set FMCR_POW_RCCAL = '1'
   { 0xFFFA, 0, 0x0001 }, //wait 1us
   { 0x1B, 0, 0x0094 }, //Set rgfrf_cal_filter = '1'
   { 0x1B, 0, 0x0095 }, //Set rgfrf_cal_filter = '1'
   { 0xFFFA, 0, 0x00C8 },  //wait 200us
   { 0x1B, 0, 0x0094 }, //Set rgfrf_cal_filter = '0'   
   { 0x00, 0, 0x0166 }, //Set FMCR_POW_RCCAL = '1' 
/*FM VCO Enable*/
   { 0x01, 0, 0x6B8A },
   { 0xFFFA, 0, 0x0001 },  //wait 1us
   { 0x00, 0, 0xC166 }, //Set FMCR_POW_AFCDAC = '1'
   { 0x0C, 0, 0xAC8C }, //Set FMCR_CONST_GM_EN = '1'
   { 0xFFFB, 0, 0x001E }, /// delay 30ms
   { 0x00, 0, 0xF166 }, //Set FMCR_POW_LOBUF =  '1'
   { 0x09, 0, 0x2964 }, //Disable auto SCAL
#ifdef FM_PowerOn_with_ShortAntenna
   { 0x26, 0, 0x0024 }, //Set FMCR_SCAL_GM_EN = '1'
#else
   { 0x2E, 0, 0x0008 }, //Set rgfrf_scal_hwtrig_dis = 1
#endif
/*FM DCOC Calibration*/
   { 0xCB, 0, 0x00B0 },
   { 0x64, 0, 0x0001 },
   { 0x63, 0, 0x0020 },
   { 0x9C, 0, 0x0044 },
   { 0x0F, 0, 0x1A08 },
   { 0x63, 0, 0x0021 },
   { 0xFFFF, 0x69, 0x0001 }, //Polling fm_intr_stc_done (69H D0) = "1"
   { 0x69, 0, 0x0001 },
   { 0x63, 0, 0x0000 },
   { 0xFFFE, 0x6F, 0x0001 }, //Polling stc_done (6FH D0) = "0"
/*Others*/
   { 0x00, 0, 0xF167 }, //Enable LNA
   { 0xFFFB, 0, 0x0032 }, // delay 50ms
   { 0x2b, 0, 0x0032 },  
   { 0x2c, 0, 0x0019 },   
   { 0x11, 0, 0x17D4 },  
///FMSYS Digital Initialization Sequences
   { 0xFFFD, 0x0062, 0x0000 }, //A2:Readback HWVER code for sanity check of register access
   { 0x64, 0, 0x0001 }, //A3:Enable HW Modem auto control
   { 0x63, 0, 0x0680 }, //A4:Enable Hardware controlled options:
   { 0x67, 0, 0x0000 }, //A6:[9:0]Set Channel Lower Bound
   { 0x66, 0, 0x2800 }, //A7:[9:0]Set Channel Upper Bound
   { 0x6A, 0, 0x0002 }, //A9:Enable iqcal_done interrupt mask
   { 0x6B, 0, 0x0002 }, 
   { 0x6D, 0, 0x1AB2 }, //A12:Mainfsm guard time setting
   { 0x6A, 0, 0x0000 }, //A15:Set appropriate interrupt mask behavior as desired
   { 0x6B, 0, 0x0000 }, //A15:Set appropriate externalinterrupt mask behavior as desired
   { 0xFFFB, 0, 0x0032 }, /// delay 50ms
///FM Digital Init: fm_rgf_dac
   { 0x9C, 0, 0x0048 }, 
   { 0x9E, 0, 0x2B24 },
   { 0x71, 0, 0x107F },//{ 0x71, 0, 0x607F }
   { 0x72, 0, 0x878F },
   { 0x73, 0, 0x07C1 },    
///FM Digital Init: fm_rgf_front
   { 0x9F, 0, 0x0000 },
   //{ 0xB4, 0, 0x8810 },    
   { 0xB8, 0, 0x006A }, 
   { 0xBB, 0, 0x006B }, 
   { 0xCB, 0, 0x00B0 }, 
#ifdef FM_PowerOn_with_ShortAntenna
    { 0xE0, 0, 0xA2E0 }, 
#else
    { 0xE0, 0, 0xA301 }, 
#endif
   { 0xE3, 0, 0x01BD },
   { 0xE4, 0, 0x008F },
   { 0xCC, 0, 0x0886 }, 
   { 0xDC, 0, 0x036A }, 
   { 0xDD, 0, 0x836A },
///FM Digital Init: update AFC gain
   { 0x96, 0, 0x41E2 },
   { 0x97, 0, 0x049A },    
   { 0x98, 0, 0x0B66 }, 
   { 0x99, 0, 0x0E1E }, 
   { 0xD0, 0, 0x0233 }, 
   { 0xD1, 0, 0x00BC }, 
   { 0x90, 0, 0x03FF },
   { 0x91, 0, 0x01BE }, 
   { 0x92, 0, 0x03FF }, 
   { 0x93, 0, 0x0354 },
   { 0x94, 0, 0x03FF }, 
   { 0x95, 0, 0x0354 },   
///FM Digital Init: fm_rgf_fmx
   { 0x9F, 0, 0x0001 }, 
   { 0xCF, 0, 0x0031 }, 
   { 0xEF, 0, 0x0033 }, 
   { 0xD1, 0, 0x3810 },
   { 0x9F, 0, 0x0002 }, 
   { 0xA0, 0, 0x0002 }, 
   { 0x9F, 0, 0x0002 },
   { 0xB2, 0, 0x00E8 }, 
   { 0xB3, 0, 0xFF28 }, 
   { 0xB4, 0, 0xFF28 },
   { 0xB5, 0, 0x00B8 }, 
   { 0xB6, 0, 0x0032 }, 
   { 0xB7, 0, 0x0032 }, 
   { 0xB8, 0, 0x0099 },
   { 0xB9, 0, 0x0019 },
   { 0xBA, 0, 0x0019 }, 
   { 0xBB, 0, 0x006D }, 
   { 0xBC, 0, 0x0035 }, 
   { 0xBD, 0, 0x0035 },
   { 0x9F, 0, 0x0002 }, 
   { 0xA0, 0, 0x0002 }, 
   { 0x9F, 0, 0x0002 },
   { 0xBE, 0, 0x0098 }, 
   { 0xBF, 0, 0xFF4E }, 
   { 0xC0, 0, 0xFF4E },
   { 0xC1, 0, 0x006D }, 
   { 0xC2, 0, 0x0040 }, 
   { 0xC3, 0, 0x0040 }, 
   { 0xC4, 0, 0x000B },
   { 0xC5, 0, 0x001E },
   { 0xC6, 0, 0x001E }, 
   { 0xC7, 0, 0x0028 }, 
   { 0xC8, 0, 0x0006 }, 
   { 0xC9, 0, 0x0006 },
   { 0xCA, 0, 0x014A }, 
   { 0xCB, 0, 0xFF83 }, 
   { 0xCC, 0, 0xFC75 }, 
   { 0xCD, 0, 0x0129 },
   { 0xCE, 0, 0xFFFC }, 
   { 0xCF, 0, 0xFE67 }, 
   { 0xD0, 0, 0x0119 },
   { 0xD1, 0, 0x0005 },
   { 0xD2, 0, 0xFED3 },
   { 0x9F, 0, 0x0001 },
   { 0xC2, 0, 0xFFCB }, 
   { 0xC4, 0, 0x0008 }, 
   { 0x9F, 0, 0x0002 },
   { 0xF1, 0, 0x1F42 }, 
   { 0xF2, 0, 0x218F }, 
   { 0x9F, 0, 0x0003 },
   { 0xE9, 0, 0x0080 }, 
   { 0x9F, 0, 0x0000 },
   { 0x9F, 0, 0x0002 },
   { 0xA1, 0, 0x1F42 },
   { 0xA2, 0, 0x218F }, 
   { 0x9F, 0, 0x0000 },
///FM Digital Init: SSR improve
   { 0x9F, 0, 0x0001 },
   { 0xBB, 0, 0x20B0 },
   { 0x9F, 0, 0x0000 },
};
#define POWER_OFF_COMMAND_COUNT 20
static const ctrl_word_operation PowerOffProc[POWER_OFF_COMMAND_COUNT] = {
 //Digital Modem Power Down
   {0x63, 0, 0x0000 }, /// Clear all hw control bits
   {0x6E, 0xFFFC, 0x0000 }, ///digital core + digital rgf reset  [1:0]  0
   {0x6E, 0xFFFC, 0x0000 }, ///digital core + digital rgf reset  [1:0]  0
   {0x6E, 0xFFFC, 0x0000 }, ///digital core + digital rgf reset  [1:0]  0
   {0x6E, 0xFFFC, 0x0000 }, ///digital core + digital rgf reset  [1:0]  0
   {0x2A, 0, 0x0020 }, ///Enable top fm adc clk gating
   {0x1E, 0, 0x0860 }, ///Turn off ADPLL circuits
   {0x20, 0, 0x0720 }, ///reset adpll
   {0x20, 0, 0x2720 }, ///deassert reset
   {0x00, 0, 0x0000 }, ///Turn off all the FM receiver circuits
   {0x01, 0, 0x0000 }, ///Turn off all the LDOs
   {0x04, 0, 0x0141 },
   {0x09, 0, 0x0964 },
   {0x09, 0, 0x0964 },
   {0x0C, 0, 0x288C },
   {0x26, 0, 0x0004 },
   {0x3A, 0, 0x0000 },
   {0x3B, 0, 0x00C3 },
   {0x3E, 0, 0x3280 },
   {0x3F, 0, 0x4E16 },  
};

void Delayus(kal_uint32 u4MicroSec)
{
    kal_uint32             start_qbit;      ///< Starting TDMA Qbit
    kal_uint32             curr_qbit;       ///< Current TDMA Qbit
    kal_uint32             elapse_qbit;     ///< Elapse number of Qbit

#if 0/*before system init done, FM won't be in use.*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
#endif
    // Read start qbit
    start_qbit = ust_get_current_time();

    /*******************************************************************//**
     * Calculate how many qbit should be waited. Store the result to "delay".
     * delay = delay * 13 / 12 ~= delay * 13 * 21.3333 / 2^8
     * => Limit: Maximum delay is 15,449,521 us = 15,449 ms = 15 sec
     ***********************************************************************/

    // delay * 13 * 21.333 ~= delay * 278
    u4MicroSec = u4MicroSec * 278;

    // delay / 2^8 ~= (delay >> 8) + 1 (for saving code, add one more qbit in all cases. Actually we only have to do such thing when "delay & 255 == 1"
    u4MicroSec = (u4MicroSec >> 8) + 1;

    // wait until "delay" qbits are all elapsed
    do {
       curr_qbit   = ust_get_current_time();
       elapse_qbit = ust_get_duration(start_qbit, curr_qbit);
    } while (elapse_qbit < u4MicroSec);

    return;
}

static void Delayms(kal_uint32 data)
{
	FMR_TRACE0(FM_DELAYMS,"FM delay [%d]ms\n",data);
	while(data--)
	{
		Delayus(1000);
	}
}
void dcl_pmu_vcama_enable(kal_bool enable)
{
	PMU_CTRL_LDO_BUCK_SET_EN val;
	val.enable=enable;
	val.mod=VCAMA;
	if (dcl_handle == DCL_HANDLE_INVALID) {
		dcl_handle=DclPMU_Open(DCL_PMU, FLAGS_NONE);
	}
	DclPMU_Control(dcl_handle, LDO_BUCK_SET_EN, (DCL_CTRL_DATA_T *)&val);
	if(!enable)
	{
		DclPMU_Close(dcl_handle);
		dcl_handle = DCL_HANDLE_INVALID;
	}	
}

void dcl_pmu_set_vcama(PMU_VOLTAGE_ENUM vol)
{
	PMU_CTRL_LDO_BUCK_SET_VOLTAGE val;
	val.voltage=vol;
	val.mod=VCAMA;
	if (dcl_handle == DCL_HANDLE_INVALID)
	{
		dcl_handle = DclPMU_Open(DCL_PMU, FLAGS_NONE);
	}
	DclPMU_Control(dcl_handle, LDO_BUCK_SET_VOLTAGE, (DCL_CTRL_DATA_T *)&val);
}
/*************************************************************
 * This function is called when FM SUBSYS needs to be powered on. 
 * MTSOC FM SUBSYS power on/off sequnce
 * power on:  
 *************************************************************/
void MTSOC_FMSYS_PowerOn(void)
{
	//kal_uint32 active;
	FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%x\n",0xA10);
	FM_ENABLE_POWER();                      // 1. assert MM Power on bit
	Delayus(10);
	FM_MEM_ON();                         //    assert Memory power on bit
	Delayus(1);
	/*do
	{
		active = DRV_Reg(FM_PWR_ACK_REG);
	}while(!(active&FM_SYS_PWR_ACK));*/
	FM_CLK_ENABLE();
	Delayus(1);
	FM_DISABLE_ISOLATION();
	Delayus(1);
	FM_DISABLE_RESET();
	Delayus(1);
	FM_FSPI_MAS_BCLK_ENABLE();
	Delayus(1);
	//FM_RESET_MTCMOS();
	FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x\n",0xA10);
}

/*************************************************************
 * This function is called when FM SUBSYS needs to be powered off. 
 * MTSOC FM SUBSYS power on/off sequnce
 * power off:  
 *************************************************************/
void MTSOC_FMSYS_PowerOff(void)
{
	FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%x\n",0xA20);
	FM_FSPI_MAS_BCLK_DISABLE();
	Delayus(1);
	FM_ENABLE_RESET();
	Delayus(1);
	FM_ENABLE_ISOLATION();
	Delayus(1);
	FM_CLK_DISABLE();
	Delayus(1);
	FM_MEM_OFF();                         //    disable Memory power on bit
	Delayus(1);
	FM_DISABLE_POWER();                      // 0. disable MM Power on bit	
	Delayus(1);
	FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x\n",0xA20);
}

kal_bool MTSOC_ReadByte(kal_uint8 addr, kal_uint16 *data)
{
    kal_uint32 active;
	
    DRV_WriteReg(FSPI_MAS_ADDR_REG, (kal_int16)(0x100 | addr));
    DRV_WriteReg(FSPI_MAS_CFG2_REG, 0); //[2:0]Actual data reads = (TRAN LEN + 1)x 16b
    DRV_WriteReg(FSPI_MAS_CTRL_REG, 0x01);
    do
    {
        active= DRV_Reg(FSPI_MAS_CTRL_REG);
    } while (1 == active);
    *data = DRV_Reg(FSPI_MAS_RDDATA_REG);
	if(FM_Data_debug == KAL_TRUE)
	{
		FMR_TRACE1(FM_READ_DATA,"--read reg[%x]=[%x]--",addr,*data);
	}
	return KAL_TRUE;
}

kal_bool MTSOC_WriteByte(kal_uint8 addr, kal_uint16 data)
{
    kal_uint32 active;
	
    DRV_WriteReg(FSPI_MAS_ADDR_REG, addr);
    DRV_WriteReg(FSPI_MAS_WRDATA_REG, data);
    DRV_WriteReg(FSPI_MAS_CTRL_REG, 0x01);
    do
    {
        active = DRV_Reg(FSPI_MAS_CTRL_REG);
    } while (1 == active);
    /* Add nop to enlarge this function to 32bytes due to thumb patch limitation */  
	if(FM_Data_debug == KAL_TRUE)
	{
		FMR_TRACE1(FM_WRITE_DATA,"--write reg[%x]-- [%x]--",addr,data);
	}
   return KAL_TRUE;
}
/***********************************************************************
*  Turn on/off tune (internal)
*
*  parameter-->ON_OFF: 1:ON, 0:OFF
*         
***********************************************************************/
void MTSOC_TUNE_ON()
{
	kal_uint16 dataRead;
	MTSOC_ReadByte(FM_MAIN_CTRL, &dataRead);
	MTSOC_WriteByte(FM_MAIN_CTRL, (dataRead&0xFFFE)|TUNE);
}

/***********************************************************************
*  Seek on/off (internal)
*
*  parameter-->ON_OFF: 1:ON, 0:OFF
*         
***********************************************************************/
void MTSOC_SEEK_ON()
{
	kal_uint16 dataRead;
	MTSOC_ReadByte(FM_MAIN_CTRL, &dataRead);
	MTSOC_WriteByte(FM_MAIN_CTRL, (dataRead&0xFFFD)|SEEK);
}

/***********************************************************************
*  Scan on(internal)
*
*  parameter-->
*         
***********************************************************************/
void MTSOC_SCAN_ON()
{
	kal_uint16 dataRead;
	MTSOC_ReadByte(FM_MAIN_CTRL, &dataRead);
	MTSOC_WriteByte(FM_MAIN_CTRL, (dataRead&0xFFFB)|SCAN);
}

/***********************************************************************
*  Mute on/off (internal)
*
*  parameter-->ON_OFF: 1:ON, 0:OFF
*         
***********************************************************************/
void MTSOC_Mute_OnOff(kal_uint8 mute)
{
	kal_uint16 dataRead;

	FMR_TRACE0(FM_MUTE_ONOFF,"FM mute:[%d]\n",mute);
	MTSOC_ReadByte(0x9C,&dataRead);                           
	if (mute == 1)                                             
		MTSOC_WriteByte(0x9C, (dataRead&0xFFFC)|0x0008);  
	else                                                       
		MTSOC_WriteByte(0x9C, (dataRead&0xFFF7));
}

/***********************************************************************
*  Soft Mute Mode(internal)
*
*  parameter-->TYPE: 1:FAST MODE, 0:NORMAL MODE
*         
***********************************************************************/
void MTSOC_SoftMute_Mode(kal_uint8 TYPE)
{
	FMR_TRACE0(FM_SOFTE_MUTE_ONOFF,"FM softer mute:[%d]\n",TYPE);
  	MTSOC_WriteByte(FM_PG_SEL, 0x0001);
	if(TYPE)
	{
  		MTSOC_WriteByte(0xC8, 0x0101);
  		MTSOC_WriteByte(0xA0, 0x0562);
  		MTSOC_WriteByte(0xD8, 0x0000);
	}
	else
	{
  		MTSOC_WriteByte(0xC8, 0x0232);
  		MTSOC_WriteByte(0xA0, 0x4562);
  		MTSOC_WriteByte(0xD8, 0x008B);
	}
	MTSOC_WriteByte(FM_PG_SEL, 0x0000);
}

/***********************************************************************
*  Hi-Lo Side Injection
*
***********************************************************************/
#define HiSideTableSize 1
void MTSOC_HL_Side_Adj(kal_int16 freq)
{
  kal_bool isHiSide = KAL_FALSE;
  kal_uint16 dataRead,indx;
  static kal_int16 Hi_Channels[1] =
{ 1042 };

  if(HiSideTableSize == 0)
  {
  return;
  }
  FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%x",0xA31);
  dataRead = sizeof(Hi_Channels)/sizeof(Hi_Channels[0]);
  for(indx=0; indx<dataRead; indx++)
  {
  	if(Hi_Channels[indx] == freq)
  	{
  		isHiSide = KAL_TRUE;
  	}
  }
  if(isHiSide)
  {
  	//Set high-side injection (AFC)
  	MTSOC_ReadByte(0x0F, &dataRead);
  	MTSOC_WriteByte(0x0F, dataRead |0x0400);
  	MTSOC_WriteByte(FM_PG_SEL, 0);
  	//Set high-side injection (DFE)
  	MTSOC_ReadByte(0xCB, &dataRead);
  	MTSOC_WriteByte(0xCB, dataRead | 0x01);
  	//MTSOC_WriteByte(0xCB, dataRead&0xFFFE);
  }
  else
  {
  	//Set low-side injection (AFC)
   	MTSOC_ReadByte(0x0F, &dataRead);
  	MTSOC_WriteByte(0x0F, dataRead&0xFBFF);
  	MTSOC_WriteByte(FM_PG_SEL, 0);
  	//Set low-side injection (DFE)
  	MTSOC_ReadByte(0xCB, &dataRead);
  	//MTSOC_WriteByte(0xCB, dataRead | 0x01);
  	MTSOC_WriteByte(0xCB, dataRead&0xFFFE);
  }
  FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x",0xA31);
}

/***********************************************************************
*  ADPLL Power Up
*
***********************************************************************/
void MTSOC_ADPLL_PowerUp(void)
{
    kal_uint16 reg_value, f16mode_enable;

    MTSOC_WriteByte(0x25, 0x0403);
    //Remove the Reset_N
    MTSOC_WriteByte(0x20, 0x2720);
    // change DLF loop gain  
    // Set FMCR_DLF_GAIN_A = "9"
    // Set FMCR_DLF_GAIN_B = "9"
    MTSOC_WriteByte(0x22, 0x9980);
    //Configure initial I_CODE for calibration
    MTSOC_WriteByte(0x25, 0x0803);
   
    // C1. Enable ADPLL DCO	Set
    // FMCR_DCO_EN = ��1��
    MTSOC_ReadByte(0x1E, &reg_value);
    f16mode_enable = reg_value&0x0200;
    MTSOC_WriteByte(0x1E, f16mode_enable|0x0861);    
    
    // C2. Turn on coarse calibration
    // Set FMCR_COARSE_EN = ��1��
    MTSOC_WriteByte(0x1E, f16mode_enable|0x0863);    
    // wait 5ms 
    Delayms(5);     

    // C3. Check DCO calibration status	
    // Poll FMR2D_DCO_CAL_STATUS = "1"
    FM_Data_debug=KAL_FALSE;
    do
    {
        MTSOC_ReadByte(0x23, &reg_value);
    }while (0x0000 == (reg_value & 0x8000));
    FM_Data_debug=KAL_TRUE;
    
    // C4. Turn on fine_A calibration
    // Set FMCR_CAL_COARSE_EN = "0"
    MTSOC_WriteByte(0x1E, f16mode_enable|0x0861);
    // Set FMCR_FINE_A_EN = "1"
    MTSOC_WriteByte(0x1E, f16mode_enable|0x0865);
    // wait 5ms 
    Delayms(5);
    
    // C5. Check DCO calibration status
    // Poll FMR2D_DCO_CAL_STATUS = "1"   
    FM_Data_debug=KAL_FALSE;
    do
    {
        MTSOC_ReadByte(0x23, &reg_value);
    }while (0x0000 == (reg_value & 0x8000));
    FM_Data_debug=KAL_TRUE;
    
    // C6. Enable Close-loop mode
    // Set FMCR_FINE_A_EN = "0"
    MTSOC_WriteByte(0x1E, f16mode_enable|0x0861);
    // Set FMCR_PLL_EN = "1"
    MTSOC_WriteByte(0x1E, f16mode_enable|0x0871);
    
    // C9. Disable fm adc ck top clock gating
    // Set rgfrf_top_ck = "1"
    MTSOC_WriteByte(0x2A, 0x1022);      
}

/***********************************************************************
*  ADPLL Power Down
*
***********************************************************************/
void MTSOC_ADPLL_PowerDown(void)
{
    kal_uint16 reg_value;
    
    // 2. ADPLL Power Off Sequence
    // Set rgfrf_top_ck = "0"
    MTSOC_ReadByte(0x2A, &reg_value);
    reg_value &= ~0x1000; 
    MTSOC_WriteByte(0x2A, reg_value); 
    // Set FMCR_OPEN_LOOP_EN = "0"
    // Set FMCR_PLL_EN = "0"
    // Set FMCR_DCO_EN = "0"
    MTSOC_ReadByte(0x1E, &reg_value);
    reg_value &= ~0x0091;        
    MTSOC_WriteByte(0x1E, reg_value);       
    // Set rgfrf_adpll_reset_n = "0"
    MTSOC_ReadByte(0x20, &reg_value);
    reg_value &= ~0x2000;        
    MTSOC_WriteByte(0x20, reg_value);               
    // Set rgfrf_adpll_reset_n = "1"
    reg_value |= 0x2000;                
    MTSOC_WriteByte(0x20, reg_value);         
}

/***********************************************************************
*  Frequency Avoidance
*
***********************************************************************/
void MTSOC_Freq_Avoid(kal_int16 freq)
{
  kal_bool isADPLL_16M = KAL_FALSE;
  kal_uint16 dataRead,indx;
  static kal_int16 Avoid_Channels[29] =
  { 767, 768, 769,  806, 807, 821, 844, 845, 883, 884, 
     916, 917, 918, 919, 920, 921, 922, 923, 924, 925, 
     926, 927, 960, 998, 999, 1036, 1037, 1075, 1076};

  FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%x",0xA32);
  dataRead = sizeof(Avoid_Channels)/sizeof(Avoid_Channels[0]);
  for(indx=0; indx<dataRead; indx++)
  {
  	if(Avoid_Channels[indx] == freq)
  	{
		isADPLL_16M = KAL_TRUE;
  	}
  }
  //isADPLL_16M = 1;
  MTSOC_ReadByte(0x1E, &dataRead);
  if(((dataRead&0x0200)&&(isADPLL_16M))||(!(dataRead&0x0200)&&(!isADPLL_16M)))
  {
  return;
  }
  
  // Disable ADPLL
  MTSOC_ADPLL_PowerDown();
  
  //Set FMCR_DCO_CK_SEL = ? (default = 0, 15.36)
  MTSOC_ReadByte(0x1E, &dataRead);
  if(isADPLL_16M)
  {		
  	MTSOC_WriteByte(0x1E, dataRead|0x0200);
  }
  else
  {
  	dataRead &= ~0x0200;
  	MTSOC_WriteByte(0x1E, dataRead);
  }
  // Ensable ADPLL
  MTSOC_ADPLL_PowerUp();
  Delayms(100);
  //Set rgfrf_enable_top_ck ="1"
  MTSOC_ReadByte(0x2A, &dataRead);
  MTSOC_WriteByte(0x2A, dataRead|0x1000);
  if(isADPLL_16M)
  {		
  	//Set rgf_f16mode_en = X	
  	MTSOC_WriteByte(0x61, 3);
  	//Set rgf_cnt_resync_b = 0    
 	MTSOC_WriteByte(0x61, 1);      
        //Set rgf_cnt_resync_b = 1    
 	MTSOC_WriteByte(0x61, 3);
  }
  else
  {
  	//Set rgf_f16mode_en = X  		
  	MTSOC_WriteByte(0x61, 2);
  //Set rgf_cnt_resync_b = 0
 	MTSOC_WriteByte(0x61, 0);          
  //Set rgf_cnt_resync_b = 1
  	MTSOC_WriteByte(0x61, 2);
  } 
 
  // Power up FM modem clk
  //MTSOC_WriteByte(0x30, 0x0006);        
  //Set rgf_auto_mclk_sel_en = 1 (only if muclk desense option is enabled)
  /*MTSOC_WriteByte(FM_PG_SEL, 1);
  MTSOC_ReadByte(0x4A, &dataRead);
  MTSOC_WriteByte(0x4A, dataRead|0x0800);
  MTSOC_WriteByte(FM_PG_SEL, 0);*/
	FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x",0xA32);
}

/***********************************************************************
*  RAMP off (internal)
*         
***********************************************************************/
void MTSOC_RampDown()
{
	kal_uint16 dataRead,i=0;	
	FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%x\n",0xA34);
	MTSOC_ReadByte(FM_DAC_CON1, &dataRead);		
	MTSOC_WriteByte(FM_DAC_CON1, (dataRead&0xFF8F)| 0x0060);  
	//Enable RGF mute
	MTSOC_Mute_OnOff(1);
  	//Set softmute to fast mode
	MTSOC_SoftMute_Mode(1);
	Delayms(35);	
    MTSOC_ReadByte(FM_MAIN_INTR, &dataRead);		
   	MTSOC_WriteByte(FM_MAIN_INTR, (dataRead | FM_INTR_STC_DONE | FM_INTR_RDS));//clear status flag
    MTSOC_ReadByte(FM_MAIN_CTRL, &dataRead);		
  	MTSOC_WriteByte(FM_MAIN_CTRL, (dataRead&0xFFF0)); //clear rgf_tune/seek/scan/rxcal
	MTSOC_ReadByte(FM_MAIN_CHANDETSTAT, &dataRead);		
	FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%x\n",0xA340);
	FM_Data_debug = KAL_FALSE;
  	while( (dataRead & MASK_STC) == 1)
  	{
		MTSOC_ReadByte(FM_MAIN_CHANDETSTAT, &dataRead);
		if(i++>1000)
		{
			i=0;
			FMR_TRACE0(FM_ON_POLLING_TIMES,"power on polling 1000 times--%d",34);
			FMR_TRACE1( FM_INFO_READ_DATA, "Register [%x] =%x\n",FM_MAIN_INTR,dataRead);
		}
  	}  	  	
	FM_Data_debug = KAL_TRUE;
	FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x\n",0xA340);
	FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x",0xA34);
}

/***********************************************************************
*  Set radio frequency (internal)
*
*  parameter-->CurFreq:set frequency
*         
***********************************************************************/
static kal_bool MTSOC_SetFreq(kal_int32 CurFreq)
{
	kal_uint32 CHAN = 0x0000;
	kal_uint16 dataRead, cnt=0;
#if (defined(MT6255FM))//VCO calibration issue
	kal_uint16 temp;
#endif

	FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%x\n",0xA30);
	//MTSOC_RampDown();
#if (defined(MT6255FM))//VCO calibration issue
	MTSOC_ReadByte(0x28, &dataRead);
	MTSOC_WriteByte(0x28, dataRead|0x0040);
#endif
   	_current_frequency = CurFreq;
	CHAN = (CurFreq - 640)<<1;
	MTSOC_ReadByte(0x0F, &dataRead);
	MTSOC_WriteByte(0x0F, (dataRead&0xFC00)|CHAN);	
    MTSOC_ReadByte(FM_MAIN_INTR, &dataRead);		
   	MTSOC_WriteByte(FM_MAIN_INTR, (dataRead | FM_INTR_STC_DONE | FM_INTR_RDS));//clear status flag
    MTSOC_TUNE_ON();	
	FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%d\n",0xA300);
	FM_Data_debug = KAL_FALSE;
	do
	{
		MTSOC_ReadByte(FM_MAIN_INTR, &dataRead);
		if((dataRead & FM_INTR_STC_DONE) == 0)
		{
			kal_sleep_task(3);
		}
		if(cnt++ > 20)
		{
			FMR_TRACE(FM_INFO_TuneTimes, "setfreq: poll many times!\n");
			cnt = 0;
		}
	}while ((dataRead & FM_INTR_STC_DONE)==0);
	FM_Data_debug = KAL_TRUE;
	FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x\n",0xA300);
   	MTSOC_WriteByte(FM_MAIN_INTR, (dataRead | FM_INTR_STC_DONE | FM_INTR_RDS));//clear status flag
#if (defined(MT6255FM))//VCO calibration issue
	MTSOC_ReadByte(0x29, &dataRead);
	if((dataRead&0x0FFF)>0xF00)
	{
		FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%x\n",0xA3A);
		MTSOC_ReadByte(0x0B, &dataRead);
		temp = dataRead&0x03FF;
		MTSOC_ReadByte(0x17, &dataRead);
		MTSOC_WriteByte(0x17, dataRead&0xFC00|(temp+1));
		MTSOC_ReadByte(0x17, &dataRead);
		MTSOC_WriteByte(0x17, dataRead|0x8000);
		//tune again
		MTSOC_RampDown();
		MTSOC_ReadByte(0x0F, &dataRead);
		MTSOC_WriteByte(0x0F, (dataRead&0xFC00)|CHAN);
		MTSOC_TUNE_ON();	
		FM_Data_debug = KAL_FALSE;
		do
		{
			MTSOC_ReadByte(FM_MAIN_INTR, &dataRead);
			if((dataRead & FM_INTR_STC_DONE) == 0)
			{
				kal_sleep_task(3);
			}
			if(cnt++ > 20)
			{
				FMR_TRACE(FM_INFO_TuneTimes, "setfreq: poll many times!\n");
				cnt = 0;
			}
		}while ((dataRead & FM_INTR_STC_DONE)==0);
		FM_Data_debug = KAL_TRUE;
		MTSOC_WriteByte(FM_MAIN_INTR, (dataRead | FM_INTR_STC_DONE | FM_INTR_RDS));//clear status flag
		//tune done
		MTSOC_ReadByte(0x17, &dataRead);
		MTSOC_WriteByte(0x17, dataRead&(~0x8000));
	}
	MTSOC_ReadByte(0x28, &dataRead);
	MTSOC_WriteByte(0x28, dataRead&(~0x0040));
#endif   	
   	Delayms(35);//for RF stable
	//Disable mute
	MTSOC_Mute_OnOff(0);
   	Delayms(35);
  	//Set softmute to normal mode
  	MTSOC_SoftMute_Mode(0);
	FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x\n",0xA30);
	return KAL_TRUE;
}//MTSOC_SetFreq

/***********************************************************************
*  HiLo side Tune (internal)
*
*  parameter-->Freq: curf:875~1080
*							 band:range87.5~108.0
*         		 space:1:100k, 0:200k
***********************************************************************/
void MTSOC_TUNE_HiLo(kal_int32 Freq, kal_int16 band, kal_int8 space)
{
	kal_int8 rssi;
	kal_uint16 dataRead;
	
	MTSOC_Mute_OnOff(1);      
    	MTSOC_RampDown();		
      	
      	MTSOC_ReadByte(3, &dataRead);
      	MTSOC_WriteByte(3, ((dataRead&0xC7FF)|(space<<13))|band);//set space(100k/200k)and band(875~1080)
        
        //Read Low-Side LO Injection
  	//R11 --> clear  D15,  clear D0/D2,  D3 is the same as default
  	MTSOC_ReadByte(11, &dataRead);
  	MTSOC_WriteByte(11, (dataRead&0x7FFA));
  	if (MTSOC_SetFreq(Freq) == 0) 
      	{
         	ASSERT(0);
      	}    
      
       	MTSOC_ReadByte(ADDR_PAMD, &dataRead);
  	rssi = (dataRead & MASK_RSSI);
  	
  	//Read Hi-Side LO Injection
  	// R11-->set D15, set D0/D2,  D3 is the same as default
  	MTSOC_ReadByte(11, &dataRead);
  	MTSOC_WriteByte(11, (dataRead&0x8005));
  	if (MTSOC_SetFreq(Freq) == 0) 
      	{
         	ASSERT(0);
      	}  
      	
      	MTSOC_ReadByte(ADDR_PAMD, &dataRead);
      	rssi = rssi- (dataRead & MASK_RSSI);  
      	
      	if (rssi < 0) //errata in 0.82
  	{ 	
		// LO
		// R11--> clear D15, set D0/D2, D3 is the same as default
		MTSOC_ReadByte(11, &dataRead);
  		MTSOC_WriteByte(11, (dataRead&0x7FFF)|0x0005);
  	}else{ 
		//HI
		//R11-->  set D15, clear D0/D2, D3 is the same as default
		MTSOC_ReadByte(11, &dataRead);
  		MTSOC_WriteByte(11, (dataRead|0x8000)&0xFFFA);
  	}
  	
  	//fine-tune !!
  	//TUNE to FreqKHz with current setting
  	if (MTSOC_SetFreq(Freq) == 0) 
      	{
         	ASSERT(0);
      	}  
	
}
/***********************************************************************
*  Get RSSI Value (internal)
*
*  parameter-->
* retrun dBuVemf,
***********************************************************************/
kal_int16 MTSOC_GetCurRSSI(void)
{
    kal_uint16 TmpReg;
    kal_int16 RSSI, dBValue;
   MTSOC_ReadByte(ADDR_RSSI, &TmpReg);
   RSSI = (kal_int16)(TmpReg&0x3FF);
   /*RS=RSSI
    *If RS>511, then RSSI(dBm)= (RS-1024)/16*6
    *                 else RSSI(dBm)= RS/16*6             */
   dBValue = (RSSI>511) ? (((RSSI-1024)*6/16)+113):(RSSI*6/16+113);
   if(dBValue < 0)
   {
       dBValue = 0;
       //kal_prompt_trace(MOD_FMR,"RSSI=%d,dBValue=%d",RSSI,dBValue);
   }
   
   return dBValue;
  //return ((TmpReg & MASK_RSSI)>>9);
}

/***********************************************************************
*  Get PAMD Value (internal)
*
*  parameter-->
***********************************************************************/
kal_uint16 MTSOC_GetCurPamd(void)
{
   kal_uint16 TmpReg, PAMD, dBValue;
   MTSOC_ReadByte(ADDR_PAMD, &TmpReg);
   /*PA=PAMD
    *If PA>127 then PAMD(dB)=  (PA-256)/16*6,
    *               else PAMD(dB)=PA/16*6                 */
   PAMD = TmpReg&0xFF;
   dBValue = (PAMD>127) ? ((256-PAMD)*6/16):0;

   return dBValue;
}

/***********************************************************************
*  Get Search freq (internal)
*
*  parameter-->
***********************************************************************/
void MTSOC_GetSearchFreq(kal_uint16 *pfreq)
{
	*pfreq = (kal_uint16)FreqKHz;
}

/*********************************************************************
*  Set Antenna type
*  parameter-->Type: 1:Short Antenna, 0:Long Antenna
*
*********************************************************************/
kal_bool MTSOC_SetAntennaType(kal_uint8 isShortATA)
{
	if(isShortATA)
	{
		MTSOC_WriteByte(0x04, 0x0145);
		MTSOC_WriteByte(0x05, 0x00FF);
		MTSOC_WriteByte(0x26, 0x0024);
		MTSOC_WriteByte(0x2E, 0x0000);	
	}
	else
	{
		MTSOC_WriteByte(0x04, 0x0142);
		MTSOC_WriteByte(0x05, 0x00E7);
		MTSOC_WriteByte(0x26, 0x0004);
		MTSOC_WriteByte(0x2E, 0x0008);	
	}
	return KAL_TRUE;
}

/*********************************************************************
*  Get Antenna type
*  parameter
*  return-->KAL_TRUE:Short Antenna, KAL_FALSE:Long Antenna
*********************************************************************/
kal_bool MTSOC_GetAntennaType(void)
{
	kal_uint16 dataRead;
	
	MTSOC_ReadByte(4, &dataRead);
	if((dataRead&0x07) == 5)
		return KAL_TRUE; //short antenna
	else if((dataRead&0x07) == 2)
		return KAL_FALSE; //long antenna
	else
		return KAL_FALSE;
}

/***********************************************************************
*  FM Chip initial
*
*  parameter-->
***********************************************************************/
void FMDrv_ChipInit(void)
{
   	//uint16 dataRead;

 	// Init Reset/Power Pin to FM
 	MTSOC_FMSYS_PowerOff();
   
	if (0xffffffff == _drv_fm_l1sm_handle)
	{
		_drv_fm_l1sm_handle = L1SM_GetHandle();
	}
}

void FMDrv_LISR(void)
{
	IRQMask(IRQ_FM_CODE);
	drv_active_hisr(DRV_FMIF_HISR_ID);
}

void FMDrv_HISR(void)
{
	ilm_struct *fmr_ilm;
	//EINT_Set_Polarity(FM_EINT_NO, fm_state);//ALBERT_0123_02
      	//FMR_RDS_INTR_OnOff(0);
	DRV_SendPrimitive(fmr_ilm,
                            MOD_EINT_HISR,
                            MOD_FMR,
                            MSG_ID_FMR_RDS_IND,
                            NULL,
                            FMR_SAP);
	msg_send_ext_queue(fmr_ilm);
}

/***********************************************************************
*  FM Interrupt initialize
*
*  parameter-->
***********************************************************************/
void FMDrv_IntrInit(void)
{
#if (defined(FM_EINT_SUPPORT_SEARCH)) || (defined(__FM_RADIO_RDS_SUPPORT__))
	FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%x\n",0xA11);
	DRV_Register_HISR(DRV_FMIF_HISR_ID, FMDrv_HISR);
	IRQ_Register_LISR(IRQ_FM_CODE, FMDrv_LISR, "FM handler");
	IRQSensitivity(IRQ_FM_CODE,LEVEL_SENSITIVE);
	IRQUnmask(IRQ_FM_CODE);
#endif
}

/***********************************************************************
*  FMDrv_Get_stereo_mono
*
*  parameter-->
* RETURNS
*  0:mono 1:stereo
***********************************************************************/
kal_uint8 FMDrv_Get_stereo_mono(void)
{
   kal_uint16 TmpReg, value;
   MTSOC_WriteByte(FM_PG_SEL, 1);
   MTSOC_ReadByte(0xF8, &TmpReg);
   MTSOC_WriteByte(FM_PG_SEL, 0);
   value = (TmpReg&0x400)>>10;
	if(value==1)
	{
		return 0;
	}
   else
	{
		return 1;
	}
}
void FMDrv_GetCQILIst(kal_char **CQIList)
{
#ifdef __MTK_TARGET__	   
//	kal_uint32 i;
	kal_uint16 TmpReg;
	//RSSI
	MTSOC_ReadByte(0xE8, &TmpReg);
	kal_sprintf(CQIList[0],"0x%04x,",TmpReg);
	//MR
	MTSOC_ReadByte(0xF2, &TmpReg);
	kal_sprintf(CQIList[1],"0x%04x,",TmpReg);
	//PAMD
	MTSOC_ReadByte(0xE9, &TmpReg);
	kal_sprintf(CQIList[2],"0x%04x,",TmpReg);
	//freq offsec
	MTSOC_ReadByte(0xF9, &TmpReg);
	kal_sprintf(CQIList[3],"0x%04x,",TmpReg);
	//pilot check
	MTSOC_WriteByte(FM_PG_SEL, 1);
	MTSOC_ReadByte(0xF8, &TmpReg);
	kal_sprintf(CQIList[4],"0x%04x,",TmpReg);
	MTSOC_WriteByte(FM_PG_SEL, 0);
	
//	kal_sprintf(CQIList[i],"0x%x\n",0x1234);
#endif
}
/*FS test code*/
void FMDrv_Freq_CQI_Log(void)
{
#ifdef __MTK_TARGET__	   
	kal_int8 drive=FS_NO_ERROR;
	FS_DiskInfo DI;
	kal_int32 ret,i,j;
	kal_wchar driName[12];
	FS_HANDLE FS_handle = 0;
	kal_uint32 WrittenLength;
	//kal_uint8 teststring[7];
	kal_char FreqNo[6];
	kal_char *CQIList[5];
	kal_char Data[5][8];
	kal_uint8 CQINum=5;
	FM_Data_debug = KAL_FALSE;
   // if (((drive = FS_GetDrive(FS_DRIVE_V_REMOVABLE, 1, FS_NO_ALT_DRIVE ) >= 'A') && (drive <= 'Z'))
	drive = FS_GetDrive(FS_DRIVE_V_NORMAL/*FS_DRIVE_V_REMOVABLE*/, 1, FS_NO_ALT_DRIVE );
//	kal_prompt_trace(MOD_FMR,"%x.drive=%d=%c",FS_DRIVE_V_NORMAL,drive,(kal_uint8)drive);
	kal_wsprintf(driName,"%c:\\",drive);
//	kal_prompt_trace(MOD_FMR,"driName=%s",driName);
	ret = FS_GetDiskInfo((const kal_wchar *)driName,&DI,FS_DI_BASIC_INFO | FS_DI_FREE_SPACE);
//	if(ret > FS_NO_ERROR)
//	{
//		kal_prompt_trace(MOD_FMR,"DriveLetter=%d,WriteProtect=%d,TotalClusters=%d,FreeClusters=%d,SectorsPerCluster%d,BytesPerSector=%d",DI.DriveLetter,DI.WriteProtect,DI.TotalClusters,DI.FreeClusters,DI.SectorsPerCluster,DI.BytesPerSector);
//	}
//	else
//	{
//		kal_prompt_trace(MOD_FMR,"ERROR.ret=%d",ret);
//	}
	if(DI.WriteProtect)
	{
//		kal_prompt_trace(MOD_FMR,"ERROR:Disk write protected!!!");
		ASSERT(0);
	}

	for(i=0; i<CQINum; i++)
	{
		CQIList[i]=Data[i];
	}
	kal_wsprintf(driName,"%c:\\FMcqiLog",drive);
	FS_handle = FS_Open((const kal_wchar*)driName,FS_READ_WRITE|FS_CREATE);
//	kal_prompt_trace(MOD_FMR,"FS_handle=%d",FS_handle);
	if(FS_handle > FS_NO_ERROR)
	{
		ret = FS_Write(FS_handle, (void *)"Freq,  RSSI,     MR,   PAMD,   ATDC,  Polit",44,&WrittenLength);
		for(i=875; i<=1080; i++)
		{
			kal_sprintf(FreqNo,"\n%d,\0",i);
			ret = FS_Write(FS_handle, (void *)FreqNo,sizeof(FreqNo),&WrittenLength);
			if(ret < FS_NO_ERROR)
			{
				//error
			}
			FMDrv_SetFreq(i);
			FMDrv_GetCQILIst(CQIList);
			for(j=0; j<CQINum; j++)
			{
				ret = FS_Write(FS_handle, (void *)CQIList[j],sizeof(Data[j]),&WrittenLength);
				if(ret < FS_NO_ERROR)
				{
					//error
				}
			}
//			kal_prompt_trace(MOD_FMR,"WrittenLength=%d",WrittenLength);
		}
		FS_Close(FS_handle);
	}
	
	FM_Data_debug = KAL_TRUE;
#endif	
}
/***********************************************************************
*  Engineer mode function (API)
*
*  parameter-->group_idx: mono\stereo\RSSI_threshold\IF_count_delta
*              item_idx: sub select index
*              item_value: set parameter value
***********************************************************************/
void FMDrv_radio_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_uint32 item_value)
{
	kal_uint16 dataRead;
	
	switch (group_idx)
	{
		case mono:
			MTSOC_WriteByte(FM_PG_SEL, 1);
			MTSOC_ReadByte(0xe0, &dataRead); 
			if(item_value == 1)
				MTSOC_WriteByte(0xe0, (dataRead&0xFFFD)|0x02);
			else
				MTSOC_WriteByte(0xe0, (dataRead&0xFFFD));
			MTSOC_WriteByte(FM_PG_SEL, 0);
			break;
		case stereo:
			MTSOC_WriteByte(FM_PG_SEL, 1);
			if(item_value == 0)
			{
				MTSOC_ReadByte(0xe0, &dataRead); 
				MTSOC_WriteByte(0xe0, dataRead&0xFFFD);
			}
			else
			{
				switch (item_idx)
				{
					case Sblend_ON:
						MTSOC_ReadByte(0xe1, &dataRead); 
						MTSOC_WriteByte(0xe1, (dataRead&0x7FFF) |(item_idx<<15));
					break;
					case Sblend_OFF:
						MTSOC_ReadByte(0xe1, &dataRead); 
						MTSOC_WriteByte(0xe1, (dataRead&0x7FFF));
					break;
				}
			}
			MTSOC_WriteByte(FM_PG_SEL, 0);
			break;
		case RSSI_threshold:
			MTSOC_WriteByte(FM_PG_SEL, 1);
			MTSOC_ReadByte(0xd3, &dataRead); 
			MTSOC_WriteByte(0xd3, (dataRead&0xFC00) |item_value);
			MTSOC_WriteByte(FM_PG_SEL, 0);
		break;
		case HCC_Enable:
			MTSOC_WriteByte(FM_PG_SEL, 1);
			MTSOC_ReadByte(0xcf, &dataRead); 
			if(item_idx)
				MTSOC_WriteByte(0xcf, dataRead |0x10);
			else
				MTSOC_WriteByte(0xcf, dataRead&(~0x10));
			MTSOC_WriteByte(FM_PG_SEL, 0);
		break;
		case PAMD_threshold:
			MTSOC_WriteByte(FM_PG_SEL, 1);
			MTSOC_ReadByte(0xd4, &dataRead); 
			MTSOC_WriteByte(0xd4, (dataRead&0xFC00) |item_value);
			MTSOC_WriteByte(FM_PG_SEL, 0);
		break;
		case Softmute_Enable:
			MTSOC_WriteByte(FM_PG_SEL, 1);
			if(item_idx)
			{
				MTSOC_ReadByte(0xcf, &dataRead); 
				MTSOC_WriteByte(0xcf, dataRead |0x20);
			}
			else
			{
				MTSOC_ReadByte(0xcf, &dataRead); 
				MTSOC_WriteByte(0xcf, dataRead&(~0x20));		
			}
			MTSOC_WriteByte(FM_PG_SEL, 0);
		break;
		case De_emphasis:
			MTSOC_WriteByte(FM_PG_SEL, 2);
			if(item_idx == 2) //75us
			{
				MTSOC_ReadByte(0xa0, &dataRead); 
				MTSOC_WriteByte(0xa0, (dataRead&0xFFFC) |0x02);
			}
			else if(item_idx == 1) //50us
			{
				MTSOC_ReadByte(0xa0, &dataRead); 
				MTSOC_WriteByte(0xa0, (dataRead&0xFFFC) |0x01);		
			}
			else if(item_idx == 0) //0us
			{
				MTSOC_ReadByte(0xa0, &dataRead); 
				MTSOC_WriteByte(0xa0, (dataRead&0xFFFC) |0x00);		
			}
			MTSOC_WriteByte(FM_PG_SEL, 0);
		break;
		case HL_Side:
		break;

		case Demod_BW:
		break;
		case Dynamic_Limiter:
			MTSOC_WriteByte(FM_PG_SEL, 1);
				MTSOC_ReadByte(0xfa, &dataRead); 
			if(item_idx)
				MTSOC_WriteByte(0xfa, (dataRead&0xFFF7) |0x00);
			else
				MTSOC_WriteByte(0xfa, (dataRead&0xFFF7) |0x08);		
			MTSOC_WriteByte(FM_PG_SEL, 0);
		break;
		case Softmute_Rate:
			MTSOC_WriteByte(FM_PG_SEL, 1);
			MTSOC_ReadByte(0xc8, &dataRead); 
			MTSOC_WriteByte(0xc8, (dataRead&0x80FF) |(item_value<<8));
			MTSOC_WriteByte(FM_PG_SEL, 0);
		break;
		case AFC_Enable:
			MTSOC_ReadByte(0x33, &dataRead);
			if(item_idx)
				MTSOC_WriteByte(0x33, dataRead|0x0400);
			else
				MTSOC_WriteByte(0x33, dataRead&0xFBFF);
		break;
		case Softmute_Level:
			MTSOC_WriteByte(FM_PG_SEL, 1);
			MTSOC_ReadByte(0xd0, &dataRead);
			if(item_value > 0x24)
				item_value = 0x24;
			MTSOC_WriteByte(0xd0, (dataRead&0xFFC0) |item_value);
			MTSOC_WriteByte(FM_PG_SEL, 0);
		break;
		case Analog_Volume:
			MTSOC_ReadByte(0xb0, &dataRead);
			MTSOC_WriteByte(0xb0, (dataRead&0xF03F)|(item_value<<6));//[11:6]
		break;
		default:
		break;
	}
}

kal_uint32 FMDrv_GetCapArray(void)
{
    kal_uint16 Reg_Data;
    kal_uint8 i;
    kal_uint32 Capvalue=0;
	kal_uint32 chara[] =
    {166, 332, 664, 1330,
     2660, 5310, 10600, 21200};
    
    MTSOC_ReadByte(FM_CAP_ARRAY, &Reg_Data);
    Reg_Data = (Reg_Data>>6) & 0x1FF;
    for (i=0; i<8; i++)
    {
        if((Reg_Data >> i)& 0x0001)
        {
            Capvalue += chara[i];
        }
    }    
    return Capvalue;
}

kal_bool FMDrv_IsChipValid( void )
{
	/// anything to do?
	return KAL_TRUE;
}

kal_uint16 FMDrv_ReadByte(kal_uint8 addr)
{
	kal_uint16 Data;
	MTSOC_ReadByte(addr, &Data);
	return Data;
}

kal_bool FMDrv_WriteByte(kal_uint8 addr, kal_uint16 data)
{
	MTSOC_WriteByte(addr, data);
	return KAL_TRUE;
}

/// level ranges from 0 to 12
void FMDrv_SetVolumeLevel(kal_uint8 level)
{
  #ifdef MTSOC_DEBUG_DUMP_LOG
   kal_sprintf((void*)_dbg_str, "\nFMDrv_SetVolumeLevel(%d);\n\0", level);
   FS_Write(_file_handle, _dbg_str, strlen((void*)_dbg_str), &_data_written);
#endif
//   MTSOC_SetVolumeLevel(level);
}

kal_bool FMDrv_SetAntennaType(kal_uint8 ATAType)
{
	MTSOC_SetAntennaType(ATAType);
    return KAL_TRUE;
}

kal_uint8 FMDrv_GetAntennaType(void)
{
	return MTSOC_GetAntennaType();
}

void FMDrv_Mute(kal_uint8 mute)
{
	MTSOC_Mute_OnOff(mute);
}

/// Get interrupt status
void FMDrv_GetIntr(kal_uint16 *dataRead)
{
	MTSOC_ReadByte(FM_MAIN_INTR, dataRead);
}

void FMDrv_ClrIntr(void)
{
#if (defined(FM_EINT_SUPPORT_SEARCH)) || (defined(__FM_RADIO_RDS_SUPPORT__))
	kal_uint16 dataRead;
	MTSOC_ReadByte(FM_MAIN_INTR, &dataRead);
	MTSOC_WriteByte(FM_MAIN_INTR, dataRead);//clear status flag
	IRQUnmask(IRQ_FM_CODE);
#endif	
}

/*************************************************************
*  Radio power on Reset
*
*************************************************************/
void FMDrv_PowerOnReset(void)
{
	kal_int32 i,j=0;
	kal_uint16 tmp_reg, tmp_reg1;
 
 	FMR_TRACE(FM_INFO_FM_COMMON_POWER_ON,"FM Common Power On");	
    L1SM_SleepDisable(_drv_fm_l1sm_handle);
    dcl_pmu_set_vcama(PMU_VOLT_02_800000_V);
 	dcl_pmu_vcama_enable(KAL_TRUE);
	MTSOC_FMSYS_PowerOn();
    FMR_TRACE(FM_INFO_FM_POWER_ON_RST,"FM Power On reset");
	for (i=0; i<POWER_ON_COMMAND_COUNT; i++)
	{
		FMR_TRACE0(FM_ON_SEQUENCE_CMD,"power on cmd:%d",i);
		if (PowerOnSetting[i].addr==0xFFFF)
		{  //polling status=1
			j=0;
			FM_Data_debug = KAL_FALSE;
			while(1)
			{		
				MTSOC_ReadByte(PowerOnSetting[i].and, &tmp_reg);
				if(j++ >1000)
				{
					FMR_TRACE0(FM_ON_POLLING_TIMES,"power on polling 1000 times==%d\n",110);
					FMR_TRACE1( FM_INFO_READ_DATA, "Register [%x] =%x\n",PowerOnSetting[i].and,tmp_reg);
					j=0;
				}
				tmp_reg &=PowerOnSetting[i].or;
				if(tmp_reg)
					break;
			}      
			FM_Data_debug = KAL_TRUE;
		}
		else if (PowerOnSetting[i].addr==0xFFFE)
		{  //polling status=0
			j=0;
			FM_Data_debug = KAL_FALSE;
			while(1)
			{		
				MTSOC_ReadByte(PowerOnSetting[i].and, &tmp_reg);
				if(j++ >1000)
				{
					FMR_TRACE0(FM_ON_POLLING_TIMES,"power on polling 1000 times==%d\n",111);
					FMR_TRACE1( FM_INFO_READ_DATA, "Register [%x] =%x\n",PowerOnSetting[i].and,tmp_reg);
					j=0;
				}
				tmp_reg &=PowerOnSetting[i].or;
				if(!tmp_reg)
					break;
			}      
			FM_Data_debug = KAL_TRUE;
		}
		else if  (PowerOnSetting[i].addr==0xFFFD)
		{  //read status
			MTSOC_ReadByte(PowerOnSetting[i].and, &tmp_reg);
			switch(PowerOnSetting[i].and)
			{
				case  0x62:
					Chip_ID = tmp_reg;
                    FMR_TRACE0(FM_INFO_FM_CHIP_ID,"FM Chip ID: %x\n",Chip_ID);
					break;
				case 0x48:
					MTSOC_ReadByte(PowerOnSetting[i].and, &tmp_reg); //0x48
					MTSOC_ReadByte(PowerOnSetting[i].or, &tmp_reg1); //0x42
					MTSOC_WriteByte(PowerOnSetting[i].or, ((tmp_reg1&0xC0C0)|(tmp_reg&0x3F3F)));
					break;
				default:
					break;
			}
		 }
		else if(PowerOnSetting[i].addr==0xFFFB)
		{ //delay ms
			Delayms(PowerOnSetting[i].or);
		}
		else if(PowerOnSetting[i].addr==0xFFFA)
		{ //delay us
			Delayus(PowerOnSetting[i].or);
		}
		else
		{
			if(PowerOnSetting[i].and != 0)
			{
				MTSOC_ReadByte(PowerOnSetting[i].addr, &tmp_reg);
				tmp_reg &= PowerOnSetting[i].and;
				tmp_reg |= PowerOnSetting[i].or;
			}
			else
				tmp_reg = PowerOnSetting[i].or;
			MTSOC_WriteByte(PowerOnSetting[i].addr, tmp_reg);
			MTSOC_ReadByte(PowerOnSetting[i].addr, &tmp_reg);/*for debug*/
		}
	}
	/* Internal Inerrupt initialize */
	FMDrv_IntrInit();
 	/*disable short antenna calibration*/
	MTSOC_WriteByte(0x2F,0x4000);
#ifdef MT6256FM/*I2S clock will dispear if outband too large  */
	MTSOC_ReadByte(0x33, &tmp_reg);
	MTSOC_WriteByte(0x33,tmp_reg|0x0020);
#endif
	_is_fm_on = KAL_TRUE;
	_current_frequency = -1;
	FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x\n",0xA1);
}


/*****************************************************************
*  Radio power off
*
*****************************************************************/
void FMDrv_PowerOffProc(void)
{
	kal_int16 i;
	kal_uint16 tmp_reg;
	FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%x\n",0xA2);
	MTSOC_RampDown();
   	Delayms(200);//for audio buffter at least 100ms
    FMR_TRACE(FM_INFO_FM_POWER_OFF,"FM Power Off");
	for (i=0; i<POWER_OFF_COMMAND_COUNT; i++)
	{
		if(PowerOffProc[i].addr==0xFFFB)
		{ //delay ms
			Delayms(PowerOffProc[i].or);
		}
		else if(PowerOffProc[i].addr==0xFFFA)
		{ //delay us
			Delayus(PowerOffProc[i].or);
		}
		else
		{
		    if(PowerOffProc[i].and != 0)
		    {
			    MTSOC_ReadByte(PowerOffProc[i].addr, &tmp_reg);
			    tmp_reg &= PowerOffProc[i].and;
			    tmp_reg |= PowerOffProc[i].or;
		    }
		    else
			    tmp_reg = PowerOffProc[i].or;
		    MTSOC_WriteByte(PowerOffProc[i].addr, tmp_reg);
	    }
	}
	MTSOC_FMSYS_PowerOff();
	dcl_pmu_vcama_enable(KAL_FALSE);
	_is_fm_on = KAL_FALSE;
	_current_frequency = -1;	
    L1SM_SleepEnable(_drv_fm_l1sm_handle);	
	FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x\n",0xA2);
}


/*********************************************************************
*  Radio set frquency
*
*  parameter-->curf:setting frequency value
                    input value: 875 - 1080 ( 87.5 MHz - 108.0 MHz)
*********************************************************************/
void FMDrv_SetFreq( kal_int16 curf )  /* input value: 875 - 1080 ( 87.5 MHz - 108.0 MHz)*/
{
	FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%x",0xA3);
	pstFMR_data->is_searching = KAL_FALSE;
	if (!_is_fm_on)
	{
		FMDrv_PowerOnReset();
	}
//	MTSOC_Mute_OnOff(1);
	MTSOC_RampDown();
	MTSOC_HL_Side_Adj(curf);
	MTSOC_Freq_Avoid(curf);
	if (MTSOC_SetFreq(curf) == 0)
	{
		ASSERT(0);
	}
	FreqKHz = curf;
	
	if(TuneAgainFlag == KAL_TRUE)
	{
		TuneAgainFlag = KAL_FALSE;
		MTSOC_RampDown();
		MTSOC_HL_Side_Adj(curf);
		MTSOC_Freq_Avoid(curf);
		if (MTSOC_SetFreq(curf) == 0)
		{
			ASSERT(0);
		}
	}
	FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x",0xA3);
}

/**********************************************************************
*  Get really signal level in current frequency
*
*  parameter-->curf:frequency value of radio now
**********************************************************************/
kal_uint16 FMDrv_GetSigLvl( kal_int16 curf )
{
	kal_uint16 rssi;
	rssi = MTSOC_GetCurRSSI();
	return rssi;
}

/**********************************************************************
*  Get really signal level in current frequency
*
*  parameter-->curf:frequency value of radio now
**********************************************************************/
kal_uint16 FMDrv_GetPamdLvl( kal_int16 curf )
{
	kal_uint16 pamd, readtimes, total=0, Average_Time=0;
	
	if (curf != _current_frequency)
	{
		FMDrv_SetFreq( curf );
	}
	for(readtimes=0;readtimes<8;readtimes++)
	{
		pamd = MTSOC_GetCurPamd();
		total += pamd;
        if (pamd != 0)
        {
            Average_Time++;
        }
		kal_sleep_task(10);	
	}
   	if(Average_Time != 0)
   	{
	    return total/Average_Time;
   	}
    else
    {
        return 0;
    }
}
/*
range:1x~50dB
return dB
*/
kal_int16 FMDrv_GetMR(void)
{
	kal_uint16 RegTemp;
	kal_int16 MR;
	MTSOC_ReadByte(0xF2, &RegTemp);
	RegTemp=(RegTemp&0x1FF);
	
	if(RegTemp > 255)
	{
		MR=(kal_int16)(RegTemp-512)*6/16;
	}
	else
	{
		MR=(kal_int16)RegTemp*6/16;
	}
	return MR;
}

/**********************************************************************
*  Radio valid station,used in HW auto search frequency to verify 
*  valid positon
*
*  parameter-->freq: start frequency
               is_step_up:1-->forward, 0-->backward
               space:search step,0:200KHz, 1:100KHz
**********************************************************************/
void FMDrv_HWSearch(kal_int16 freq, kal_bool is_step_up, kal_int16 space, kal_bool is_preset)
{
	kal_int32 targetFreq;
	kal_uint8 UpDown_flag=1;
	kal_uint16 dataRead;
	kal_uint8 _step;   
			
	if (is_step_up)
		UpDown_flag = 0;
	HWSearch_flag = 0;
	Valid_flag = 0;
 	FreqSt = FreqKHz;			
	targetFreq = (kal_int32)(freq);
	if( space )
	{// 100K
	  _step = 1;
	}
	else
	{ //200K
	  _step = 2;
	}
    	MTSOC_RampDown();		
	//- Modified code start  
      	if (MTSOC_SetFreq(targetFreq) == 0) 
      	{
      	   ASSERT(0);
      	}
    	MTSOC_RampDown();		
	Delayms(50);
       // Setting before seek
    MTSOC_ReadByte(ADDR_SEARCH_THRESHOLD, &dataRead);  
#ifdef INTERNAL_ANTENNAL_SUPPORT
    if(FMDrv_GetAntennaType() == KAL_TRUE)
    {
        dataRead = ((dataRead&0xFC00)|(FMR_RSSI_THRESHOLD_SANT & 0x03FF));
    }
    else
#endif
    {
        dataRead = ((dataRead&0xFC00)|(FMR_RSSI_THRESHOLD_LANT & 0x03FF));
    }
    MTSOC_WriteByte(ADDR_SEARCH_THRESHOLD, dataRead);
    MTSOC_ReadByte(0xE1, &dataRead);  
    MTSOC_WriteByte(0xE1, ((dataRead&0xFF00)|FMR_CQI_TH));
    	MTSOC_ReadByte(FM_MAIN_CFG2, &dataRead);  
      	MTSOC_WriteByte(FM_MAIN_CFG2, (dataRead&0xFC00)|((FMR_SCAN_BAND_DN-640)<<1));//set space(100k/200k)and band(875~1080)and up/down
    	MTSOC_ReadByte(FM_MAIN_CFG1, &dataRead);  
      	MTSOC_WriteByte(FM_MAIN_CFG1, (dataRead&0x8800)|(UpDown_flag<<10)|(1<<(12+_step))|((FMR_SCAN_BAND_UP-640)<<1));//set space(100k/200k)and band(875~1080)and up/down
    	MTSOC_ReadByte(FM_MAIN_CFG1, &dataRead);

	if (!is_preset)
  	{	// this is not a auto_scan function !
		MTSOC_WriteByte(FM_MAIN_CFG1, dataRead&0xF7FF|0x0800); //enable wrap , if it is not auto scan function
    }else{
		MTSOC_WriteByte(FM_MAIN_CFG1, dataRead&0xF7FF); //disable wrap , if it is auto scan function
	}
	if (!is_preset)
  	{	// this is not a auto_scan function !
    		MTSOC_SEEK_ON();
        }else{
     		MTSOC_SCAN_ON();   
        }

}

kal_uint8 FMDrv_HWSpolling(kal_uint16 *curf, kal_uint8 *is_valid)
{
	kal_uint16 dataRead,i=0;
   	//- Modified code end
	FMR_TRACE0(FMDRV_API_ENTRANCE,"ENTRANCE:%x",0xA5);
   	MTSOC_ReadByte(FM_MAIN_INTR, &dataRead);
   	HWSearch_flag = dataRead & FM_INTR_STC_DONE; // check STC flag 
	FM_Data_debug = KAL_FALSE;
  	while( HWSearch_flag == 0)
  	{
  		if(pstFMR_data->bHWsearchStop == 1)
  		{
  			TuneAgainFlag = KAL_TRUE;
  			*curf = FreqSt;
  			*is_valid = 0;
			FMDrv_ClrIntr();
			FM_Data_debug = KAL_TRUE;
  			return 0;
  		}
		MTSOC_ReadByte(FM_MAIN_INTR, &dataRead);
		if(i++ >1000)
		{
			FMR_TRACE0(FM_ON_POLLING_TIMES,"power on polling 1000 times==%d\n",50);
			FMR_TRACE1(FM_INFO_READ_DATA, "Register [%x] =%x\n",FM_MAIN_INTR,dataRead);
			i=0;
		}
		HWSearch_flag = dataRead & FM_INTR_STC_DONE; // check STC flag 
		Valid_flag = 1;
	   	//MTSOC_ReadByte(FM_MAIN_CHANDETSTAT, &dataRead);   	
		kal_sleep_task(5);
  	}
	FM_Data_debug = KAL_TRUE;
	MTSOC_ReadByte(FM_MAIN_INTR, &dataRead);	
   	MTSOC_WriteByte(FM_MAIN_INTR, (dataRead | FM_INTR_STC_DONE | FM_INTR_RDS));//clear status flag
    MTSOC_ReadByte(FM_MAIN_INTR, &dataRead);/*for dbg*/
   	MTSOC_ReadByte(FM_MAIN_CHANDETSTAT, &dataRead);   	
 	FreqKHz = 640 + ((dataRead & MASK_READCHAN )>> (SHIFT_READCHAN+1) );			
	//kal_prompt_trace(MOD_FMR, "FreqKHz: %x", FreqKHz);
	if(FreqKHz > 1080)
		FreqKHz = 1080;
	else if(FreqKHz < 875)
		FreqKHz = 875;
  	
	if (HWSearch_flag != 0)
	{
		MTSOC_GetSearchFreq(curf); 
		if (Valid_flag == 1)
			*is_valid = 1;
		else
			*is_valid = 0;
		FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x\n",0xA50);
		return 1;
	}
	else
	{
		FMR_TRACE0(FMDRV_API_EXIT,"EXIT:%x\n",0xA51);
		return 0;
	}
}

kal_uint8 FMDrv_ValidStop(kal_int16 freq, kal_int8 signalvl, kal_bool is_step_up)
{
   return 1;
}

void FMDrv_GetScanTbl(kal_uint16 *BitMapData, kal_uint8 offset)
{
	kal_uint16 dataRead;
	
	MTSOC_ReadByte(0x80+offset, &dataRead);	
	*(BitMapData+offset) = dataRead;
}
kal_bool FMDrv_Seek_Eliminate(kal_uint16 rFreq, kal_uint16 RSSI)
{
	kal_uint16 RssiTemp;	
	kal_bool EmptyChannel = KAL_FALSE;

	FMDrv_SCAN_Sort_Tune_Internal(rFreq,&RssiTemp);
	if(RssiTemp < RSSI)
	{
		EmptyChannel = KAL_TRUE;
	}

	return EmptyChannel;
}
kal_bool FMDrv_SCAN_Sort_Tune(FMR_SCAN_Sort_Data *p_SortData,kal_uint8 rChannelNo)
{
    kal_uint8 i;
    for(i = 0; i < rChannelNo; i++)
    {
        if(!pstFMR_data->bHWsearchStop)
        {
            FMDrv_SCAN_Sort_Tune_Internal(p_SortData[i].ChannelNo,&p_SortData[i].RSSI);//get rssi
        }
        else
        {
            return KAL_FALSE;
        }
    }

    return KAL_TRUE;
}
void FMDrv_SCAN_Sort_Tune_Internal(kal_uint16 freq,kal_uint16 *p_RSSI)
{
	kal_uint32 CHAN = 0x0000;
	kal_uint16 dataRead,RSSIValue=0;
    kal_uint8 cnt=0,GetTime=4;
    //clear rgf_tune/seek/scan/rxcal
	MTSOC_ReadByte(FM_MAIN_CTRL, &dataRead);		
  	MTSOC_WriteByte(FM_MAIN_CTRL, (dataRead&0xFFF0)); 
	MTSOC_ReadByte(FM_MAIN_CHANDETSTAT, &dataRead);		
  	while( (dataRead & MASK_STC) == 1)
  	{
		MTSOC_ReadByte(FM_MAIN_CHANDETSTAT, &dataRead);
  	}  	  	
    //set desired channel
	CHAN = (freq - 640)<<1;
	MTSOC_ReadByte(0x0F, &dataRead);
	MTSOC_WriteByte(0x0F, (dataRead&0xFC00)|CHAN);
    MTSOC_TUNE_ON();	
	do
	{
		if(pstFMR_data->bHWsearchStop)/*if search abort, cancel polling*/
		{
			return;
		}
		MTSOC_ReadByte(FM_MAIN_INTR, &dataRead);
		if((dataRead & MASK_STC) == 0)
		{
			kal_sleep_task(1);
		}
	}while ((dataRead & MASK_STC)==0);
   	MTSOC_WriteByte(FM_MAIN_INTR, (dataRead&~MASK_STC)|MASK_STC);//clear status flag
   	for(cnt=0; cnt<GetTime; cnt++)
   	{
   	    RSSIValue += FMDrv_GetSigLvl(freq);
        Delayus(2250);
   	}
    *p_RSSI = RSSIValue/GetTime;
	kal_trace(TRACE_INFO,FM_INFO_SortTuneRssi,freq,RSSIValue/GetTime);
    
    return;
}
/**APIs for EM**/
kal_uint32 FMDrv_GetRSSITH(kal_uint8 type)/*0: long ant. 1: short ant.*/
{
	kal_uint32 RSSI_TH;
	if(type == 0)
	{
		RSSI_TH = FMR_RSSI_THRESHOLD_LANT;
	}
	else
	{
		RSSI_TH = FMR_RSSI_THRESHOLD_SANT;
	}
	return RSSI_TH;
}
void FMDrv_SetRSSITH(kal_uint8 type,kal_uint32 value)/*0: long ant. 1: short ant.*/
{
	if(type == 0)
	{
		FMR_RSSI_THRESHOLD_LANT = value;
	}
	else
	{
		FMR_RSSI_THRESHOLD_SANT = value;
	}
}
kal_int32 FMDrv_RSSI_Hex2Db(kal_uint32 Hex)
{
	kal_int32 dBValue;
	
    dBValue = Hex&0x3FF;
	dBValue = (kal_int32)((dBValue>511)?(((float)dBValue-1024)*6/16+113):((float)dBValue*6/16+113));
    
    return dBValue;
}
kal_uint32 FMDrv_RSSI_Db2Hex(kal_int32 Db)
{
	kal_int32 dBValue;
	kal_uint32 HexValue;
	dBValue = (Db-113)*16/6;
	HexValue = (kal_uint32)((dBValue < 0)?(dBValue+1024):dBValue);
	return HexValue;
}
void FMDrv_Force_Mono(void)
{
	kal_uint16 RegTemp;
	MTSOC_WriteByte(FM_PG_SEL, 1);
	MTSOC_ReadByte(0xE0,&RegTemp);
	MTSOC_WriteByte(0xE0, RegTemp|0x0001);
	MTSOC_WriteByte(FM_PG_SEL, 0);
}
void FMDrv_Force_Stereo(void)
{
	kal_uint16 RegTemp;
	MTSOC_WriteByte(FM_PG_SEL, 1);
	MTSOC_ReadByte(0xE0,&RegTemp);
	MTSOC_WriteByte(0xE0, RegTemp&(~0x0001));
	MTSOC_WriteByte(FM_PG_SEL, 0);
}

#endif // defined(MTSOC)
