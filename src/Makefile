CC=gcc
LIBTOOL=$(LDIR)/bin/libtool
IDIR =../include
CFLAGS= -I$(IDIR) -Wall -Wextra -pedantic ## -Werror
LDFLAGS= #-static

## Debug build
CFLAGS += -g -DDEBUG


## Project structure
ODIR=./obj
LDIR =../lib
BINDIR=../bin

## Libraries
LIBS=-pthread -lgcc_s
LIBINCLUDE=$(LDIR)
LIBBIN=$(LDIR)
CFLAGS+=-I$(LIBINCLUDE)
LDFLAGS+=-L$(LIBBIN)

LIBS += -lmpg123
LIBS += -lao
LIBS += -lcurl

## Generic rules
## Build object files as non executable
$(ODIR)/%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

## Exact rules
all: server client

_SERVER_OBJ = main.o mplayer.o utils.o
SERVER_OBJ = $(patsubst %,$(ODIR)/%,$(_SERVER_OBJ))

server: $(SERVER_OBJ)
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $(BINDIR)/$@

_CLIENT_OBJ = client.o utils.o
CLIENT_OBJ = $(patsubst %,$(ODIR)/%,$(_CLIENT_OBJ))

client: $(CLIENT_OBJ)
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $(BINDIR)/$@

idv3dump: idv3dump.o
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $(BINDIR)/$@

.PHONY: all clean clean_build

clean_build: clean all

clean:
	rm -f $(ODIR)/*.o *~ $(INCDIR)/*~ $(BINDIR)/*

