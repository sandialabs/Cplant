#!/usr/bin/perl

# This script takes a Cplant log file as input from stdin as expands jobs
# that span multiple days into multiple entries.  The output of this script
# is NOT sorted. 

$hour0  = "00:00:00";
$hour24 = "23:59:59";
$seconds24 = (24 * 60 * 60);

sub date_compare{
    my ( $date1, $date2 ) = @_;

    my ( $month1, $day1, $year1 ) = split '/', $date1;
    my ( $month2, $day2, $year2 ) = split '/', $date2;
    my $ret = 0;

    if ( ($ret = $year1  <=> $year2)  != 0 ) {
	return $ret;
    }

    if ( ($ret = $month1 <=> $month2) != 0 ) {
	return $ret;
    }

    return ( $day1 <=> $day2 );

}

while(<>) {

    @tmp = split;

    ( $startdate, $starttime, $stopdate, $stoptime,
      $elapsed, $nodes, $user, $executable ) = @tmp; 


    if ( date_compare($startdate,$stopdate) < 0 ) {

	( $firstmonth, $firstday, $firstyear) = split /\//, $startdate;
	( $lastmonth,  $lastday,  $lastyear)  = split /\//, $stopdate;

     # elapsed time for first day ##################

     ($hr, $min, $sec) = split ":", $starttime;

     $elapsedSeconds = $seconds24 - ($sec + ($min*60) + ($hr*3600)); 

     $hr = int ($elapsedSeconds / 3600);
     $min = int (($elapsedSeconds % 3600) / 60);
     $sec = $elapsedSeconds % 60;    

     $elapsed = "$hr:$min:$sec";
     ###############################################

	print "$startdate $starttime $startdate $hour24 $elapsed $nodes $user $executable\n";

    

	for ( $i=$firstday+1; $i<$lastday; $i++ ) {

	    $startdate = "$firstmonth/$i/$firstyear";

        $elapsed = "24:00:00";

	    print "$startdate $hour0 $startdate $hour24 $elapsed $nodes $user $executable\n";
	    
	}
     # elapsed time for last day ##################

     ($hr, $min, $sec) = split ":", $stoptime;

     $elapsedSeconds = $sec + ($min*60) + ($hr*3600); 

     $hr = int ($elapsedSeconds / 3600);
     $min = int (($elapsedSeconds % 3600) / 60);
     $sec = $elapsedSeconds % 60;    

     $elapsed = "$hr:$min:$sec";
     ###############################################

	print "$stopdate $hour0 $stopdate $stoptime $elapsed $nodes $user $executable\n";

    }
    else {

        print "$startdate $starttime $stopdate $stoptime $elapsed $nodes $user $executable\n";

    }

}

