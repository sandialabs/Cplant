#!/usr/local/bin/perl
#
##############################################################
#   Prime time version: We only include jobs with execution
#     time overlapping the hours of 8:00AM to 6:00PM
##############################################################
#
#  There's a userlog file on every service node.  We want
#  want to combine them all into one file for each week.
#
#  This script takes userlog files and:
#
#    o  separate jobs that run over a date boundary 
#       into separate daily entries
#    o  write a separate file for each work week  
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
# weekstart contains Monday of each week
# weekend contains Friday of each week
# holiday lists week days that were holidays, hence not prime time
#
@weekstart = qw(02/07/2000 02/14/2000 02/21/2000 02/28/2000 03/06/2000 03/13/2000 03/20/2000 03/27/2000 04/03/2000 04/10/2000 04/17/2000 04/24/2000 05/01/2000 05/08/2000  05/15/2000);
@weekend   = qw(02/11/2000 02/18/2000 02/25/2000 03/03/2000 03/10/2000 03/17/2000 03/25/2000 03/31/2000 04/07/2000 04/14/2000 04/21/2000 04/28/2000 05/05/2000 05/12/2000 05/19/2000);
@holiday = qw(05/29/2000 07/04/2000 09/04/2000 11/23/2000 11/24/2000 12/25/2000 12/27/2000 12/29/2000);

$primetimeStart="08:00:00";
$primetimeEnd  ="17:59:59";

sub compare_time {
    my ( $time1, $time2 ) = @_;
 
    my ( $hour1, $min1, $sec1 ) = split ':', $time1;
    my ( $hour2, $min2, $sec2 ) = split ':', $time2;

    my $ret = 0;

    if ( ($ret = $hour1  <=> $hour2)  != 0 ) {
    return $ret;
    }
 
    if ( ($ret = $min1 <=> $min2) != 0 ) {
    return $ret;
    }
 
    return ( $sec1 <=> $sec2 );

}

#
#  did the job overlap prime time?
#
#  args: jobdate jobstarttime jobendtime
#
sub prime_time {
    my ( $date, $fromtime, $totime ) = @_;
 
    if ( compare_time($fromtime, $primetimeEnd) > 0){
        return 0;
    }
    if ( compare_time($totime, $primetimeStart) < 0){
        return 0;
    }

    foreach $dayoff (@holiday){

        if ($dayoff eq $date){
            return 0;
        }    
         
    }
    return 1;
}



$nweeks = @weekstart;

for ($i = 0 ; $i < $nweeks ; $i++){

    @fromdate = split '/', $weekstart[$i];
    @todate   = split '/', $weekend[$i];

    $sortfrom[$i] = $fromdate[2].$fromdate[0].$fromdate[1];
    $sortto[$i]   = $todate[2].$todate[0].$todate[1];

    $outfile[$i] = "userlog.prime.$sortfrom[$i].$sortto[$i]";

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
# if and only if the job overlapped primetime

  $lasti = 0;

  while ($line = <LOGF>){

     @fields = split ' ', $line;
     ($date, $starttime, $enddate, $endtime, @fields) = @fields;

     if ( prime_time($date, $starttime, $endtime) == 0){
         next; 
     }

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

