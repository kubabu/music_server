IDIR =../include
CC=gcc
CFLAGS=-Wall -I$(IDIR)
LDFLAGS= ##-static

ODIR=obj
LDIR =../lib

LIBINCLUDE=../lib/
LIBBIN=../lib/
LIBS=-pthread

CFLAGS+=-I$(LIBINCLUDE)
LDFLAGS+=-L$(LIBBIN)

BINDIR=../bin

_DEPS = controls.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_SERVER_OBJ = main.o
SERVER_OBJ = $(patsubst %,$(ODIR)/%,$(_SERVER_OBJ))

_CLIENT_OBJ = client.o
CLIENT_OBJ = $(patsubst %,$(ODIR)/%,$(_CLIENT_OBJ))


$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(SERVER_OBJ)
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $(BINDIR)/$@

client: $(CLIENT_OBJ)
	$(CC) $^ $(LDFLAGS) $(LIBS) -o $(BINDIR)/$@

.PHONY: clean valgrind_test_server

clean:
	rm -f $(ODIR)/*.o *~ $(INCDIR)/*~ $(BINDIR)/* 


VALGRIND_FLAGS = --leak-check=full --track-origins=yes
## tests together with builds is a mess and will be removed 
test_server_with_valgrind:
	valgrind $(VALGRIND_FLAGS) $(BINDIR)/server

test_run_client:
	$(BINDIR)/client localhost 1234
