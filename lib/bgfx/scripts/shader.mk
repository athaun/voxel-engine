#
# Copyright 2011-2020 Branimir Karadzic. All rights reserved.
# License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
#
# Define the current directory as THISDIR using the last makefile in the MAKEFILE_LIST
THISDIR:=$(dir $(lastword $(MAKEFILE_LIST)))

# Include the tools.mk file from the same directory as this makefile
include $(THISDIR)/tools.mk

# Define the SHADERS_DIR variable if it's not already defined
ifndef SHADERS_DIR
	# Set the default value for SHADERS_DIR
	SHADERS_DIR=shader_files/
endif

# If TARGET is not defined, define the all and rebuild targets
ifndef TARGET
.PHONY: all
all:
	@echo Usage: make TARGET=# [clean, all, rebuild]
	@echo "  TARGET=0 (hlsl  - d3d9)"
	@echo "  TARGET=1 (hlsl  - d3d11)"
	@echo "  TARGET=2 (essl  - nacl)"
	@echo "  TARGET=3 (essl  - android)"
	@echo "  TARGET=4 (glsl)"
	@echo "  TARGET=5 (metal)"
	@echo "  TARGET=6 (pssl)"
	@echo "  TARGET=7 (spirv)"

.PHONY: rebuild
rebuild:
	@make -s --no-print-directory TARGET=0 clean all
	@make -s --no-print-directory TARGET=1 clean all
	@make -s --no-print-directory TARGET=2 clean all
	@make -s --no-print-directory TARGET=3 clean all
	@make -s --no-print-directory TARGET=4 clean all
	@make -s --no-print-directory TARGET=5 clean all
	@make -s --no-print-directory TARGET=7 clean all

else

# Define ADDITIONAL_INCLUDES variable if it's not already defined
ifndef ADDITIONAL_INCLUDES
	# Set the default value for ADDITIONAL_INCLUDES
	ADDITIONAL_INCLUDES=
endif

# Define VS_FLAGS and FS_FLAGS based on the TARGET value
ifeq ($(TARGET), 0)
	VS_FLAGS=--platform windows -p vs_3_0 -O 3
	FS_FLAGS=--platform windows -p ps_3_0 -O 3
	SHADER_PATH=shaders/dx9
else ifeq ($(TARGET), 1)
	VS_FLAGS=--platform windows -p vs_5_0 -O 3
	FS_FLAGS=--platform windows -p ps_5_0 -O 3
	CS_FLAGS=--platform windows -p cs_5_0 -O 1
	SHADER_PATH=shaders/dx11
else ifeq ($(TARGET), 2)
	VS_FLAGS=--platform nacl
	FS_FLAGS=--platform nacl
	SHADER_PATH=shaders/essl
else ifeq ($(TARGET), 3)
	VS_FLAGS=--platform android
	FS_FLAGS=--platform android
	CS_FLAGS=--platform android
	SHADER_PATH=shaders/essl
else ifeq ($(TARGET), 4)
	VS_FLAGS=--platform linux -p 120
	FS_FLAGS=--platform linux -p 120
	CS_FLAGS=--platform linux -p 430
	SHADER_PATH=shaders/glsl
else ifeq ($(TARGET), 5)
	VS_FLAGS=--platform osx -p metal
	FS_FLAGS=--platform osx -p metal
	CS_FLAGS=--platform osx -p metal
	SHADER_PATH=shaders/metal
else ifeq ($(TARGET), 6)
	VS_FLAGS=--platform orbis -p pssl
	FS_FLAGS=--platform orbis -p pssl
	CS_FLAGS=--platform orbis -p pssl
	SHADER_PATH=shaders/pssl
else ifeq ($(TARGET), 7)
	VS_FLAGS=--platform linux -p spirv
	FS_FLAGS=--platform linux -p spirv
	CS_FLAGS=--platform linux -p spirv
	SHADER_PATH=shaders/spirv
endif

# Set THISDIR to the current directory
THISDIR := $(dir $(lastword $(MAKEFILE_LIST)))

# Add the source directory and additional includes to VS_FLAGS, FS_FLAGS, and CS_FLAGS
VS_FLAGS+=-i $(THISDIR)../src/ $(ADDITIONAL_INCLUDES)
FS_FLAGS+=-i $(THISDIR)../src/ $(ADDITIONAL_INCLUDES)
CS_FLAGS+=-i $(THISDIR)../src/ $(ADDITIONAL_INCLUDES)

# Set the build output and intermediate directories based on the SHADER_PATH
BUILD_OUTPUT_DIR=$(addprefix ./, $(RUNTIME_DIR)/$(SHADER_PATH))
BUILD_INTERMEDIATE_DIR=$(addprefix $(BUILD_DIR)/, $(SHADER_PATH))

