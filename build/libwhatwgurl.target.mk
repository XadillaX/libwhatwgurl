# This file is generated by gyp; do not edit.

TOOLSET := target
TARGET := libwhatwgurl
DEFS_Debug := \
	'-DNODE_GYP_MODULE_NAME=libwhatwgurl' \
	'-DUSING_UV_SHARED=1' \
	'-DUSING_V8_SHARED=1' \
	'-DV8_DEPRECATION_WARNINGS=1' \
	'-DV8_DEPRECATION_WARNINGS' \
	'-DV8_IMMINENT_DEPRECATION_WARNINGS' \
	'-D_LARGEFILE_SOURCE' \
	'-D_FILE_OFFSET_BITS=64' \
	'-D__STDC_FORMAT_MACROS' \
	'-DOPENSSL_NO_PINSHARED' \
	'-DOPENSSL_THREADS' \
	'-DDEBUG' \
	'-D_DEBUG' \
	'-DV8_ENABLE_CHECKS'

# Flags passed to all source files.
CFLAGS_Debug := \
	-fPIC \
	-pthread \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-m64 \
	-g \
	-O0 \
	-g \
	-O0

# Flags passed to only C files.
CFLAGS_C_Debug :=

# Flags passed to only C++ files.
CFLAGS_CC_Debug := \
	-fno-rtti \
	-fno-exceptions \
	-std=gnu++1y

INCS_Debug := \
	-I/home/xadillax/.cache/node-gyp/14.18.2/include/node \
	-I/home/xadillax/.cache/node-gyp/14.18.2/src \
	-I/home/xadillax/.cache/node-gyp/14.18.2/deps/openssl/config \
	-I/home/xadillax/.cache/node-gyp/14.18.2/deps/openssl/openssl/include \
	-I/home/xadillax/.cache/node-gyp/14.18.2/deps/uv/include \
	-I/home/xadillax/.cache/node-gyp/14.18.2/deps/zlib \
	-I/home/xadillax/.cache/node-gyp/14.18.2/deps/v8/include \
	-I$(srcdir)/include

DEFS_Release := \
	'-DNODE_GYP_MODULE_NAME=libwhatwgurl' \
	'-DUSING_UV_SHARED=1' \
	'-DUSING_V8_SHARED=1' \
	'-DV8_DEPRECATION_WARNINGS=1' \
	'-DV8_DEPRECATION_WARNINGS' \
	'-DV8_IMMINENT_DEPRECATION_WARNINGS' \
	'-D_LARGEFILE_SOURCE' \
	'-D_FILE_OFFSET_BITS=64' \
	'-D__STDC_FORMAT_MACROS' \
	'-DOPENSSL_NO_PINSHARED' \
	'-DOPENSSL_THREADS'

# Flags passed to all source files.
CFLAGS_Release := \
	-fPIC \
	-pthread \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-m64 \
	-O3 \
	-O3 \
	-fno-omit-frame-pointer

# Flags passed to only C files.
CFLAGS_C_Release :=

# Flags passed to only C++ files.
CFLAGS_CC_Release := \
	-fno-rtti \
	-fno-exceptions \
	-std=gnu++1y

INCS_Release := \
	-I/home/xadillax/.cache/node-gyp/14.18.2/include/node \
	-I/home/xadillax/.cache/node-gyp/14.18.2/src \
	-I/home/xadillax/.cache/node-gyp/14.18.2/deps/openssl/config \
	-I/home/xadillax/.cache/node-gyp/14.18.2/deps/openssl/openssl/include \
	-I/home/xadillax/.cache/node-gyp/14.18.2/deps/uv/include \
	-I/home/xadillax/.cache/node-gyp/14.18.2/deps/zlib \
	-I/home/xadillax/.cache/node-gyp/14.18.2/deps/v8/include \
	-I$(srcdir)/include

OBJS := \
	$(obj).target/$(TARGET)/src/host/host.o \
	$(obj).target/$(TARGET)/src/host/ip_util.o \
	$(obj).target/$(TARGET)/src/utils/assert.o \
	$(obj).target/$(TARGET)/src/code_points.o \
	$(obj).target/$(TARGET)/src/idna.o \
	$(obj).target/$(TARGET)/src/parse.o \
	$(obj).target/$(TARGET)/src/path.o \
	$(obj).target/$(TARGET)/src/percent_encode-data.o \
	$(obj).target/$(TARGET)/src/percent_encode.o \
	$(obj).target/$(TARGET)/src/scheme.o \
	$(obj).target/$(TARGET)/src/temp_string_buffer.o \
	$(obj).target/$(TARGET)/src/url_core.o \
	$(obj).target/$(TARGET)/src/url_search_params.o

# Add to the list of files we specially track dependencies for.
all_deps += $(OBJS)

# CFLAGS et al overrides must be target-local.
# See "Target-specific Variable Values" in the GNU Make manual.
$(OBJS): TOOLSET := $(TOOLSET)
$(OBJS): GYP_CFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_C_$(BUILDTYPE))
$(OBJS): GYP_CXXFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_CC_$(BUILDTYPE))

# Suffix rules, putting all outputs into $(obj).

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(srcdir)/%.cc FORCE_DO_CMD
	@$(call do_cmd,cxx,1)

# Try building from generated source, too.

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj).$(TOOLSET)/%.cc FORCE_DO_CMD
	@$(call do_cmd,cxx,1)

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj)/%.cc FORCE_DO_CMD
	@$(call do_cmd,cxx,1)

# End of this set of suffix rules
### Rules for final target.
LDFLAGS_Debug := \
	-pthread \
	-rdynamic \
	-m64

LDFLAGS_Release := \
	-pthread \
	-rdynamic \
	-m64

LIBS :=

$(obj).target/whatwgurl.a: GYP_LDFLAGS := $(LDFLAGS_$(BUILDTYPE))
$(obj).target/whatwgurl.a: LIBS := $(LIBS)
$(obj).target/whatwgurl.a: TOOLSET := $(TOOLSET)
$(obj).target/whatwgurl.a: $(OBJS) FORCE_DO_CMD
	$(call do_cmd,alink)

all_deps += $(obj).target/whatwgurl.a
# Add target alias
.PHONY: libwhatwgurl
libwhatwgurl: $(obj).target/whatwgurl.a

# Add target alias to "all" target.
.PHONY: all
all: libwhatwgurl

# Add target alias
.PHONY: libwhatwgurl
libwhatwgurl: $(builddir)/whatwgurl.a

# Copy this to the static library output path.
$(builddir)/whatwgurl.a: TOOLSET := $(TOOLSET)
$(builddir)/whatwgurl.a: $(obj).target/whatwgurl.a FORCE_DO_CMD
	$(call do_cmd,copy)

all_deps += $(builddir)/whatwgurl.a
# Short alias for building this static library.
.PHONY: whatwgurl.a
whatwgurl.a: $(obj).target/whatwgurl.a $(builddir)/whatwgurl.a

# Add static library to "all" target.
.PHONY: all
all: $(builddir)/whatwgurl.a

