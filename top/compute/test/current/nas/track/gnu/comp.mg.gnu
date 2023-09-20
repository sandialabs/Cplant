##
##  Display MFlops/processor as a function of number of nodes
##

set terminal x11
set title  "NPB 2, MG Class A benchmark, comparison"
set xlabel "Number of nodes"
set ylabel "Mflops/processor"
##set key 200,200
plot \
    "../plots/mg.A.plot" using 1:4 title "Cplant Alpha cluster" with linespoints, \
    "../nasClusterResults/pgon.mg.A.plot" using 1:4 title "WPAFB Paragon" with linespoints, \
    "../nasClusterResults/now.mg.A.plot" using 1:4 title "NOW Sparc cluster" with linespoints, \
    "../nasClusterResults/nas.mg.A.plot" using 1:4 title "NAS P6 cluster" with linespoints
pause -1 "Hit return to continue"
set output "mg.comp.ps"
set terminal postscript landscape monochrome 16
plot \
    "../plots/mg.A.plot" using 1:4 title "Cplant Alpha cluster" with linespoints, \
    "../nasClusterResults/pgon.mg.A.plot" using 1:4 title "WPAFB Paragon" with linespoints, \
    "../nasClusterResults/now.mg.A.plot" using 1:4 title "NOW sparc cluster" with linespoints, \
    "../nasClusterResults/nas.mg.A.plot" using 1:4 title "NAS P6 cluster" with linespoints
set nologscale xy
set terminal x11
