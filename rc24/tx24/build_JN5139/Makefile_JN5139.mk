#########################################################################
#
# MODULE:   AN1046_154_Coord
#
# DESCRIPTION: MakeFile
#
############################################################################
# 
# This software is owned by Jennic and/or its supplier and is protected
# under applicable copyright laws. All rights are reserved. We grant You,
# and any third parties, a license to use this software solely and
# exclusively on Jennic products. You, and any third parties must reproduce
# the copyright and warranty notice and any other legend of ownership on each
# copy or partial copy of the software.
# 
# THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
# EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
# ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
# BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
# INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
# 
# Copyright Jennic Ltd 2009. All rights reserved
#
#############################################################################
# Subversion variables
# $HeadURL:  $
# $Revision: 13257 $
# $LastChangedBy: pjtw $
# $LastChangedDate: 2009-05-26 16:11:24 +0100 (Tue, 26 May 2009) $
# $Id: Makefile 13257 2009-05-26 15:11:24Z mwild $ 
#
#############################################################################

# Application target name

TARGET = tx24

##############################################################################
#User definable make parameters that may be overwritten from the command line

# Default target device is the JN5139
JENNIC_CHIP ?= JN5139R1

#define JN5139 in the same way as 5148 and 5168
CFLAGS += -DJN5139=5139
##############################################################################
# Default DK2 development kit target hardware

# Set DK1 board for JN5121
ifeq ($(JENNIC_CHIP),JN5121)
   CFLAGS += -DBUILD_JN5121
   JENNIC_PCB  ?= DEVKIT1
else
   JENNIC_PCB ?= DEVKIT2
endif


##############################################################################
# Select the network stack (e.g. MAC, ZBPRO)

JENNIC_STACK ?= MAC

##############################################################################
# Debug options define DEBUG either HW or SW
#DEBUG ?=SW
#DEBUG ?=HW
#
# Define which UART to use for debug
DEBUG_PORT ?= UART1

##############################################################################
# Define TRACE to use with DBG module
#TRACE ?=1

##############################################################################
# Path definitions

SDK_BASE_DIR ?= /cygdrive/c/Jennic/cygwin/jennic/SDK


# Use if application directory contains multiple targets
ifndef SDK_BASE_DIR
SDK_BASE_DIR			= $(abspath ../../../..)
endif

#SDK_BASE_DIR   	 	= $(abspath ../../../..)
APP_BASE            	 	= $(abspath ../..)
APP_BLD_DIR			= $(APP_BASE)/$(TARGET)/Build
APP_SRC_DIR 	        	= $(APP_BASE)/$(TARGET)/Source
APP_COMMON_SRC_DIR = $(APP_BASE)/Common/Source


# Use if application directory contains single target
#SDK_BASE_DIR         	= $(abspath ../../..)
#APP_BASE            	= $(abspath ..)
#APP_BLD_DIR          	= $(APP_BASE)/Build
#APP_SRC_DIR          	= $(APP_BASE)/Source

##############################################################################
# Application Source files

# Note: Path to source file is found using vpath below, so only .c filename is required
APPSRC  = txmain.c
APPSRC += display.c
APPSRC += hopping.c
APPSRC += lcdEADog.c
APPSRC += model.c
APPSRC += mymodels.c
APPSRC += pcComs.c
APPSRC += ppm.c
APPSRC += ps2.c
APPSRC += radiocoms.c
APPSRC += routedmessage.c
APPSRC += store.c
APPSRC += swEventQueue.c
APPSRC += tsc2003.c
APPSRC += wii.c
APPSRC += hwutils.c
APPSRC += codeUpdate.c
APPSRC += gui_walnut.c
APPSRC += commonCommands.c
APPSRC += exceptions.c

APPSRC += AppQueueApi.c
##############################################################################
# Additional Application Source directories
# Define any additional application directories outside the application directory
# e.g. for AppQueueApi

#ADDITIONAL_SRC_DIR += $(COMPONENTS_BASE_DIR)/AppQueueApi/Source
ADDITIONAL_SRC_DIR += $(SDK_BASE_DIR)/Common/Source

##############################################################################
# Standard Application header search paths

INCFLAGS += -I$(APP_SRC_DIR)
INCFLAGS += -I$(APP_SRC_DIR)/..
INCFLAGS += -I$(APP_COMMON_SRC_DIR)

