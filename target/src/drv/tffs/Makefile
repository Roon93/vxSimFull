# Makefile - makefile for bsp/src/drv/tffs
#
# modification history
# --------------------
# 01h,26feb02,yp   added cfiamd.o to objects
# 01g,04dec01,nrv  added amd29LvMtd.o and removed i28f016Mantadsp.o
# 01f,12oct01,tam  added repackaging support
# 01e,23apr01,mem  Remove nfdc2048.c and nfdc2148.c
# 01d,07may99,jmb  add Manta driver
# 01c,16jan98,yp   added tffsConfig.c to DOC_FILES
# 01b,11jan98,hdn  added i28f008Baja.o
# 01a,28may97,yp   created
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES= tffsConfig.c

LIB_BASE_NAME= tffs

EXTRA_INCLUDE=-I$(TGT_DIR)/h/tffs

OBJS_COMMON =

OBJS        = amdmtd.o i28f008.o i28f016.o wamdmtd.o \
	      cfiamd.o cfiscs.o i28f008Baja.o amd29LvMtd.o 

include $(TGT_DIR)/h/make/rules.library
CFLAGS += -fsigned-char

CC_OPTIM=$(CC_OPTIM_DRIVER)
