# Makefile - makefile for bsp/src/drv/end
#
# modification history
# --------------------
# 01g,12oct98,vin   added elt3c509End.c
# 01f,25aug97,vin   deleted the sonic driver for the beta2 patch.
# 01e,03May97,sal   restored sonic driver to DOC_FILES
# 01d,30apr97,jag   deleted the sonic driver from the DOC_FILES due to man page
#		    errors.
# 01c,24apr97,map   added dec21x40End
# 01b,18apr97,sal   added the sn83932End.c
# 01a,28feb97,vin   created from 01d of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building ENDs (Enhanced
# Network Drivers) into the VxWorks library.
#
# Old style network drivers (i.e. BSD style) are contained in src/drv/netif.
#
#*/

TGT_DIR=$(WIND_BASE)/target

DOC_FILES= endLib.c ln7990End.c ei82596End.c motCmpEnd.c \
           dec21x40End.c sn83932End.c elt3c509End.c ln97xEnd.c

LIBNAME=lib$(CPU)$(TOOL)vx.a
LIBDIRNAME=obj$(CPU)$(TOOL)vx

EXTRA_INCLUDE=-I$(WIND_BASE)/target/h/

OBJS_COMMON = endLib.o ln7990End.o motCmpEnd.o ei82596End.o \
              dec21x40End.o sn83932End.o elt3c509End.o ln97xEnd.o

OBJS        = $(OBJS_COMMON) #$(OBJS_<CPU>)

include $(TGT_DIR)/h/make/rules.library

CC_OPTIM=$(CC_OPTIM_DRIVER)
