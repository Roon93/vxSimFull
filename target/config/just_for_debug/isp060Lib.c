/* isp060Lib.c - MC68060 unimplemented integer intruction exception library */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,24jun94,tpr	clean up following code review.
		written.
*/

/* 
DESCRIPTION

This file provides the isp060COTblInit() function which initialized the
call out section of the umimplemented integer instruction handler. This
handler is provided by Motorola. The call out section is used by the
exception handler to call function which are host operating system dependent.
The call out handles the host operating system depend functions RELATIVE to
the call out section top address.
*/

#include "vxWorks.h"
#include "isp060Lib.h"
#include "os060Lib.h"

/********************************************************************************
* isp060COTblInit - initialize the call out table of unimpl. integer handler
*
* This function fills the call out table ISP_060_CO_TBL with the relative
* addresses needed by the unimplemented integer intruction handler.
*
* RETURNS : N/A
*
*/
void isp060COTblInit (void)
    {
    ISP_060_CO_TBL[0] = (int ) _060_real_chk - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[1] = (int ) _060_real_divbyzero - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[2] = (int ) _060_real_trace - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[3] = (int ) _060_real_access - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[4] = (int ) _060_isp_done - (int ) ISP_060_CO_TBL;

    ISP_060_CO_TBL[5] = (int ) _060_real_cas - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[6] = (int ) _060_real_cas2 - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[7] = (int ) _060_real_lock_page - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[8] = (int ) _060_real_unlock_page - (int ) ISP_060_CO_TBL;

    /* entry 9 to 15 are Motorola reserved */

    ISP_060_CO_TBL[16] = (int ) _060_imem_read - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[17] = (int ) _060_dmem_read - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[18] = (int ) _060_dmem_write - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[19] = (int ) _060_imem_read_word - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[20] = (int ) _060_imem_read_long - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[21] = (int ) _060_dmem_read_byte - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[22] = (int ) _060_dmem_read_word - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[23] = (int ) _060_dmem_read_long - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[24] = (int ) _060_dmem_write_byte - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[25] = (int ) _060_dmem_write_word - (int ) ISP_060_CO_TBL;
    ISP_060_CO_TBL[26] = (int ) _060_dmem_write_long - (int ) ISP_060_CO_TBL;
    }

