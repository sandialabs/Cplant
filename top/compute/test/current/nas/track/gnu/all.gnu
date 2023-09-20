##
##  Display MFlops/processor as a function of number of nodes
##

set terminal x11
set title  "NPB 2, All Class A benchmarks, Cplant Alpha/Linux cluster"
set xlabel "Number of nodes"
set ylabel "Mflops/processor"
##set key 200,200
plot [0:40] [0:100]\
    "../plots/ep.A.plot" using 1:4 title "EP class A" with linespoints, \
    "../plots/cg.A.plot" using 1:4 title "CG class A" with linespoints, \
    "../plots/mg.A.plot" using 1:4 title "MG class A" with linespoints, \
    "../plots/ft.A.plot" using 1:4 title "FT class A" with linespoints, \
    "../plots/is.A.plot" using 1:4 title "IS class A" with linespoints, \
    "../plots/lu.A.plot" using 1:4 title "LU class A" with linespoints, \
    "../plots/sp.A.plot" using 1:4 title "SP class A" with linespoints, \
    "../plots/bt.A.plot" using 1:4 title "BT class A" with linespoints
pause -1 "Hit return to continue"
set output "all.ps"
set terminal postscript landscape monochrome 16
plot [0:40] [0:100]\
    "../plots/ep.A.plot" using 1:4 title "EP class A" with linespoints, \
    "../plots/cg.A.plot" using 1:4 title "CG class A" with linespoints, \
    "../plots/mg.A.plot" using 1:4 title "MG class A" with linespoints, \
    "../plots/ft.A.plot" using 1:4 title "FT class A" with linespoints, \
    "../plots/is.A.plot" using 1:4 title "IS class A" with linespoints, \
    "../plots/lu.A.plot" using 1:4 title "LU class A" with linespoints, \
    "../plots/sp.A.plot" using 1:4 title "SP class A" with linespoints, \
    "../plots/bt.A.plot" using 1:4 title "BT class A" with linespoints
set nologscale xy
set terminal x11
