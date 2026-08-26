# ==============================================
# 32位树莓派交叉编译专用 Makefile
# 兼容：树莓派3/4/5/CM0/CM3/CM4 32位系统 (Raspberry Pi OS 32-bit)
# 特别优化：树莓派CM0 Lite (512MB内存) 低内存环境
# ==============================================

# 交叉编译工具链（自动检测是否存在）
CROSS_COMPILE ?= arm-linux-gnueabihf-
CC=$(CROSS_COMPILE)gcc
CXX=$(CROSS_COMPILE)g++
AR=$(CROSS_COMPILE)ar
STRIP=$(CROSS_COMPILE)strip
OBJDUMP=$(CROSS_COMPILE)objdump

# 检查工具链是否存在
ifeq ($(shell which $(CC) 2>/dev/null),)
$(error 错误: 未找到交叉编译工具链 $(CC)！请先安装: sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf)
endif

# ==============================================
# 编译选项（根据目标板选择，默认CM0/CM3/CM4通用）
# ==============================================

# 树莓派CM0/CM3/CM4/3B+ 通用（推荐，兼容性最好）
CC_CFLAGS=-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard -mtune=cortex-a7 \
          -Wall -O2 -ffunction-sections -fdata-sections -fstack-protector-strong

# 树莓派4/5 专用（性能更好，不兼容CM0/CM3）
# CC_CFLAGS=-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard -mtune=cortex-a72 \
#           -Wall -O2 -ffunction-sections -fdata-sections -fstack-protector-strong

# 树莓派3 专用
# CC_CFLAGS=-march=armv8-a+crc -mfpu=neon-fp-armv8 -mfloat-abi=hard -mtune=cortex-a53 \
#           -Wall -O2 -ffunction-sections -fdata-sections -fstack-protector-strong

# 调试版本（用于gdb调试，关闭优化，保留符号）
# CC_CFLAGS=-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard -mtune=cortex-a7 \
#           -Wall -O0 -g -ggdb -DDEBUG

# ==============================================
# 通用编译选项
# ==============================================


# ==============================================
# 【必须配置】自定义sysroot路径（从CM0拷出的库文件目录）
# ==============================================
RPI_SYSROOT ?= $(HOME)/pro/raspberry_cm0_stable_20250916/rpi_sysroot_lib

# C编译器选项
CFLAGS += $(CC_CFLAGS)
CFLAGS += --sysroot=$(RPI_SYSROOT)
CFLAGS += -Wno-multichar -Wno-deprecated-declarations -Wno-unused-result
CFLAGS += -Wno-unused-variable -Wno-format -Wno-return-type
CFLAGS += -Wno-sign-compare -Wno-unused-label -Wno-missing-field-initializers
CFLAGS += -Wno-unused-function -Wno-unused-but-set-variable

# C++编译器选项
CXXFLAGS += $(CFLAGS)
CXXFLAGS += -Wno-register -Wno-pessimizing-move -Wno-class-memaccess
CXXFLAGS += -std=c++17 -fno-rtti -fno-exceptions  # LVGL不需要RTTI和异常，大幅减小体积

# 自动生成依赖文件
CFLAGS += -MMD -MP
CXXFLAGS += -MMD -MP

# ==============================================
# 链接选项（修复静态链接顺序问题）
# ==============================================
LINKFLAGS += --sysroot=$(RPI_SYSROOT)
# 注意：静态链接顺序非常重要！必须按照"依赖者在前，被依赖者在后"的顺序
LINKFLAGS += -Wl,--gc-sections -Wl,--as-needed
LINKFLAGS += -static-libgcc -static-libstdc++  # 只静态链接gcc和stdc++，避免libc静态链接的问题
# 完全静态链接（如果需要单文件部署，取消下面注释）
LINKFLAGS += -Wl,--allow-shlib-undefined  # 新增这一行
#LINKFLAGS += -static
LINKFLAGS += -lasound
LINKFLAGS += -lcamera
LINKFLAGS += -lcamera-base
LINKFLAGS += -ldrm
LINKFLAGS += -lpthread -lm -lrt -ldl 

