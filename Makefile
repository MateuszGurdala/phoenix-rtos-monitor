include ../phoenix-rtos-build/Makefile.common

.DEFAULT_GOAL := all

# single component in this whole repo
NAME := monitorsrv
SRCS := $(wildcard *.c)
LIBS := libtty
include $(binary.mk)

ALL_COMPONENTS := monitorsrv
DEFAULT_COMPONENTS := $(ALL_COMPONENTS)

# create generic targets
.PHONY: all install clean
all: $(DEFAULT_COMPONENTS)
install: $(patsubst %,%-install,$(DEFAULT_COMPONENTS))
clean: $(patsubst %,%-clean,$(ALL_COMPONENTS))
