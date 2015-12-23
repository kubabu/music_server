## Project structure
ODIR=./obj
LDIR =../lib
BINDIR=../bin

.PHONY: clean test_server_with_valgrind test_client_run

clean:
	rm -f $(ODIR)/*.o *~ $(INCDIR)/*~ $(BINDIR)/*


## tests in make are only temporary solution
VALGRIND_FLAGS = --leak-check=full --track-origins=yes --show-leak-kinds=all
VALGRIND_SUPPRESS_GEN = --gen-suppressions=all
VALGRIND_SUPPRESS = --suppressions=filters_libao.supp

VALGRIND_FLAGS +=  $(VALGRIND_SUPPRESS)
VALGRIND_DUMP = temp_filters.supp

test_server_with_dump:
	valgrind $(VALGRIND_FLAGS) $(VALGRIND_SUPPRESS_GEN) $(BINDIR)/server 2>$(VALGRIND_DUMP)

test_server_with_valgrind:
	valgrind $(VALGRIND_FLAGS) $(BINDIR)/server

test_client_run:
	valgrind $(BINDIR)/client localhost 1234

