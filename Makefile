
TARGET = stromplatine3

# compiler
CROSS_COMPILE_PREFIX = arm-none-eabi-
CC  = $(CROSS_COMPILE_PREFIX)gcc
GDB = $(CROSS_COMPILE_PREFIX)gdb


LIBS = m c
LIB_PATHS =
INCLUDES = src/ \
	src/target/include \
	src/framework/include

LINKER_SCRIPT_FILE = src/target/stm32f0/stm32.ld
MAP_FILE = $(TARGET).map

# Flags
CPPFLAGS = -c -std=c99 -ggdb -Wall -Wextra -mthumb -mcpu=cortex-m0 -msoft-float -DSTM32F0
LINKERFLAGS =  -mthumb -mcpu=cortex-m0 -msoft-float -nostartfiles -Wl,-Map=$(TARGET).map -T$(LINKER_SCRIPT_FILE)

ifdef RELEASE
CPPFLAGS += -O2
else 
CPPFLAGS += -O0
endif

C_SUFFIX = .c
OBJ_SUFFIX = .o
DEP_SUFFIX = .d
OBJ_DIR = obj/

# files that should be listed first when linking
IMPORTANT_ORDER_FILES = vectorISR.c
IGNORE_STRINGS = */archive/*


IGNORE_STRINGS += $(IMPORTANT_ORDER_FILES)
C_FILES   = $(sort $(filter-out $(IGNORE_STRINGS), $(shell find src -name "*$(C_SUFFIX)" | grep -v $(addprefix -e, $(IGNORE_STRINGS)))))
IMPORTANT_C_FILES = $(sort $(filter-out $(IGNORE_STRINGS), $(shell find src -name "*$(C_SUFFIX)" | grep $(addprefix -e, $(IGNORE_STRINGS)))))
OBJ_FILES = $(addprefix $(OBJ_DIR), $(C_FILES:%$(C_SUFFIX)=%$(OBJ_SUFFIX)) $(IMPORTANT_C_FILES:%$(C_SUFFIX)=%$(OBJ_SUFFIX)))
DEP_FILES = $(addprefix $(OBJ_DIR), $(C_FILES:%$(C_SUFFIX)=%$(DEP_SUFFIX)) $(IMPORTANT_C_FILES:%$(C_SUFFIX)=%$(DEP_SUFFIX)))


INCLUDE_CMD = $(addprefix -I, $(INCLUDES))
LIB_CMD = $(addprefix -l, $(LIBS))
LIB_PATH_CMD = $(addprefix -L, $(LIB_PATHS))

ifndef VERBOSE
SILENT = @
endif


.phyony: all clean flash

all: $(TARGET).elf

clean:
	rm -rf $(OBJ_DIR) $(TARGET).elf $(TARGET).map

depend: $(DEP_FILES)


$(TARGET).elf: $(OBJ_FILES)
	$(SILENT) echo linking $(target)
	$(SILENT) $(CC) -o $@ $^ $(LINKERFLAGS) $(LIB_PATH_CMD) $(LIB_CMD)
	@ echo done

$(OBJ_DIR)%$(OBJ_SUFFIX): %$(C_SUFFIX)
	@echo building $<
	@ mkdir -p $(dir $@)
	$(SILENT) $(CC) $(CPPFLAGS) $(INCLUDE_CMD) -o $@ $<
	@ mkdir -p $(dir $@)
	@ $(CC) $(CPPFLAGS) $(INCLUDE_CMD) -MM -MF $(OBJ_DIR)$*.d $<
	@ mv -f $(OBJ_DIR)$*.d $(OBJ_DIR)$*.d.tmp
	@ sed -e 's|.*:|$@:|' < $(OBJ_DIR)$*.d.tmp > $(OBJ_DIR)$*.d
	@ rm -f $(OBJ_DIR)$*.d.tmp
	
docu: $(TARGET).doxyfile $(C_FILES)
	doxygen $(TARGET).doxyfile

flash: $(TARGET).elf
	$(SILENT) $(GDB) -batch -x bin/flashGDB $<

dbg:
	@echo $(OBJ_FILES)
	@echo $(DEP_FILES)

-include $(DEP_FILES)
