#! /usr/bin/perl

# smp_check.pl -- see if the indicated file
# has "define CONFIG_SMP 1"
#
# usage: % smp_check.pl filename
#
$infile = $ARGV[0];

if ( ! open(INCL, "$infile") ) {
  print("smp_check.pl: cant open $infile\n");
  exit -1;
}

while ( <INCL> ) {
  if ( /define CONFIG_SMP 1/ ) {
    print("yes");
    exit 0;
  }
}
close(INCL);
print("no");
