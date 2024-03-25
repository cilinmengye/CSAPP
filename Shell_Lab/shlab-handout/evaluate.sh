#!/bin/bash

make test01 > myAnsfile.txt
make test02 >> myAnsfile.txt
make test03 >> myAnsfile.txt
make test04 >> myAnsfile.txt
make test05 >> myAnsfile.txt
make test06 >> myAnsfile.txt
make test07 >> myAnsfile.txt
make test08 >> myAnsfile.txt
make test09 >> myAnsfile.txt
make test10 >> myAnsfile.txt
make test11 >> myAnsfile.txt
make test12 >> myAnsfile.txt
make test13 >> myAnsfile.txt
make test14 >> myAnsfile.txt
make test15 >> myAnsfile.txt

make rtest01 > rAnsfile.txt
make rtest02 >> rAnsfile.txt
make rtest03 >> rAnsfile.txt
make rtest04 >> rAnsfile.txt
make rtest05 >> rAnsfile.txt
make rtest06 >> rAnsfile.txt
make rtest07 >> rAnsfile.txt
make rtest08 >> rAnsfile.txt
make rtest09 >> rAnsfile.txt
make rtest10 >> rAnsfile.txt
make rtest11 >> rAnsfile.txt
make rtest12 >> rAnsfile.txt
make rtest13 >> rAnsfile.txt
make rtest14 >> rAnsfile.txt
make rtest15 >> rAnsfile.txt

sed -i 's/([^)]*)/1000/g' myAnsfile.txt
sed -i 's/([^)]*)/1000/g' rAnsfile.txt

diff myAnsfile.txt rAnsfile.txt > ans.txt