# Get the list of vertex shader sources and their corresponding dependencies
VS_SOURCES=$(notdir $(wildcard $(addprefix $(SHADERS_DIR), vs_*.sc)))
VS_DEPS=$(addprefix $(BUILD_INTERMEDIATE_DIR)/,$(addsuffix .bin.d, $(basename $(notdir $(VS_SOURCES)))))

# Get the list of fragment shader sources and their corresponding dependencies
FS_SOURCES=$(notdir $(wildcard $(addprefix $(SHADERS_DIR), fs_*.sc)))
FS_DEPS=$(addprefix $(BUILD_INTERMEDIATE_DIR)/,$(addsuffix .bin.d, $(basename $(notdir $(FS_SOURCES)))))

# Get the list of compute shader sources and their corresponding dependencies
CS_SOURCES=$(notdir $(wildcard $(addprefix $(SHADERS_DIR), cs_*.sc)))
CS_DEPS=$(addprefix $(BUILD_INTERMEDIATE_DIR)/,$(addsuffix .bin.d, $(basename $(notdir $(CS_SOURCES)))))

# Get the list of vertex shader binaries
VS_BIN = $(addprefix $(BUILD_INTERMEDIATE_DIR)/, $(addsuffix .bin, $(basename $(notdir $(VS_SOURCES)))))

# Get the list of fragment shader binaries
FS_BIN = $(addprefix $(BUILD_INTERMEDIATE_DIR)/, $(addsuffix .bin, $(basename $(notdir $(FS_SOURCES)))))

# Get the list of compute shader binaries
CS_BIN = $(addprefix $(BUILD_INTERMEDIATE_DIR)/, $(addsuffix .bin, $(basename $(notdir $(CS_SOURCES)))))

# Combine all the shader binaries into a single BIN variable
BIN = $(VS_BIN) $(FS_BIN)
ASM = $(VS_ASM) $(FS_ASM)

# Add compute shader binaries and assembly files to BIN and ASM if the TARGET supports it
ifeq ($(TARGET), $(filter $(TARGET),1 3 4 5 6 7))
	BIN += $(CS_BIN)
	ASM += $(CS_ASM)
endif

# Rule to compile vertex shader sources into binaries
$(BUILD_INTERMEDIATE_DIR)/vs_%.bin: $(SHADERS_DIR)vs_%.sc
	@echo "HELLO?"
	@echo "$(SHADERC) $(VS_FLAGS) --type vertex --depends -o $(@) -f $(<) --disasm"
	$(SHADERC) $(VS_FLAGS) --type vertex --depends -o $(@) -f $(<) --disasm
	$(SILENT) cp $(@) $(BUILD_OUTPUT_DIR)/$(@F)

# Rule to compile fragment shader sources into binaries
$(BUILD_INTERMEDIATE_DIR)/fs_%.bin: $(SHADERS_DIR)fs_%.sc
	@echo [$(<)]
	$(SILENT) $(SHADERC) $(FS_FLAGS) --type fragment --depends -o $(@) -f $(<) --disasm
	$(SILENT) cp $(@) $(BUILD_OUTPUT_DIR)/$(@F)

# Rule to compile compute shader sources into binaries
$(BUILD_INTERMEDIATE_DIR)/cs_%.bin: $(SHADERS_DIR)cs_%.sc
	@echo [$(<)]
	$(SILENT) $(SHADERC) $(CS_FLAGS) --type compute --depends -o $(@) -f $(<) --disasm
	$(SILENT) cp $(@) $(BUILD_OUTPUT_DIR)/$(@F)

# Target to build all shaders
.PHONY: all
all: dirs $(BIN)
	@echo Target $(SHADER_PATH)

# Target to clean the generated binaries
.PHONY: clean
clean:
	@echo Cleaning...
	@-rm -vf $(BIN)

# Target to clean all generated files
.PHONY: cleanall
cleanall:
	@echo Cleaning...
	@-$(call CMD_RMDIR,$(BUILD_INTERMEDIATE_DIR))

# Target to create the build and output directories
.PHONY: dirs
dirs:
	@-$(call CMD_MKDIR,$(BUILD_INTERMEDIATE_DIR))
	@-$(call CMD_MKDIR,$(BUILD_OUTPUT_DIR))

# Target to rebuild all shaders
.PHONY: rebuild
rebuild: clean all

# Include the generated dependencies
-include $(VS_DEPS)
-include $(FS_DEPS)
-include $(CS_DEPS)

endif # TARGET