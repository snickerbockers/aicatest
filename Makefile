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

AS=sh-elf-as
LD=sh-elf-ld
CC=sh-elf-gcc
TARGET=stopwatch.elf

.PHONY: run all

all: $(TARGET)

clean:
	rm -f $(TARGET) init.o stopwatch.o romfont.o

init.o: init.s
	$(AS) -little -o init.o init.s

$(TARGET): init.o stopwatch.o romfont.o
	$(CC) -Wl,-e_start,-Ttext,0x8c010000 $^ -o $(TARGET) -nostartfiles -nostdlib -lgcc

stopwatch.o: stopwatch.c pvr.h tmu.h romfont.h
	$(CC) -c $< -nostartfiles -nostdlib

romfont.o: romfont.c romfont.h
	$(CC) -c $< -nostartfiles -nostdlib


run: $(TARGET)
	washingtondc -c test -- $<
