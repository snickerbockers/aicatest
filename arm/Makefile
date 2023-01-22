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
ARM_AS=arm-eabi-as
ARM_LD=arm-eabi-ld
ARM_OBJCOPY=arm-eabi-objcopy
ARM_CC=arm-eabi-gcc

OBJECTS=arm7_fw.o message.o arm_main.o

all: arm7_fw.bin

clean:
	rm -f arm7_fw.elf arm7_fw.bin $(OBJECTS)

.PHONY: all

%.o: %.s
	$(ARM_AS) $<  -EL -mcpu=arm7di  -o $@

arm7_fw.elf: $(OBJECTS)
	$(ARM_LD) -Ttext 0x00000000 $^ -o $@

arm7_fw.bin: arm7_fw.elf
	$(ARM_OBJCOPY) -O binary $< $@

# NOTE: building for arm7tdmi is potentially dangerous since dreamcast only has arm7di
#       but this is the closest arch that is supported on modern gcc
#
#       in the future i should look into installing an older gcc version with armv3 support
arm_main.o: arm_main.c
	$(ARM_CC) -mcpu=arm7tdmi -ffreestanding -fomit-frame-pointer -Wl,-Ttext=0 -nostartfiles -nostdlib -c $<
