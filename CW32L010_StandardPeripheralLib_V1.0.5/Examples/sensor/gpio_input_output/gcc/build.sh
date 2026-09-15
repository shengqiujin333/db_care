#!/bin/sh
# GNU 交叉编译验证 (CW32L010Y8M6, Cortex-M0+), 传感器工程
# 输出: obj/*.o, obj/sensor_fw.elf/.hex/.bin
set -e
cd "$(dirname "$0")"
ARM_PACK=/c/Users/kason/AppData/Local/Arm/Packs/ARM/CMSIS/5.9.0/CMSIS/Core/Include
S=..
L=../../../..
INC="-I$S/USER/inc -I$S/COMMON -I$S/UM2005C -I$L/Libraries/inc -I$S/MDK/RTE -I$ARM_PACK"
mkdir -p obj
echo "== compile =="
for f in $S/USER/src/*.c $S/COMMON/*.c $S/UM2005C/*.c $L/Libraries/src/*.c; do
  arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -O1 -Wall -Wextra -c $INC "$f" -o obj/$(basename $f .c).o
done
arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -c startup_cw32l010.S -o obj/startup.o
echo "== link =="
arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -O1 -ffreestanding -nostartfiles \
  --specs=nano.specs -Wl,--gc-sections -Wl,--entry=Reset_Handler \
  -T cw32l010.ld -o obj/sensor_fw.elf obj/*.o -Wl,--print-memory-usage
arm-none-eabi-objcopy -O ihex obj/sensor_fw.elf obj/sensor_fw.hex
arm-none-eabi-objcopy -O binary obj/sensor_fw.elf obj/sensor_fw.bin
echo "== done: obj/sensor_fw.elf/.hex/.bin =="
