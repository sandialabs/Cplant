# config/suite.def
# This file is used to build several benchmarks with a single command. 
# Typing "make suite" in the main directory will build all the benchmarks
# specified in this file. 
# Each line of this file contains a benchmark name, class, and number
# of nodes. The name is one of "cg", "is", "ep", mg", "ft", "sp", "bt", 
# and "lu". 
# The class is one of "S", "W", "A", "B", and "C". 
# The number of nodes must be a legal number for a particular
# benchmark. The utility which parses this file is primitive, so
# formatting is inflexible. Separate name/class/number by tabs. 
# Comments start with "#" as the first character on a line. 
# No blank lines. 
ep      B       2
ep      B       4
ep      B       8
ep       B      16
ep       B      32
ep       B      64
mg      B       2
mg      B       4
mg      B       8
mg      B       16
mg      B       32
mg      B       64
cg      B       2
cg      B       4
cg      B       8
cg      B       16
cg      B       32
cg      B       64
ft      B       2
ft      B       4
ft      B       8
ft      B       16
ft      B       32
ft      B       64
is      B       2
is      B       4
is      B       8
is      B       16
is      B       32
is      B       64
lu       B      2
lu       B      4
lu       B      8
lu       B      16
lu       B      32
lu       B      64
sp       B      4
sp       B      9
sp       B      16
sp       B      25
sp       B      36
sp       B      49
bt       B      4
bt       B      9
bt       B      16
bt       B      25
bt       B      36
bt       B      49
