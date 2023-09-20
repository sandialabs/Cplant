#! /usr/bin/perl

# path_prefix.pl -- replace value of CPLANT_PATH
# from top/include/config/cplant.h with
# string "CPLANT_PATH" in the indicated files
#
# usage: % path_prefix.pl includefile infile outfile
#
# where includefile is the path to a header file with the
# string "define CPLANT_PATH" followd by a VALUE in quotes
# (i.e. this file #defines CPLANT_PATH AS A STRING)
#
# infile is the input file with occurrences of "CPLANT_PATH"
#
# outfile will be created -- it will be the same as infile
# but w/ the "CPLANT_PATH" occurrences replaced by VALUE...

$incl    = $ARGV[0];
$infile  = $ARGV[1];
$outfile = $ARGV[2];

if ( ! open(INCL, "$incl") ) {
  print("path_prefix.pl: cant open $incl\n");
  exit -1;
}

# get the value of CPLANT_PATH

while ( <INCL> ) {

  if ( /define CPLANT_PATH/ ) {
    @words = split();
    $val = $words[2];
    chop($val);
    $val = substr($val,1);
  }

}

# do replacement

if ( ! open(INFILE, "$infile") ) {
  print("path_prefix.pl: cant open $infile for reading\n");
  exit -1;
}
if ( ! open(OUTFILE, ">$outfile") ) {
  print("path_prefix.pl: cant open $outfile for writing\n");
  exit -1;
}

while ( <INFILE> ) {
  s/CPLANT_PATH/$val/g;
  print OUTFILE;
}

close(INFILE);
close(OUTFILE);