# Application specific include files
#Set v1 style include paths
INCFLAGS += -I$(SDK_BASE_DIR)/Chip/Common/Include
INCFLAGS += -I$(SDK_BASE_DIR)/Common/Include
ifeq ($(JENNIC_PCB),DEVKIT1)
   INCFLAGS += -I$(SDK_BASE_DIR)/Platform/DK1/Include 
else
   INCFLAGS += -I$(SDK_BASE_DIR)/Platform/DK2/Include 
endif
INCFLAGS += -I$(SDK_BASE_DIR)/Platform/Common/Include

INCFLAGS += -I$(COMPONENTS_BASE_DIR)/AppQueueApi/Include 


##############################################################################
# Application libraries
# Specify additional Component libraries

#APPLIBS += 

##############################################################################

# You should not need to edit below this line

##############################################################################
##############################################################################
# Configure for the selected chip or chip family
# Call v1 style config.mk
BASE_DIR=$(SDK_BASE_DIR)
include $(SDK_BASE_DIR)/Common/Build/config.mk
##############################################################################
INCFLAGS += -I$(SDK_BASE_DIR)/Chip/$(JENNIC_CHIP_FAMILY)/Include

APPOBJS = $(APPSRC:.c=.o)

##############################################################################
# Application dynamic dependencies

APPDEPS = $(APPOBJS:.o=.d)

#########################################################################
# Linker

# Add application libraries before chip specific libraries to linker so
# symbols are resolved correctly (i.e. ordering is significant for GCC)

#LDLIBS := $(addsuffix _$(JENNIC_CHIP_FAMILY),$(APPLIBS)) $(LDLIBS)

# Set linker variables from V1 Style makefile
BOARDAPI_LIB  = $(BOARDAPI_BASE)/Library
BOARD_LIB     = BoardLib_$(JENNIC_CHIP_FAMILY)


LIBFILE = $(SDK_BASE_DIR)/Common/Library/libm.a


LIBFILE += $(BOARDAPI_LIB)/$(BOARD_LIB).a
LIBFILE += $(SDK_BASE_DIR)/Chip/$(JENNIC_CHIP_FAMILY)/Library/ChipLib.a

LIBFILE += $(SDK_BASE_DIR)/Common/Library/libc.a


STACK_BASE    = $(BASE_DIR)/Chip/$(JENNIC_CHIP_FAMILY)
STACK_BLD     = $(STACK_BASE)/Build
LINKER_FILE = AppBuild_$(JENNIC_CHIP).ld

# Tell linker to garbage collect unused section
LDFLAGS += --gc-sections 
# Define entry points to linker
LDFLAGS += -u_AppColdStart -u_AppWarmStart
#########################################################################

# Debug Support
ifeq ($(DEBUG), SW)
CFLAGS  := $(subst -Os,,$(CFLAGS))
CFLAGS  += -g -O0 -DGDB
CFLAGS  += -DSWDEBUG

BIN_SUFFIX ?= _swdbg
$(info Building SW debug version ...)
endif

#########################################################################
# Dependency rules

.PHONY: all clean
# Path to directories containing application source 
vpath % $(APP_SRC_DIR):$(APP_COMMON_SRC_DIR):$(ADDITIONAL_SRC_DIR)


all: $(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).bin


# comment out the include below as prevents clean from working on V1 SDK
#-include $(APPDEPS)
%.d:
	rm -f $*.o


%.o: %.S
	$(info Assembling $< ...)
	$(CC) -c -o $(subst Source,Build,$@) $(CFLAGS) $(INCFLAGS) $< -MD -MF $*.d -MP
	@echo

%.o: %.c 
	$(info Compiling $< ...)
	$(CC) -c -o $(subst Source,Build,$@) $(CFLAGS) $(INCFLAGS) $< -MD -MF $*.d -MP
	@echo

$(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).elf: $(APPOBJS)
	$(info Linking $@ ...)
# Use LD rather than gcc for JN5139
	$(LD) -L$(STACK_BLD) -T$(LINKER_FILE) -o $@ $(LDFLAGS) $(LIBS) $(APPOBJS) $(LIBFILE)
	ba-elf-size $@
	@echo

$(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).bin: $(TARGET)_$(JENNIC_CHIP)$(BIN_SUFFIX).elf 
	$(info Generating binary ...)
	$(OBJCOPY) -S -O binary $< $@
	
#########################################################################

clean:
	rm -f $(APPOBJS) $(APPDEPS) $(TARGET)_$(JENNIC_CHIP)*.bin $(TARGET)_$(JENNIC_CHIP)*.elf $(TARGET)_$(JENNIC_CHIP)*.map

#########################################################################
