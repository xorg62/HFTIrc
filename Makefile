# HFTIRC Makefile

EXEC=hftirc
SRC= confparse/util.c      \
     confparse/confparse.c \
     confparse/confparse.h \
     config.c \
     ui.c     \
     irc.c    \
     hftirc.c \
     util.c   \
     input.c  \
     hftirc.h

CFLAGS+=-Wall -g -D_XOPEN_SOURCE_EXTENDED -D_XOPEN_SOURCE -D_XOPEN_CURSES
LDFLAGS+=-lncursesw -lpthread /usr/lib/libircclient.a

OBJ=$(SRC:.c=.o)
UNAME=$(shell uname)

ifeq ($(UNAME),Linux)
	CFLAGS += -D_GNU_SOURCE
endif

ifeq ($(UNAME),FreeBSD)
    CFLAGS+=-I/usr/local/include
    LDFLAGS+=-L/usr/local/lib
endif

all: $(EXEC)

$(EXEC): $(OBJ)
	@echo "LINK	$@"
	@$(CC) -o $@ $(OBJ) $(LDFLAGS) $(CFLAGS)

.c.o:
	@echo "CC	$<"
	@$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	@rm -rf *.o

mrproper: clean
	@rm -rf $(EXEC)
