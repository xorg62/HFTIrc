# HFTIRC Makefile
PREFIX?=	/usr/local

include	config.mk
include config.local.mk

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

CFLAGS+=	${INCLUDES} ${NCURSES5INCDIR} ${INCLIBIRCCLIENT}
CFLAGS+=	-Wall -g -D_XOPEN_SOURCE_EXTENDED -D_XOPEN_SOURCE -D_XOPEN_CURSES

LDFLAGS+=	${LIBDIR} ${NCURSES5LIBDIR} ${LIBIRCCLIENTLIB}

OBJ=$(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	@echo "LINK	$@"
	@$(CC) -o $@ $(OBJ) $(LDFLAGS) $(CFLAGS)

.c.o:
	@echo "CC	$<"
	@$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)
