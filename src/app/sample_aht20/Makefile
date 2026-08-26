# ===================== 配置项（你只需要改这里）=====================
# 交叉编译器（树莓派3/4/CM0 通用）
CC = arm-linux-gnueabihf-gcc
# 最终生成的可执行文件名字
TARGET = aht20
# 你的 C 语言源文件
SRC = aht20.c
# 编译参数（开启警告，标准C语言）
CFLAGS = -Wall -O2 -std=c99
# ==================================================================

# 默认编译目标
all: $(TARGET)

# 链接生成可执行文件
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

# 清理编译产物
clean:
	rm -f $(TARGET) *.o

# 上传到树莓派（可选，把IP和路径改成你的）
deploy: $(TARGET)
	scp $(TARGET) pi@树莓派IP:/home/pi/
