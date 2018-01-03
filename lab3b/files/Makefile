# NAME: Brandon Hua
# EMAIL: huabrandon0@gmail.com
# ID: 804595738

# Makefile for Project 3B

UID=804595738

default: lab3b.py lab3b
	chmod +x lab3b.py
	chmod +x lab3b

dist: lab3b.py Makefile README lab3b
	tar -czf lab3b-$(UID).tar.gz lab3b.py Makefile README lab3b

clean:
	rm -f *.tar.gz

.PHONY: default dist clean
