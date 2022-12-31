
OS = LINUX
#OS = MACOSX

PROG = video_start_button

ifeq ($(OS), LINUX)
TARGET = $(PROG)
CC = gcc
STRIP = strip
CFLAGS = -Wall -O2 -DOS_$(OS)
LIBS = -lusb
else ifeq ($(OS), MACOSX)
TARGET = $(PROG)
#SDK = /Developer/SDKs/MacOSX10.5.sdk
#ARCH = -mmacosx-version-min=10.14
SDK =
ARCH =
CC = gcc
STRIP = strip
#CFLAGS = -Wall -O2 -DOS_$(OS) -isysroot $(SDK) $(ARCH)
CFLAGS = -Wall -O2 -DOS_$(OS)
#LIBS = $(ARCH) -Wl,-syslibroot,$(SDK) -framework IOKit -framework CoreFoundation
LIBS = $(ARCH) -framework IOKit -framework CoreFoundation
endif

OBJS = $(PROG).o hid.o


all: $(TARGET)

$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(OBJS) $(LIBS)
	$(STRIP) $(PROG)

$(PROG).exe: $(PROG)
	cp $(PROG) $(PROG).exe

$(PROG).dmg: $(PROG)
	mkdir tmp
	cp $(PROG) tmp
	hdiutil create -ov -volname "Raw HID Test" -srcfolder tmp $(PROG).dmg

hid.o: hid_$(OS).c hid.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(PROG) $(PROG).exe $(PROG).dmg
	rm -rf tmp

