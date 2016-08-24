# Makefile - makefile for target/src/drv/usb/target
#
# modification history
# --------------------
# 01b,12oct01,tam  added repackaging support
# 01a,27mar00,wef   created
#
# DESCRIPTION
# This file contains the makefile rules for building the usb library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES=	usbTargKbdLib.c usbTargPhilipsD12EvalLib.c usbTargPrnLib.c\
		usbTcdPdiusbd12EvalLib.c

LIB_BASE_NAME=usb

OBJS =		usbTargKbdLib.o usbTargPhilipsD12EvalLib.o usbTargPrnLib.o\
		usbTcdPdiusbd12EvalLib.o



include $(TGT_DIR)/h/make/rules.library

DOC_DIR         = $(DOCS_ROOT)/vxworks/ref/usb
DOC_CHAPTER     = USB_libraries
DOC_OPTS        = $(DOC_OPTS_MAN) -category USB
