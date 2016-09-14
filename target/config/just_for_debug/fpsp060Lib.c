/* fpsp060Lib.c - MC68060 floating point library */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,28jun94,tpr clean up following code review.
		written.
*/

/* 
DESCRIPTION

This file provides an function which initializes the call out table used by
the floating point handlers. This table is filled with the relative function
addresses provided by the host OS (VxWorks) to allow floating point handlers
integration.
*/

#include "vxWorks.h"
#include "iv.h"
#include "intLib.h"
#include "fpsp060Lib.h"
#include "os060Lib.h"

FUNCPTR _060_real_fline_hdl;	/* global variable which handles the default */
				/* handler connected to the F-line exception */

/********************************************************************************
* fpsp060COTblInit - initialize the call out table of floating point handlers
*
* This function fills the call out table FPSP_060_CO_TBL with the relative
* addresses needed by the unimplemented floating point intruction handler
* and the floating point handlers.
*
* This function also saves the default handler number 11 in the
* _060_real_fline_hdl global variable. This default handler will be called
* if an instruction cannot be emulated by the unimplemented FP exception
* handler hooked at the exception 11.
*
* RETURNS : N/A
*
*/
void fpsp060COTblInit (void)
    {

    /* save the default handler of the exception number 11 */
    _060_real_fline_hdl =  intVecGet ((FUNCPTR *) IV_LINE_1111_EMULATOR);

    /* fill the FPSP_060_CO_TBL call out table */
    FPSP_060_CO_TBL[0] = (int ) _060_real_bsun - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[1] = (int ) _060_real_snan - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[2] = (int ) _060_real_operr - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[3] = (int ) _060_real_ovfl - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[4] = (int ) _060_real_unfl - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[5] = (int ) _060_real_dz - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[6] = (int ) _060_real_inex - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[7] = (int ) _060_real_fline - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[8] = (int ) _060_real_fpu_disabled - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[9] = (int ) _060_real_trap - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[10] = (int ) _060_real_trace - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[11] = (int ) _060_real_access - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[12] = (int ) _060_fpsp_done - (int ) FPSP_060_CO_TBL;

    /* entry 13 to 15 are Motorola reserved */

    FPSP_060_CO_TBL[16] = (int ) _060_imem_read - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[17] = (int ) _060_dmem_read - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[18] = (int ) _060_dmem_write - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[19] = (int ) _060_imem_read_word - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[20] = (int ) _060_imem_read_long - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[21] = (int ) _060_dmem_read_byte - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[22] = (int ) _060_dmem_read_word - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[23] = (int ) _060_dmem_read_long - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[24] = (int ) _060_dmem_write_byte - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[25] = (int ) _060_dmem_write_word - (int ) FPSP_060_CO_TBL;
    FPSP_060_CO_TBL[26] = (int ) _060_dmem_write_long - (int ) FPSP_060_CO_TBL;
    }

