#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2b_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2b-1.png ... throughput vs number of threads for mutex and spin-lock
#                   synchronized list operations
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#

# general plot parameters
set terminal png
set datafile separator ","

# 1. throughput vs number of threads for mutex and spin-lock
set title "Lab2B-1: Throughput vs Number of Threads"
set xlabel "Number of Threads"
set ylabel "Throughput (ops/s)"
set logscale x 2
set logscale y 10
set output 'lab2b_1.png'

# grep out any-threaded, 1000-iteration, synced, non-yield results
plot \
	"< grep -E 'list-none-m,(1|2|4|8|12|16|24),1000,1,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title '(adjusted) list w/mutex' with linespoints lc rgb 'blue', \
	"< grep -E 'list-none-s,(1|2|4|8|12|16|24),1000,1,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title '(adjusted) list w/spin-lock' with linespoints lc rgb 'green'

# 2. wait-for-lock time, avg time/op vs number of threads for mutex
set title "Lab2B-2: Average Wait-for-lock Time, Average Time/Op vs Number of Threads"
set xlabel "Number of Threads"
set ylabel "Time (ns)"
set logscale x 2
set logscale y 10
set output 'lab2b_2.png'

# grep out only any-threaded, 1000-iteration, mutex-synced, non-yield results
plot \
	"< grep -E 'list-none-m,(1|2|4|8|16|24),1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'average wait-for-lock time' with linespoints lc rgb 'blue', \
	"< grep -E 'list-none-m,(1|2|4|8|16|24),1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'average time/op' with linespoints lc rgb 'green'

# 3. testing if partitioned list works
set title "Lab2B-3: Threads and Iterations that Run Without Failure"
set xlabel "Threads"
set logscale x 2
set ylabel "Iterations Per Thread"
set logscale y 10
set output 'lab2b_3.png'

# grep out successful runs with yield=id
plot \
	"< grep 'list-id-none' lab2b_list.csv" using ($2):($3) \
	title 'no synchronization' with points lc rgb 'red', \
	"< grep 'list-id-s' lab2b_list.csv" using ($2):($3) \
	title 'spinlock' with points lc rgb 'blue', \
	"< grep 'list-id-m' lab2b_list.csv" using ($2):($3) \
	title 'mutex' with points lc rgb 'green'

# 4. testing performance of partitioned list with mutex
set title "Lab2B-4: Performance of Mutex-synchronized Partitioned List"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (ops/s)"
set logscale y 10
set output 'lab2b_4.png'

# grep out multi-list mutex-synchronized runs
plot \
	"< grep -E 'list-none-m,(1|2|4|8|12),1000,1,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title '1 list' with linespoints lc rgb 'blue', \
	"< grep -E 'list-none-m,(1|2|4|8|12),1000,4,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title '4 lists' with linespoints lc rgb 'green', \
	"< grep -E 'list-none-m,(1|2|4|8|12),1000,8,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title '8 lists' with linespoints lc rgb 'red', \
	"< grep -E 'list-none-m,(1|2|4|8|12),1000,16,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title '16 lists' with linespoints lc rgb 'orange'
	
# 4. testing performance of partitioned list with mutex
set title "Lab2B-5: Performance of Spinlock-synchronized Partitioned List"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput (ops/s)"
set logscale y 10
set output 'lab2b_5.png'

# grep out multi-list spinlock-synchronized runs
plot \
	"< grep -E 'list-none-s,(1|2|4|8|12),1000,1,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title '1 list' with linespoints lc rgb 'blue', \
	"< grep -E 'list-none-s,(1|2|4|8|12),1000,4,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title '4 lists' with linespoints lc rgb 'green', \
	"< grep -E 'list-none-s,(1|2|4|8|12),1000,8,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title '8 lists' with linespoints lc rgb 'red', \
	"< grep -E 'list-none-s,(1|2|4|8|12),1000,16,' lab2b_list.csv" using ($2):(1000000000)/($7) \
	title '16 lists' with linespoints lc rgb 'orange'
