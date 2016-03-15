#
# Makefile for XFireDB
#
# (c) 2016 Michel Megens  <dev@michelmegens.net>
#

make = /usr/bin/make

TARGET = host
BUILD_FILE = scripts/make/$(TARGET).make

include $(BUILD_FILE)

ifneq ($(TOOLCHAIN),)
	cmake_toolchain = '-DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN)'
endif

ifeq ($(DEBUG),true)
	cmake_debug = '-DXFIREDB_DEBUG=true'
else
	cmake_deubg = '-DXFIREDB_DEBUG=false'
endif

ifeq ($(CLIENTS),true)
	cmake_clients = '-DXFIREDB_CLIENTS=true'
else
	cmake_clients = '-DXFIREDB_CLIENTS=false'
endif

ifeq ($(SERVER),true)
	cmake_server = '-DXFIREDB_SERVER=true'
else
	cmake_server = '-DXFIREDB_SERVER=false'
endif

ifeq ($(X64),$(filter $(X64),true ))
	cmake_x64 = '-DX64=true'
else
	cmake_x64 = '-DX64=false'
endif

ifneq ($(RBCONF),)
	cmake_rbconf = '-DRUBY_RBCONF=$(RBCONF)'
endif

cmake_prefix = '-DCMAKE_INSTALL_PREFIX=$(PREFIX)' 
cmake_opts = $(cmake_prefix) $(cmake_toolchain) $(cmake_debug) $(cmake_clients) $(cmake_server) $(cmake_x64) $(cmake_rbconf)

all:
	@$(make) all -s -C build

configure:
	@echo 'Configuring for target: $(TARGET)'
	@mkdir -p build
	@cd build; cmake $(cmake_opts) ..

install:
	@$(make) install -s -C build

distclean:
	@rm -fr build

.DEFAULT:
	@$(make) $@ -s -C build

.PHONY: distclean configure install all

