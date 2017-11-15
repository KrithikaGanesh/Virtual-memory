LINUX_KERN ?= /lib/modules/`uname -r`/build/

EXTRA_CFLAGS  += -DMODULE=1 -D__KERNEL__=1


petmem-y := 	main.o \
		buddy.o \
		on_demand.o 

petmem-objs := $(petmem-y)
obj-m := petmem.o


all:
	$(MAKE) -C $(LINUX_KERN) M=$(PWD) modules

clean:
	$(MAKE) -C $(LINUX_KERN) M=$(PWD) clean

