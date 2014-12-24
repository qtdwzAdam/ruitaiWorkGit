# Makefile
#
# ============================================================================
# Copyright (c) Texas Instruments Inc 2009
#
# Use of this software is controlled by the terms and conditions found in the
# license agreement under which this software has been supplied or provided.
# ============================================================================

ROOTDIR = /usr/local/icetek-dm6446-kbe/dvsdk_2_00_00_22
TARGET = $(notdir $(CURDIR))

include $(ROOTDIR)/Rules.make

# Comment this out if you want to see full compiler and linker output.
VERBOSE = @

# Package path for the XDC tools
DVSDKDEMO = $(ROOTDIR)/dvsdk_demos_2_00_00_07_community_kernel
XDC_PATH = $(USER_XDC_PATH);$(DVSDKDEMO)/packages;$(DMAI_INSTALL_DIR)/packages;$(CE_INSTALL_DIR)/packages;$(FC_INSTALL_DIR)/packages;$(LINK_INSTALL_DIR);$(XDAIS_INSTALL_DIR)/packages;$(CMEM_INSTALL_DIR)/packages;$(CODEC_INSTALL_DIR)/packages;../

# Where to output configuration files
XDC_CFG		= local

# Output compiler options
XDC_CFLAGS	= $(XDC_CFG)/compiler.opt

# Output linker file
XDC_LFILE	= $(XDC_CFG)/linker.cmd

# Input configuration file
XDC_CFGFILE	= local.cfg

# Platform (board) to build for
XDC_PLATFORM = ti.platforms.evmDM6446

# Target tools
XDC_TARGET = gnu.targets.arm.GCArmv5T

# The XDC configuration tool command line
CONFIGURO = $(XDC_INSTALL_DIR)/xs xdc.tools.configuro

C_FLAGS += -Wall -g

LINUXLIBS = $(ROOTDIR)/linuxlibs-2009.11-armv5te

LD_FLAGS += -L/usr/local/icetek-dm6446-kbe/arm-2009q1/arm-none-linux-gnueabi/lib
LD_FLAGS += -L$(LINUXLIBS)/lib -lpthread -lpng -ljpeg -lfreetype -lasound -lz

LD_LIBS += $(DMAI_INSTALL_DIR)/packages/ti/sdo/dmai/lib/dmai_linux_dm6446.a470MV
LD_LIBS += $(DVSDKDEMO)/packages/ti/sdo/simplewidget/lib/simplewidget_dm6446.a470MV

COMPILE.c = $(VERBOSE) $(CSTOOL_PREFIX)gcc $(C_FLAGS) $(CPP_FLAGS) -c
LINK.c = $(VERBOSE) $(CSTOOL_PREFIX)gcc $(LD_FLAGS)

SOURCES = $(wildcard *.c) $(wildcard ../*.c)
HEADERS = $(wildcard *.h) $(wildcard ../*.h)

OBJFILES = $(SOURCES:%.c=%.o)

.PHONY: clean install

all:	dm6446

dm6446:	dm6446_al

dm6446_al:	$(TARGET)

install:	$(if $(wildcard $(TARGET)), install_$(TARGET))

install_$(TARGET):
	@install -d $(EXEC_DIR)
	@install $(TARGET) $(EXEC_DIR)
	@install $(TARGET).txt $(EXEC_DIR)
	@echo
	@echo Installed $(TARGET) binaries to $(EXEC_DIR)..

$(TARGET):	$(OBJFILES) $(XDC_LFILE)
	@echo
	@echo Linking $@ from $^..
	$(LINK.c) -o $@ $^ $(LD_LIBS)

$(OBJFILES):	%.o: %.c $(HEADERS) $(XDC_CFLAGS)
	@echo Compiling $@ from $<..
	$(COMPILE.c) $(shell cat $(XDC_CFLAGS)) -o $@ $<

$(XDC_LFILE) $(XDC_CFLAGS):	$(XDC_CFGFILE)
	@echo
	@echo ======== Building $(TARGET) ========
	@echo Configuring application using $<
	@echo
	$(VERBOSE) XDCPATH="$(XDC_PATH)" $(CONFIGURO) -c $(CSTOOL_DIR) -o $(XDC_CFG) -t $(XDC_TARGET) -p $(XDC_PLATFORM) -b ../../config.bld $(XDC_CFGFILE)

clean:
	@echo Removing generated files..
	$(VERBOSE) -$(RM) -rf $(XDC_CFG) $(OBJFILES) $(TARGET) *~ *.d .dep
