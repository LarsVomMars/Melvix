# MIT License, Copyright (c) 2020 Marvin Borner

COBJS = psf.o \
	bmp.o \
	png.o \
	gfx.o \
	gui.o \
	msg.o
CC = ccache ../cross/opt/bin/i686-elf-gcc
LD = ccache ../cross/opt/bin/i686-elf-ld
AR = ccache ../cross/opt/bin/i686-elf-ar

WARNINGS = -Wall -Wextra -pedantic-errors -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wnested-externs -Wno-long-long
CFLAGS = $(WARNINGS) -nostdlib -nostdinc -fno-builtin -std=c99 -m32 -Iinc/ -I../libc/inc/ -fPIE -Duserspace -Ofast

all: libgui

%.o: %.c
	@$(CC) -c $(CFLAGS) $< -o $@

libgui: $(COBJS)
	@mkdir -p ../build/
	@$(AR) rcs ../build/libgui.a $+

clean:
	@find . -name "*.o" -type f -delete
