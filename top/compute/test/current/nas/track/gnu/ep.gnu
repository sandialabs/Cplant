##
##  Display MFlops/processor as a function of number of nodes
##

set terminal x11
set title  "NPB 2, EP Class A benchmark, Cplant Alpha/Linux cluster"
set xlabel "Number of nodes"
set ylabel "Mflops/processor"
#set logscale x
#set yrange [10:1000]
set key 200,200
plot \
    "../plots/ep.A.plot" using 1:4 title "EP class A" with linespoints
pause -1 "Hit return to continue"
set output "ep.ps"
set terminal postscript landscape monochrome 16
plot \
    "../plots/ep.A.plot" using 1:4 title "EP class A" with linespoints
set nologscale xy
set terminal x11
