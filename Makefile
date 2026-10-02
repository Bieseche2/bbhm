# BBhM - Bieseche Bullshit Hen manager
# Build com PSL1GHT (imagem hldtux/ps3dev-sdl2). make -> .self | make pkg -> .pkg
.DEFAULT_GOAL := all

ifeq ($(strip $(PSL1GHT)),)
$(error Defina PSL1GHT no ambiente)
endif
include $(PSL1GHT)/ppu_rules

TARGET    := bbhm
TITLE     := BBhM
APPID     := BBHM00001
CONTENTID := UP0001-$(APPID)_00-0000000000000000
ICON0     := $(CURDIR)/ICON0.PNG
PKGFILES  := $(CURDIR)/pkgfiles

PORTLIBS ?= $(PS3DEV)/portlibs/ppu

SRCS   := $(wildcard source/*.c)
OFILES := $(SRCS:.c=.o)

CFLAGS += -O2 -Wall -mcpu=cell -I$(PORTLIBS)/include -I$(PORTLIBS)/include/SDL2

# Se o link falhar, ajuste a ordem/lista aqui (o log do Actions mostra o que falta)
LIBS := -L$(PORTLIBS)/lib -lSDL2_ttf -lfreetype -lSDL2 -lm \
        -lgcm_sys -lrsx -lsysutil -lrt -llv2 -lio -laudio -lz

all: $(TARGET).self

$(TARGET).elf: $(OFILES)
$(CC) $(OFILES) $(LIBS) -o $@

%.o: %.c
$(CC) $(CFLAGS) -c $< -o $@

clean:
rm -f source/*.o $(TARGET).elf $(TARGET).self *.pkg
rm -rf build

.PHONY: all clean
