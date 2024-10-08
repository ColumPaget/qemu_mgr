OBJ=common.o command-line.o os-commands.o kvm.o devices.o sound-devices.o actions.o interactive.o interactive_xdialog.o interactive_vt100.o tap-netcfg.o images.o image-config.o config-templates.o qmp.o mount.o vnc.o screenshot.o 

FLAGS=-g -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_UNISTD_H=1 -DSTDC_HEADERS=1 -D_FILE_OFFSET_BITS=64 -DHAVE_LIBUSEFUL_5_LIBUSEFUL_H=1 -DHAVE_LIBUSEFUL_5=1 -DHAVE_LIBSSL=1 -DHAVE_LIBCRYPTO=1
LIBS=-lcrypto -lssl -lUseful-5  

all: $(OBJ)
	gcc $(FLAGS) -oqemu_mgr main.c $(OBJ) $(LIBS)

common.o: common.h common.c
	gcc $(FLAGS) -c common.c

kvm.o: kvm.h kvm.c
	gcc $(FLAGS) -c kvm.c

devices.o: devices.h devices.c
	gcc $(FLAGS) -c devices.c

command-line.o: command-line.h command-line.c
	gcc $(FLAGS) -c command-line.c

sound-devices.o: sound-devices.h sound-devices.c
	gcc $(FLAGS) -c sound-devices.c

actions.o: actions.h actions.c
	gcc $(FLAGS) -c actions.c

images.o: images.h images.c
	gcc $(FLAGS) -c images.c

image-config.o: image-config.h image-config.c
	gcc $(FLAGS) -c image-config.c

config-templates.o: config-templates.h config-templates.c
	gcc $(FLAGS) -c config-templates.c

interactive.o: interactive.h interactive.c
	gcc $(FLAGS) -c interactive.c

interactive_xdialog.o: interactive_xdialog.h interactive_xdialog.c
	gcc $(FLAGS) -c interactive_xdialog.c

interactive_vt100.o: interactive_vt100.h interactive_vt100.c
	gcc $(FLAGS) -c interactive_vt100.c

tap-netcfg.o: tap-netcfg.h tap-netcfg.c
	gcc $(FLAGS) -c tap-netcfg.c

os-commands.o: os-commands.h os-commands.c
	gcc $(FLAGS) -c os-commands.c

qmp.o: qmp.h qmp.c
	gcc $(FLAGS) -c qmp.c

mount.o: mount.h mount.c
	gcc $(FLAGS) -c mount.c

vnc.o: vnc.h vnc.c
	gcc $(FLAGS) -c vnc.c

screenshot.o: screenshot.h screenshot.c
	gcc $(FLAGS) -c screenshot.c

libUseful-4/libUseful.a:
	$(MAKE) -C libUseful-4

clean:
	rm -f *.o *.orig */*.o */*.so */*.a qemu_mgr

test:
	echo "no tests"

