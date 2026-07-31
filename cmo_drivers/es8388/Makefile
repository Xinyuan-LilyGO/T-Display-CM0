# 模块驱动，必须以obj-m=xxx形式编写
ccflags-y := -fno-stack-protector
obj-m += snd-soc-es8328-i2c.o
snd-soc-es8328-i2c-objs := es8328.o es8328-i2c.o

KDIR = /home/admin/pro/raspberry_cm0_stable_20250916/linux-stable_20250916
CROSS = ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

all:
	$(MAKE) -C $(KDIR) M=$(PWD) $(CROSS) modules

clean:
	$(MAKE) -C $(KDIR) M=`pwd` $(CROSS) clean
