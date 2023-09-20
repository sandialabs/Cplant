##
##  Display MFlops/processor as a function of number of nodes
##

set terminal x11
set title  "NPB 2, CG Class A benchmark, comparison"
set xlabel "Number of nodes"
set ylabel "Mflops/processor"
##set key 200,200
plot \
    "../plots/cg.A.plot" using 1:4 title "Cplant Alpha cluster" with linespoints, \
    "../nasClusterResults/pgon.cg.A.plot" using 1:4 title "WPAFB Paragon" with linespoints
pause -1 "Hit return to continue"
set output "cg.comp.ps"
set terminal postscript landscape monochrome 16
plot \
    "../plots/cg.A.plot" using 1:4 title "Cplant Alpha cluster" with linespoints, \
    "../nasClusterResults/pgon.cg.A.plot" using 1:4 title "WPAFB Paragon" with linespoints
set nologscale xy
set terminal x11
