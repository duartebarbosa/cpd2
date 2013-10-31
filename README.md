ist1314-cpd
===========

How to test:
 - extract the big examples provided in http://algos.inesc-id.pt/~jcm/cpd into instances/
 - run "bash test-me.sh"

Word of caution: the times are provided using /usr/bin/time.
They are NOT near as acurate as the ones when compiling with -DGETTIME=1 (in makefile)
Why: we are pipping the output of the execution and then comparing it with the
sample output. Obviously, it would always fail because of the extra "time: xxx" line.
