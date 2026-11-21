# ==========================================================================
#  Lost Rune Adventure  -  PSP homebrew platformer
#  Build:  make            -> EBOOT.PBP  (+ .prx/.elf)
#          ./build_iso.sh  -> Adventure.iso  (for the "Ppsspp rom" folder)
#  Needs:  PSPSDK (pspdev) with PSPDEV / PSPSDK set in the environment.
# ==========================================================================

TARGET      = adventure
OBJS        = src/main.o src/gfx.o src/text.o src/game.o src/render.o
BUILD_PRX   = 1
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Lost Rune Adventure

INCDIR =
CFLAGS  = -O2 -G0 -Wall -Wextra
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBDIR =
LDFLAGS =
LIBS    = -lpspgu -lpspctrl -lpspdisplay -lpspge -lpspdebug -lm

# Find the SDK even if PSPSDK was not exported.
ifeq ($(PSPSDK),)
PSPSDK := $(shell psp-config --pspsdk-path 2>/dev/null)
endif
ifndef PSPSDK
$(error PSPSDK is not set and psp-config was not found. Install pspdev / the PSP SDK first.)
endif

include $(PSPSDK)/lib/build.mk
