#! /usr/bin/python
"""Get suppress configurations from Valgrind output"""
# Run Valgrind with --gen-suppresions=all 2>memcheck_dump
# then this will add suppress filters to given file
import subprocess
import os


STANDALONE = 1
VERBOSE = 1


def main():
    "main function, stop bothering mem pyflakes"
    memcheck_dump_fname = "temp_filters.supp"

    supp_filters = "filters_libao.supp"

    if STANDALONE:
        subprocess.call(["make", "test_server_with_dump"])

    val_out_lines = []
    with open(memcheck_dump_fname, "r") as val_out:
        val_out_lines = val_out.readlines()
    if VERBOSE:
        for dump_line in val_out_lines:
            print dump_line

    val_suppressions = [line for line in val_out_lines
                        if not line.startswith("==")]

    with open(supp_filters, "a+") as val_suppressions_filters:
        for supp_line in val_suppressions:
            val_suppressions_filters.write(supp_line)

    os.remove(memcheck_dump_fname)


if __name__ == "__main__":
    main()
