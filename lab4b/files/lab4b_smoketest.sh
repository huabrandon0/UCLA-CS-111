#!/bin/bash

PGM="lab4b"
rm -f test_logfile

./$PGM --period=1 --scale=F --log=test_logfile <<-EOF
SCALE=C
SCALE=F
PERIOD=2
START
STOP
OFF
EOF

if [ $? -ne 0 ]
then
	echo "Not all options and commands are supported"
	rm -f test_logfile
	exit 1
fi

if [ ! -s test_logfile ]
then
	echo "Did not create a log file"
	rm -f test_logfile
	exit 1
fi

egrep 'SCALE=C' test_logfile> /dev/null
if [ $? -eq 1 ]
then
	echo "Command SCALE=C not logged"
	rm -f test_logfile
	exit 1
fi

egrep 'SCALE=F' test_logfile> /dev/null
if [ $? -eq 1 ]
then
	echo "Command SCALE=F not logged"
	rm -f test_logfile
	exit 1
fi

egrep 'PERIOD=2' test_logfile> /dev/null
if [ $? -eq 1 ]
then
	echo "Command PERIOD=# not logged"
	rm -f test_logfile
	exit 1
fi

egrep 'START' test_logfile> /dev/null
if [ $? -eq 1 ]
then
	echo "Command START not logged"
	rm -f test_logfile
	exit 1
fi

egrep 'STOP' test_logfile> /dev/null
if [ $? -eq 1 ]
then
	echo "Command STOP not logged"
	rm -f test_logfile
	exit 1
fi

egrep 'OFF' test_logfile> /dev/null
if [ $? -eq 1 ]
then
	echo "Command OFF not logged"
	rm -f test_logfile
	exit 1
fi

egrep '[0-9][0-9]:[0-9][0-9]:[0-9][0-9] SHUTDOWN' test_logfile> /dev/null
if [ $? -eq 1 ]
then
	echo "SHUTDOWN not logged"
	rm -f test_logfile
	exit 1
fi

./$PGM --bogus
if [ $? -eq 0 ]
then
	echo "Bad option not detected"
	rm -f test_logfile
	exit 1
fi

rm -f test_logfile

exit 0