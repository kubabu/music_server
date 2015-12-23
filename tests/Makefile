## Project structure
ODIR=./obj
LDIR =../lib
BINDIR=../bin

.PHONY: clean test_server_with_valgrind test_run_client

clean:
	rm -f $(ODIR)/*.o *~ $(INCDIR)/*~ $(BINDIR)/*


## tests in make are only temporary solution
VALGRIND_FLAGS = --leak-check=full --track-origins=yes --show-leak-kinds=all
VALGRIND_SUPPRESS_GEN = --gen-suppressions=all
VALGRIND_SUPPRESS = --suppressions=filters.supp 

VALGRIND_FLAGS += $(VALGRIND_SUPPRESS_GEN) $(VALGRIND_SUPPRESS)

test_server_with_valgrind:
	valgrind $(VALGRIND_FLAGS) $(BINDIR)/server

test_client_run:
	valgrind $(BINDIR)/client localhost 1234

