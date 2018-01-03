# NAME: Brandon Hua
# EMAIL: huabrandon0@gmail.com
# ID: 804595738
# Makefile

CC = gcc
EXECUTABLE = lab1a
UID = 804595738

default: lab1a.c
	$(CC) -o $(EXECUTABLE) lab1a.c

.PHONY: default clean dist

clean:
	rm -f *.o *.tar.gz $(EXECUTABLE)

submission_files = lab1a.c Makefile README

dist:
	tar -czf lab1a-$(UID).tar.gz $(submission_files)
