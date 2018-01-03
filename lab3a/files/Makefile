# NAME: Brandon Hua, Ryan Ohara
# EMAIL: huabrandon0@gmail.com, ryanohara@g.ucla.edu
# ID: 804595738, 404846404

# Makefile for Project 3A

CC=gcc
EXECUTABLE=lab3a
UID=804595738
SUBMISSION_FILES=lab3a.c ext2_fs.h Makefile README

default: lab3a.c ext2_fs.h
	$(CC) lab3a.c -o $(EXECUTABLE)

dist:
	tar -czf lab3a-$(UID).tar.gz $(SUBMISSION_FILES)

clean:
	rm -f $(EXECUTABLE) *.tar.gz

.PHONY: default dist clean
