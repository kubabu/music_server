CC=gcc
IDIR =../include
CFLAGS=-Wall -I$(IDIR)
LDFLAGS= #-static

## Project structure
ODIR=./obj
LDIR =../lib
BINDIR=../bin

## Libraries
LIBS=-pthread
LIBINCLUDE=$(LDIR)
LIBBIN=$(LDIR)
CFLAGS+=-I$(LIBINCLUDE)
LDFLAGS+=-L$(LIBBIN)

LIBS += -lmpg123
MPG123PATH=../lib/mpg123-1.22.4/src/libmpg123
CFLAGS += -I$(MPG123PATH)
LDFLAGS += -L$(MPG123PATH)

LIBS += -lao
LIBAOINCL=../lib/libao-1.2.0/include
CFLAGS += -I$(LIBAOINCL)
LIBAOBIN=../lib/libao-1.2.0/src
LDFLAGS += -L$(LIBAOBIN)
LIBS += -lao


## Generic rules
## Build object files as non executable
$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

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

.PHONY: all clean clean_build test_server_with_valgrind test_run_client

clean_build: clean all

clean:
	rm -f $(ODIR)/*.o *~ $(INCDIR)/*~ $(BINDIR)/*

## tests in make are only temporary solution
VALGRIND_FLAGS = --leak-check=full --track-origins=yes
## tests together with builds is a mess and will be removed
test_server_with_valgrind: server
	valgrind $(VALGRIND_FLAGS) $(BINDIR)/server

test_run_client: client
	valgrind $(BINDIR)/client localhost 1234

