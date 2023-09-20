##
##  Display MFlops/processor as a function of number of nodes
##

set terminal x11
set title  "NPB 2, CG Class A benchmark, Cplant Alpha/Linux cluster"
set xlabel "Number of nodes"
set ylabel "Mflops/processor"
#set logscale x
#set yrange [10:1000]
set key 200,200
plot \
    "../plots/cg.A.plot" using 1:4 title "CG class A" with linespoints
pause -1 "Hit return to continue"
set output "cg.ps"
set terminal postscript landscape monochrome 16
plot \
    "../plots/cg.A.plot" using 1:4 title "CG class A" with linespoints
set nologscale xy
set terminal x11
