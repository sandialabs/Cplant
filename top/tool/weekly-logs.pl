#!/usr/local/bin/perl -w
#
#  There's a userlog file on every service node.  We want
#  want to combine them all into one file for each week.
#
#  This script takes userlog files and:
#
#    o  separate jobs that run over a date boundary 
#       into separate daily entries
#    o  writes a separate file for each work week  
#       ("userlog.YYYYMMDD.YYYYMMDD")
#
#  To use this script:
#
#  1. Type in below the work weeks for which you want log files.
#
#  2. Script looks for files called "userlog" one directory level below it,
#     so something like "rsh c-15.SU-n cat /tmp/userlog > n/userlog"
#     for every service node and then run this script.
#
#
#  PLEASE don't laugh.  I'm not a perl programmer.
#

#
# weekstart contains first day of each week
# weekend contains last day of each week
#
@weekstart = qw(02/07/2000 02/14/2000 02/21/2000 02/28/2000 03/06/2000 03/13/2000 03/20/2000 03/27/2000 04/03/2000 04/10/2000 04/17/2000 04/24/2000 05/01/2000 05/08/2000 05/15/2000);
@weekend   = qw(02/13/2000 02/20/2000 02/27/2000 03/05/2000 03/12/2000 03/19/2000 03/26/2000 04/02/2000 04/09/2000 04/16/2000 04/23/2000 04/30/2000 05/07/2000 05/14/2000 05/21/2000);

$nweeks = @weekstart;

for ($i = 0 ; $i < $nweeks ; $i++){

    @fromdate = split '/', $weekstart[$i];
    @todate   = split '/', $weekend[$i];

    $sortfrom[$i] = $fromdate[2].$fromdate[0].$fromdate[1];
    $sortto[$i]   = $todate[2].$todate[0].$todate[1];

    $outfile[$i] = "userlog.all.$sortfrom[$i].$sortto[$i]";

    $nlines[$i] = 0;

    $cmd = "rm -f $outfile[$i]";
    system($cmd);
}

foreach $logFile (`ls */userlog`){

  chop($logFile);

  print "processing $logFile\n";

  open(LOGF, $logFile) or die "can't open $logFile: $!\n";

# if entries span more than one day, divide them into one
# entry for each day

  $tmpfile = $logFile.".expanded" ;

  $cmd = "expand-jobs.pl < $logFile > $tmpfile\n ";

  system ( $cmd );

  close(LOGF);

  open(LOGF, $tmpfile) or die "can't open $tmpfile: $!\n";

# for each entry, determine which week the entry falls in and
# write it out to the file for that week

  $lasti = 0;

  while ($line = <LOGF>){

     @fields = split ' ', $line;
     ($date, @fields) = @fields;

     ($mo,$dy,$yr) = split '/', $date;

     $sortdate = $yr.$mo.$dy;

     $which = -1;

     if ( ($sortdate >= $sortfrom[0]) && ($sortdate <= $sortto[$nweeks-1]) ){

         if (( $sortdate >= $sortfrom[$lasti] ) && ($sortdate <= $sortto[$lasti])){
             $which = $lasti;
         }
         else{
             for ($i=$nweeks-1; $i>=0 ; $i--){
                 if (( $sortdate  >= $sortfrom[$i] ) && ($sortdate <= $sortto[$i])){
                     $which = $i;
                     $lasti = $i;
                     last;
                 }
             }
         }
     }

     if ($which >= 0){
         $lines[$which][$nlines[$which]++] =  $line;
     }
  }
  close(LOGF);
  for ($i = 0; $i < $nweeks; $i++){

      if ($nlines[$i] > 0){
          open(OUTF, ">>$outfile[$i]");

          for ($j = 0; $j < $nlines[$i]; $j++){

              print OUTF $lines[$i][$j];
          }
          print "    $nlines[$i] to $outfile[$i]\n";
          close(OUTF);
          $nlines[$i] = 0;
      }
  }
}

