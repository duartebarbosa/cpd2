ist1314-cpd
===========

How to test:
 - extract the big examples provided in http://algos.inesc-id.pt/~jcm/cpd into instances/
 - run "bash test-me.sh"

Word of caution: the times are provided using /usr/bin/time.
They are NOT near as acurate as the ones when compiling with -DGETTIME=1 (in makefile)
Why did I did this: we are pipping the output of the execution and then comparing it with
the sample output. Obviously, it will always fail because of the extra "time: xxx" line.

Parameters:
$ a.out world_10.in 20 5 30 5
$ a.out world_10.in 20 50 30 1000000
$ a.out world_100.in 20 50 30 100000
$ a.out world_1000.in 20 50 30 10000
$ a.out world_10000.in 20 50 30 100