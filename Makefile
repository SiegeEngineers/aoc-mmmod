CC = i686-w64-mingw32-gcc
WINDRES = i686-w64-mingw32-windres

OPTFLAGS = -O3 -s
DBGFLAGS = -DDEBUG -g

ifeq ($(RELEASE),1)
  FLAGS = $(OPTFLAGS)
else
  FLAGS = $(DBGFLAGS)
endif

all: mmmod.dll

clean:
	rm -f mmmod.dll strings.res

.PHONY: all clean

strings.res: strings.rc
	$(WINDRES) -o $@ -O coff $<
mmmod.dll: hook.c main.c strings.res
	$(CC) -o $@ -Wall -m32 $(FLAGS) -shared $^
