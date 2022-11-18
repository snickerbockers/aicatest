################################################################################
#
#
#     Copyright (C) 2022 snickerbockers
#
#     This program is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
#
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#
################################################################################

AS=sh4-elf-as
LD=sh4-elf-ld
CC=sh4-elf-gcc
OBJCOPY=sh4-elf-objcopy
SCRAMBLE=scramble
MAKEIP=makeip
CDI4DC=cdi4dc

ELF=aica_test.elf
TARGET=aica_test.cdi

.PHONY: run all

all: $(TARGET)

clean:
	rm -f $(ELF) init.o main.o romfont.o maple.o

init.o: init.s
	$(AS) -little -o init.o init.s

$(ELF): init.o main.o romfont.o maple.o
	$(CC) -Wl,-e_start,-Ttext,0x8c010000 $^ -o $(ELF) -nostartfiles -nostdlib -lgcc

main.o: main.c pvr.h tmu.h romfont.h maple.h
	$(CC) -c $< -nostartfiles -nostdlib

romfont.o: romfont.c romfont.h
	$(CC) -c $< -nostartfiles -nostdlib

maple.o: maple.c maple.h
	$(CC) -c $< -nostartfiles -nostdlib

run: $(TARGET)
	washingtondc -c test -- $<

1st_read.bin.unscrambled: $(ELF)
	$(OBJCOPY) -R .stack -O binary $< $@

isodir/1st_read.bin: 1st_read.bin.unscrambled
	mkdir -p isodir
	$(SCRAMBLE) $< $@

filesystem.iso: isodir/1st_read.bin IP.BIN $(shell find isodir)
	mkisofs -G IP.BIN -C 0,11702 -o $@ isodir

IP.BIN:
	$(MAKEIP) ./ip.txt ./IP.BIN

$(TARGET): filesystem.iso
	$(CDI4DC) $< $@

