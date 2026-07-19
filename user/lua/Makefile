# Makefile for building Lua for the mboot OS
# Cross-compilation with clang i386-elf target

CC = clang
LD = ld.lld
AR = llvm-ar
RM = rm -f

CFLAGS = -target i386-elf -m32 -ffreestanding -nostdlib -fno-pic -mno-red-zone \
         -O2 -Wall -Wextra -g -c \
         -I. -I../include \
         -ffunction-sections -fdata-sections

LDFLAGS = -m elf_i386 -T ../lua.ld -nostdlib -static --gc-sections
LIBC_OBJ = ../../build/libc.o
CRT0_OBJ = ../../build/crt0.o

LUA_A = liblua.a
CORE_O = lapi.o lcode.o lctype.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o \
         lmem.o lobject.o lopcodes.o lparser.o lstate.o lstring.o ltable.o \
         ltm.o lundump.o lvm.o lzio.o
LIB_O = lauxlib.o lbaselib.o lcorolib.o ldblib.o liolib.o lmathlib.o \
        loadlib.o loslib.o lstrlib.o ltablib.o lutf8lib.o linit.o
BASE_O = $(CORE_O) $(LIB_O)

LUA_T = lua
LUA_O = lua.o

LUAC_T = luac
LUAC_O = luac.o

ALL_O = $(BASE_O) $(LUA_O) $(LUAC_O)
ALL_T = $(LUA_A) $(LUA_T) $(LUAC_T)

all: $(ALL_T)

$(LUA_A): $(BASE_O)
	$(AR) rcs $@ $(BASE_O)

$(LUA_T): $(LUA_O) $(LUA_A) $(LIBC_OBJ) $(CRT0_OBJ)
	$(LD) $(LDFLAGS) $(LIBC_OBJ) $(CRT0_OBJ) $(LUA_O) $(LUA_A) -o $@

$(LUAC_T): $(LUAC_O) $(LUA_A) $(LIBC_OBJ) $(CRT0_OBJ)
	$(LD) $(LDFLAGS) $(LIBC_OBJ) $(CRT0_OBJ) $(LUAC_O) $(LUA_A) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(ALL_T) $(ALL_O) $(LUA_A)

.PHONY: all clean
