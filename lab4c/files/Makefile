# NAME: Brandon Hua
# EMAIL: huabrandon0@gmail.com
# ID: 804595738

# Makefile for Project 4C

CC=gcc
CFLAGS=-lm -lmraa
SSLFLAGS=-lssl -lcrypto
UID=804595738
EXEC_TCP=lab4c_tcp
EXEC_TLS=lab4c_tls
SUBMISSION_FILES=lab4c_tcp.c lab4c_tls.c Makefile README

default: lab4c_tcp.c lab4c_tls.c
	$(CC) lab4c_tcp.c -o $(EXEC_TCP) $(CFLAGS)
	$(CC) lab4c_tls.c -o $(EXEC_TLS) $(CFLAGS) $(SSLFLAGS)
	
clean:
	rm -f $(EXEC_TCP) $(EXEC_TLS) *.tar.gz
	
dist:
	tar -czf lab4c-$(UID).tar.gz $(SUBMISSION_FILES)

.PHONY: default clean dist