# ==============================================
# LVGL 配置
# ==============================================

# LVGL 核心配置（必须！告诉LVGL配置文件位置）
LV_CONF_PATH = $(shell pwd)
LV_CONF_INCLUDE = -I$(LV_CONF_PATH)

# 包含LVGL Makefile
include lvgl/lvgl.mk
include ui_brookesia/ui_brookesia.mk
# ==============================================
# 本地源文件和头文件
# ==============================================

# 本地头文件路径
LOCAL_CFLAGS = -I. -I./port $(LV_CONF_INCLUDE)

# 源文件列表（自动递归查找，不需要手动添加）
SRCS_C = $(wildcard *.c)
SRCS_C += $(LVGL_CSRCS)
SRCS_C += $(wildcard port/*.c)
SRCS_C += $(wildcard audio/*.c)
# SRCS_C += $(wildcard network/*.c)
# SRCS_C += $(wildcard drivers/*.c)
SRCS_C += $(BROOKESIA_CSRCS)

SRCS_CXX += $(wildcard *.cpp)
SRCS_CXX += $(wildcard port/*.cpp)
SRCS_CXX += $(BROOKESIA_CXXSRCS)

# 目标文件和依赖文件
OBJS_C = $(patsubst %.c,%.o,$(SRCS_C))
OBJS_CXX = $(patsubst %.cpp,%.o,$(SRCS_CXX))
OBJS = $(OBJS_C) $(OBJS_CXX)
DEPS = $(OBJS:.o=.d)

# 输出文件名
OUT = $(shell basename $(shell pwd)).out

# ==============================================
# 主目标
# ==============================================

# 默认目标：编译并显示信息
all: check_toolchain $(OUT) size_info

# 检查工具链
check_toolchain:
	@echo "🔧 使用交叉编译工具链: $(CROSS_COMPILE)"
	@$(CC) --version | head -n1

# 链接目标（修复了静态链接顺序）
$(OUT): $(OBJS)
	@echo "🔗 链接目标文件..."
	$(CXX) -o $@ $^ $(LINKFLAGS)
	@echo "✂️  剥离调试符号..."
	$(STRIP) --strip-all $@

# 编译C文件
$(OBJS_C): %.o: %.c
	@echo "📝 编译: $<"
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) $(LVGL_CFLAGS) $(BROOKESIA_CFLAGS) -c $< -o $@

# 编译C++文件
$(OBJS_CXX): %.o: %.cpp
	@echo "📝 编译: $<"
	$(CXX) $(CXXFLAGS) $(LOCAL_CFLAGS) $(LVGL_CFLAGS) $(BROOKESIA_CXXFLAGS) -c $< -o $@

# 显示文件大小信息
size_info: $(OUT)
	@echo ""
	@echo "✅ 编译完成！"
	@echo "📦 输出文件: $(OUT)"
	@echo "📏 文件大小: $(shell ls -lh $(OUT) | awk '{print $$5}')"
	@echo "🏷️  架构验证: $(shell file $(OUT) | grep -o 'ARM.*')"
	@echo "💾 段大小分布:"
	@$(OBJDUMP) -h $(OUT) | grep -E "^\s+[0-9]+" | awk '{printf "   %-8s %8s\n", $$2, $$3}'

# ==============================================
# 辅助目标
# ==============================================

# 调试版本（保留符号，不剥离）
debug: CFLAGS += -O0 -g -ggdb -DDEBUG
debug: CXXFLAGS += -O0 -g -ggdb -DDEBUG
debug: STRIP = true  # 不剥离符号
debug: all
	@echo "🔍 调试版本编译完成，可使用gdb进行调试"

# 并行编译（推荐使用，速度更快）
fast:
	$(MAKE) -j$(shell nproc) all

# 彻底清理
clean:
	@echo "🧹 清理所有编译文件..."
	-rm -f $(OUT)
	-rm -f $(OBJS)
	-rm -f $(DEPS)
	-rm -f $(shell find . -name "*.o" -o -name "*.d")
	@echo "✅ 清理完成"

# 包含自动生成的依赖文件
-include $(DEPS)

.PHONY: all check_toolchain size_info debug fast clean
