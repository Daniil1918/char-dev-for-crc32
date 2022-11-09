obj-m += crcdev.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	gcc test-crc32.c -o test

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rm test

load:
	sudo insmod crcdev.ko

unload:
	sudo rmmod crcdev.ko