# NAME: Brandon Hua
# EMAIL: huabrandon0@gmail.com
# ID: 804595738
# Makefile

CC = gcc
CFLAGS = -g
EXECUTABLE = lab0
UID = 804595738

default: lab0.c
	$(CC) $(CFLAGS) -o $(EXECUTABLE) lab0.c

TESTS = testEC0_1 testEC0_2 testEC0_3 testEC1 testEC2 testEC3 testEC4 testEC139
check: default $(TESTS)
	echo "All tests passed."

testEC0_1:
	echo "testing" | ./$(EXECUTABLE) > o_file; \
	if [ $$? -ne 0 ]; then \
	  echo "make check failed: $@"; exit 1; \
	fi ;

testEC0_2:
	echo "testing" > i_file; \
	./$(EXECUTABLE) --input=i_file --output=o_file; \
	if [ $$? -ne 0 ]; then \
	  echo "make check failed: $@"; exit 1; \
	fi; \
	rm -f i_file o_file

testEC0_3:
	echo "testing" > i_file; \
	./$(EXECUTABLE) --input=i_file --output=o_file; \
	cmp i_file o_file; \
	if [ $$? -ne 0 ]; then \
	  echo "make check failed: $@"; exit 1; \
	fi; \
	rm -f i_file o_file

testEC1:
	./$(EXECUTABLE) --fake_option >/dev/null 2>&1; \
	if [ $$? -ne 1 ]; then \
	  echo "make check failed: $@"; exit 1; \
	fi

testEC2:
	touch i_file; \
	chmod u-r i_file; \
	./$(EXECUTABLE) --input=i_file >/dev/null 2>&1; \
	if [ $$? -ne 2 ]; then \
	  echo "make check failed: testEC2"; exit 1; \
	fi; \
	rm -f i_file

testEC3:
	touch o_file; \
	chmod u-w o_file; \
	./$(EXECUTABLE) --output=o_file >/dev/null 2>&1; \
	if [ $$? -ne 3 ]; then \
	  echo "make check failed: $@"; exit; \
	fi; \
	rm -f o_file

testEC4:
	./$(EXECUTABLE) --segfault --catch >/dev/null 2>&1; \
	if [ $$? -ne 4 ]; then \
	  echo "make check failed: $@"; exit; \
	fi

testEC139:
	./$(EXECUTABLE) --segfault >/dev/null 2>&1; \
	if [ $$? -ne 139 ]; then \
	  echo "make check failed: $@"; exit; \
	fi

.PHONY: default clean check $(TESTS) dist

clean:
	rm -f *.o *.tar.gz $(EXECUTABLE)

submission_files = lab0.c Makefile backtrace.png breakpoint.png README

dist:
	tar -czf lab0-$(UID).tar.gz $(submission_files)
