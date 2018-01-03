#!/bin/sh

rm -f lab2b_list.csv

#lab2b_1.png Throughput (throughput vs number of threads)
./lab2_list --sync=m --iterations=1000 --threads=1 >> lab2b_list.csv
./lab2_list --sync=m --iterations=1000 --threads=2 >> lab2b_list.csv
./lab2_list --sync=m --iterations=1000 --threads=4 >> lab2b_list.csv
./lab2_list --sync=m --iterations=1000 --threads=8 >> lab2b_list.csv
./lab2_list --sync=m --iterations=1000 --threads=12 >> lab2b_list.csv
./lab2_list --sync=m --iterations=1000 --threads=16 >> lab2b_list.csv
./lab2_list --sync=m --iterations=1000 --threads=24 >> lab2b_list.csv

./lab2_list --sync=s --iterations=1000 --threads=1 >> lab2b_list.csv
./lab2_list --sync=s --iterations=1000 --threads=2 >> lab2b_list.csv
./lab2_list --sync=s --iterations=1000 --threads=4 >> lab2b_list.csv
./lab2_list --sync=s --iterations=1000 --threads=8 >> lab2b_list.csv
./lab2_list --sync=s --iterations=1000 --threads=12 >> lab2b_list.csv
./lab2_list --sync=s --iterations=1000 --threads=16 >> lab2b_list.csv
./lab2_list --sync=s --iterations=1000 --threads=24 >> lab2b_list.csv

##lab2b_2.png Mutex Wait Time (wait-for-lock time, avg time/op vs number of threads)
#./lab2_list --sync=m --iterations=1000 --threads=1 >> lab2b_list.csv
#./lab2_list --sync=m --iterations=1000 --threads=2 >> lab2b_list.csv
#./lab2_list --sync=m --iterations=1000 --threads=4 >> lab2b_list.csv
#./lab2_list --sync=m --iterations=1000 --threads=8 >> lab2b_list.csv
#./lab2_list --sync=m --iterations=1000 --threads=16 >> lab2b_list.csv
#./lab2_list --sync=m --iterations=1000 --threads=24 >> lab2b_list.csv

#data already gotten in lab2b_1.png Throughput tests

#lab2b_3.png Partitioned List Tests
#no sync
./lab2_list --yield=id --lists=4 --threads=1 --iterations=1 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=1 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=1 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=1 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=1 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=1 --iterations=2 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=2 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=2 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=2 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=2 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=1 --iterations=4 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=4 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=4 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=4 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=4 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=1 --iterations=8 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=8 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=8 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=8 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=8 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=1 --iterations=16 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=16 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=16 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=16 >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=16 >> lab2b_list.csv

#sync=s,m
./lab2_list --yield=id --lists=4 --threads=1 --iterations=10 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=10 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=10 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=10 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=10 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=1 --iterations=20 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=20 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=20 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=20 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=20 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=1 --iterations=40 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=40 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=40 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=40 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=40 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=1 --iterations=80 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=80 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=80 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=80 --sync=s >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=80 --sync=s >> lab2b_list.csv

./lab2_list --yield=id --lists=4 --threads=1 --iterations=10 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=10 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=10 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=10 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=10 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=1 --iterations=20 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=20 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=20 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=20 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=20 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=1 --iterations=40 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=40 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=40 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=40 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=40 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=1 --iterations=80 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=4 --iterations=80 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=8 --iterations=80 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=12 --iterations=80 --sync=m >> lab2b_list.csv
./lab2_list --yield=id --lists=4 --threads=16 --iterations=80 --sync=m >> lab2b_list.csv

#lab2b_4.png Testing partitioned list performance with mutex
#./lab2_list --iterations=1000 --threads=1 --lists=1 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=1 --lists=4 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=1 --lists=8 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=1 --lists=16 --sync=m >> lab2b_list.csv
#./lab2_list --iterations=1000 --threads=2 --lists=1 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=2 --lists=4 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=2 --lists=8 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=2 --lists=16 --sync=m >> lab2b_list.csv
#./lab2_list --iterations=1000 --threads=4 --lists=1 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=4 --lists=4 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=4 --lists=8 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=4 --lists=16 --sync=m >> lab2b_list.csv
#./lab2_list --iterations=1000 --threads=8 --lists=1 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=8 --lists=4 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=8 --lists=8 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=8 --lists=16 --sync=m >> lab2b_list.csv
#./lab2_list --iterations=1000 --threads=12 --lists=1 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=12 --lists=4 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=12 --lists=8 --sync=m >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=12 --lists=16 --sync=m >> lab2b_list.csv

#lab2b_5.png Testing partitioned list performance with spinlock
#./lab2_list --iterations=1000 --threads=1 --lists=1 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=1 --lists=4 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=1 --lists=8 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=1 --lists=16 --sync=s >> lab2b_list.csv
#./lab2_list --iterations=1000 --threads=2 --lists=1 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=2 --lists=4 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=2 --lists=8 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=2 --lists=16 --sync=s >> lab2b_list.csv
#./lab2_list --iterations=1000 --threads=4 --lists=1 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=4 --lists=4 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=4 --lists=8 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=4 --lists=16 --sync=s >> lab2b_list.csv
#./lab2_list --iterations=1000 --threads=8 --lists=1 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=8 --lists=4 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=8 --lists=8 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=8 --lists=16 --sync=s >> lab2b_list.csv
#./lab2_list --iterations=1000 --threads=12 --lists=1 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=12 --lists=4 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=12 --lists=8 --sync=s >> lab2b_list.csv
./lab2_list --iterations=1000 --threads=12 --lists=16 --sync=s >> lab2b_list.csv

