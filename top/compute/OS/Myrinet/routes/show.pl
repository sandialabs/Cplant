#! /usr/bin/perl

# show.pl

# wrap showroutesiberia so that getting the route out of
# the route file is automated

# usage -- % show.pl src dest
# where src and dst are node ids

if ( ! $ARGV[0] || ! $ARGV[1] ) {
  print("show.pl usage: % show.pl src dst\n");
  print("where src and dst are physical node ids\n");
  exit(-1);
}

if ( $ARGV[0] == "-h" ) {
  print("show.pl usage: % show.pl src dst\n");
  print("where src and dst are physical node ids\n");
  exit(0);
}

$src = $ARGV[0];
$dst = $ARGV[1];


$machine = "`/cplant/sbin/pnid2hname $src siberia `";

$linenum = $dst+1;

$line = `head -$linenum /cplant/routes/$machine.SM-0.siberia | tail -1`;

$route = `/cplant/sbin/showroutesiberia $src $dst $line`;

print("$route\n");
