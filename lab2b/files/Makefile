# NAME: Brandon Hua
# EMAIL: huabrandon0@gmail.com
# ID: 804595738
# Makefile for Project 2B

CC = gcc
CFLAGS = -g
UID = 804595738
EXECUTABLE = lab2_list
CSV = lab2b_list.csv
PNGS = lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png
SUBMISSION_FILES = SortedList.h SortedList.c lab2_list.c \
Makefile $(CSV) profile.out $(PNGS) lab2b_list.gp README

export PATH := /usr/local/cs/bin:$(PATH)

default: lab2_list.c SortedList.h SortedList.c
	gcc $(CFLAGS) lab2_list.c SortedList.c -lpthread -o $(EXECUTABLE)

tests: clean default
	chmod +x list_exerciser.sh
	-./list_exerciser.sh
	
profile: clean default
	-rm -f ./raw.gperf
	LD_PRELOAD=/u/cs/ugrad/brandonh/lib/libprofiler.so.0 \
	CPUPROFILE=raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.out
	pprof --list=test_run ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf
	
graphs: clean default tests
	chmod +x lab2b_list.gp
	gnuplot ./lab2b_list.gp
	
dist: clean default tests profile graphs
	tar -czf lab2b-$(UID).tar.gz $(SUBMISSION_FILES)

clean:
	rm -f $(EXECUTABLE) *.o $(CSV) $(PNGS) *.tar.gz profile.out

.PHONY: default tests profile graphs dist clean