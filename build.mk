# Build

rwildcard=$(foreach d,$(wildcard $1*), \
          $(call rwildcard,$d/,$2) \
          $(filter $(subst *,%,$2),$d))

SRCFILES=$(call rwildcard,$(SRCDIR)/,*.c *.S)
SRCFILES=$(call rwildcard,$(SRCDIR)/,*.c *.S)

TAG_ID=$(shell git -C $(SRCDIR) tag --points-at HEAD)
REV_ID=$(shell git -C $(SRCDIR) rev-parse --short HEAD)

ifneq ($(TAG_ID),)
VERSION_CFLAGS=-DCOMMIT_ID=\"$(TAG_ID)\"
else
VERSION_CFLAGS=-DCOMMIT_ID=\"$(REV_ID)\"
endif
