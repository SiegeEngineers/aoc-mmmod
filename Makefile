CC = i686-w64-mingw32-gcc
WINDRES = i686-w64-mingw32-windres

LOADER_NAME = language_x1_p1.dll

OPTFLAGS = -O3 -s
DBGFLAGS = -DDEBUG -g
ifeq ($(RELEASE),1)
  FLAGS = $(OPTFLAGS)
else
  FLAGS = $(DBGFLAGS)
endif

all: $(LOADER_NAME)

clean:
	rm -f $(LOADER_NAME) strings.res

.PHONY: all clean

strings.res: strings.rc
	$(WINDRES) -o $@ -O coff $<
$(LOADER_NAME): *.c strings.res
	$(CC) -o $@ -Wall -m32 $(FLAGS) -shared -static-libgcc $^
