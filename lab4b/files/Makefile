# NAME: Brandon Hua
# EMAIL: huabrandon0@gmail.com
# ID: 804595738
# Makefile for Project 4B

CC = gcc
CFLAGS = -lmraa -lm
UID = 804595738
EXECUTABLE = lab4b
SUBMISSION_FILES = lab4b.c Makefile lab4b_smoketest.sh README

default: lab4b.c
	$(CC) lab4b.c $(CFLAGS) -o $(EXECUTABLE)
	
check: default
	chmod u+x lab4b_smoketest.sh
	-./lab4b_smoketest.sh; \
	if [ $$? -eq 0 ]; \
	then \
		echo "All tests passed"; \
	else \
		echo "A test failed"; \
	fi
	
	
clean:
	rm -f $(EXECUTABLE) *.tar.gz

dist:
	tar -czf lab4b-$(UID).tar.gz $(SUBMISSION_FILES)

.PHONY: default check clean dist