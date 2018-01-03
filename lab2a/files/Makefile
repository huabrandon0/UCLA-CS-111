# NAME: Brandon Hua
# EMAIL: huabrandon0@gmail.com
# ID: 804595738
# Makefile

CC = gcc

UID = 804595738

EXECUTABLE_ADD = lab2_add
EXECUTABLE_LIST = lab2_list

CSV_ADD = lab2_add.csv
CSV_LIST = lab2_list.csv

PNG_ADD1 = lab2_add-1.png
PNG_ADD2 = lab2_add-2.png
PNG_ADD3 = lab2_add-3.png
PNG_ADD4 = lab2_add-4.png
PNG_ADD5 = lab2_add-5.png

PNG_LIST1 = lab2_list-1.png
PNG_LIST2 = lab2_list-2.png
PNG_LIST3 = lab2_list-3.png
PNG_LIST4 = lab2_list-4.png

SOURCE_FILES = lab2_add.c SortedList.h SortedList.c lab2_list.c
EXECUTABLES = $(EXECUTABLE_ADD) $(EXECUTABLE_LIST)
CSVS = $(CSV_ADD) $(CSV_LIST)
PNGS = $(PNG_ADD1) $(PNG_ADD2) $(PNG_ADD3) $(PNG_ADD4) $(PNG_ADD5) $(PNG_LIST1) $(PNG_LIST2) $(PNG_LIST3) $(PNG_LIST4)
SCRIPTS = add_tests.sh list_tests.sh lab2_add.gp lab2_list.gp

SUBMISSION_FILES = $(SOURCE_FILES) Makefile $(CSVS) $(PNGS) $(SCRIPTS) README

default: build

build: lab2_add lab2_list

lab2_add: lab2_add.c
	$(CC) lab2_add.c -lpthread -o $(EXECUTABLE_ADD)

lab2_list: lab2_list.c SortedList.c SortedList.h
	$(CC) lab2_list.c SortedList.c -lpthread -o $(EXECUTABLE_LIST)

tests: build
	chmod +x add_tests.sh list_tests.sh
	-./add_tests.sh
	-./list_tests.sh

graphs: clean build tests
	chmod +x lab2_add.gp lab2_list.gp
	gnuplot ./lab2_add.gp
	gnuplot ./lab2_list.gp

clean:
	rm -f *.o *.tar.gz $(EXECUTABLES) $(CSVS) $(PNGS)

dist: clean build tests graphs
	tar -czf lab2a-$(UID).tar.gz $(SUBMISSION_FILES)

.PHONY: default build tests graphs clean dist
