# Copyright 2010, The Native Client SDK Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can
# be found in the LICENSE file.

# Modified by Kate Volkova <gloom.msk@gmail.com>

# NACL_SDK_ROOT should be set before calling make

.PHONY: all clean

CCFILES = ssh_plugin_module.cc ssh_plugin.cc  npp_gate.cc libssh2_nacl.cc terminal.cc js_utilities.cc
#npn_bridge.cc - only for dubugging

OBJECTS_X86_32 = $(CCFILES:%.cc=%_x86_32.o)
OBJECTS_X86_64 = $(CCFILES:%.cc=%_x86_64.o)

LIBPREFIX=../third_party/

INCLUDES = -I$(LIBPREFIX)libssh2-1.2.6/include/ -I$(LIBPREFIX)openssl-1.0.0a/include/ -I$(LIBPREFIX)libssh2-1.2.6/src/
CFLAGS = -Wall -Wno-long-long -pthread -DXP_UNIX -Werror -I$(INCLUDES)
LDFLAGS = -lgoogle_nacl_imc \
          -lgoogle_nacl_npruntime \
          -lpthread \
          -lsrpc \
	        -L$(LIBPREFIX)libssh2-1.2.6/src/.libs -lssh2 \
	        -L$(LIBPREFIX)zlib-1.2.5 -lz \
	        -lnosys \
	        -L$(LIBPREFIX)openssl-1.0.0a/ -lcrypto \
	        -L../naclstubs -lstubs \
          $(ARCH_FLAGS)
OPT_FLAGS = -O2

all: check_variables ssh_plugin_x86_32.nexe ssh_plugin_x86_64.nexe

# common.mk has rules to build .o files from .cc files.
-include common.mk

ssh_plugin_x86_32.nexe: $(OBJECTS_X86_32)
	$(CPP) $^ $(LDFLAGS) -m32 -o $@

ssh_plugin_x86_64.nexe: $(OBJECTS_X86_64)
	$(CPP) $^ $(LDFLAGS) -m64 -o $@

clean:
	-$(RM) $(OBJECTS_X86_32) $(OBJECTS_X86_64) \
	    ssh_plugin_x86_32.nexe ssh_plugin_x86_64.nexe

# This target is used by the SDK build system to produce a pre-built version
# of the .nexe.  You do not need to call this target.
install_prebuilt: ssh_plugin_x86_32.nexe ssh_plugin_x86_64.nexe
	-$(RM) $(OBJECTS_X86_32) $(OBJECTS_X86_64)
