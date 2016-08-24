# Makefile - makefile for /target/src/usb/target
#
# modification history
# --------------------
# 01c,19oct01,tam  added support for new archive directory hierarchy
# 01b,24apr00,wef   added files common to host stack into this directory
# 01a,27mar00,wef   created from 01t of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the usb library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES=	usbTargLib.c usbTcdLib.c ossLib.c usbDescrCopyLib.c\
		usbHandleLib.c usbListLib.c usbQueueLib.c


LIB_BASE_NAME   = usb

OBJS=		usbTargLib.o usbTcdLib.o ossLib.o usbDescrCopyLib.o\
		usbHandleLib.o usbListLib.o usbQueueLib.o


include $(TGT_DIR)/h/make/rules.library

DOC_DIR         = $(DOCS_ROOT)/vxworks/ref/usb
DOC_CHAPTER     = USB_libraries
DOC_OPTS        = $(DOC_OPTS_MAN) -category USB
