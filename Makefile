## Cross-compilation commands 
CC      = arm-none-eabi-gcc
LD      = arm-none-eabi-gcc
AR      = arm-none-eabi-ar
AS      = arm-none-eabi-as
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size

# our code
OBJS  = main.o clock.o spindle_encoder.o servo.o display.o input.o
# startup files and anything else
OBJS += stm32/system_stm32f1xx.o stm32/startup_stm32f103x6.o


## Platform and optimization options
CFLAGS += -c -fno-common -Os -g -mcpu=cortex-m3 -mthumb -std=c99
CFLAGS += -Wall -ffunction-sections -fdata-sections -fno-builtin
CFLAGS += -Wno-unused-function -ffreestanding
CFLAGS += -mfloat-abi=soft
CFLAGS += -DSTM32F103x6
LFLAGS  = -Tstm32/STM32F103X6_FLASH.ld -nostartfiles -Wl,--gc-sections
LFLAGS += -mcpu=cortex-m3 -mthumb -mfloat-abi=soft

## Locate the main libraries
CMSIS     = /home/stephen/projects/STM32Cube_FW_F3_V1.10.0/Drivers/CMSIS/

## Drivers/library includes
#INC     = -I$(CMSIS)/Include/
#INC    += -I$(CMSIS)/Device/ST/STM32F3xx/Include/ -I./
INC += -I./stm32/
CFLAGS += $(INC)

# drivers/lib sources -> objects -> local object files 
#VPATH                = $(CMSIS_SRC)

#CORE_OBJ_SRC         = $(wildcard $(CMSIS_SRC)/*.c)

#CORE_LIB_OBJS        = $(CORE_OBJ_SRC:.c=.o)
#CORE_LOCAL_LIB_OBJS  = $(notdir $(CORE_LIB_OBJS))

## Rules
all: size flash

#core.a: $(CORE_OBJ_SRC)
#	$(MAKE) $(CORE_LOCAL_LIB_OBJS)
#	$(AR) rcs core.a $(CORE_LOCAL_LIB_OBJS)
#	rm -f $(CORE_LOCAL_LIB_OBJS)

main.elf: $(OBJS)
	$(LD) -g $(LFLAGS) -o main.elf $(OBJS)

%.bin: %.elf
	$(OBJCOPY) --strip-unneeded -O binary $< $@

## Convenience targets

flash: main.bin
	sudo openocd -f interface/stlink-v2.cfg -f target/stm32f1x.cfg -c "program $^ reset exit 0x08000000"
	#st-flash --reset write $< 0x8000000 

size: main.elf
	$(SIZE) $< 

clean:
	-rm -f $(OBJS) main.lst main.elf main.hex main.map main.bin main.list

distclean: clean
	-rm -f *.o core.a $(CORE_LIB_OBJS) $(CORE_LOCAL_LIB_OBJS) 

.PHONY: all flash size clean distclean
