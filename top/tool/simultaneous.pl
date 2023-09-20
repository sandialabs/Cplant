#!/usr/bin/perl

# This script takes as input a sorted Cplant log file and calculates the
# maximum number of simultaneous users and simultaneous jobs for each day.
# The number of simultaneous users is the number of distinct users whose
# jobs were executing simultaneously at any given moment during the day.
# The number of simultaneous jobs is the number of jobs that were executing
# simultaneously at any given moment during the day, possibly from the same
# user.
#
# This script currently assumes the input file spans a single month.
#

%simul = ();

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

sub time_compare{

    my ( $time1, $time2 ) = @_;
    my ( $hour1, $min1, $sec1   ) = split ':', $time1;
    my ( $hour2, $min2, $sec2   ) = split ':', $time2;
    my $ret = 0;

    if ( ($ret = $hour1 <=> $hour2) != 0 ) {
	return $ret;
    }

    if ( ($ret = $min1  <=> $min2 ) != 0 ) {
	return $ret;
    }

    return ( $sec1 <=> $sec2 );

}

while(<>) {

    @tmp = split;

    ( $startdate, $starttime, $stopdate, $stoptime,
      $elapsed, $nodes, $user, $executable ) = @tmp; 

    push @log, [ @tmp ]; 

}

print STDERR "Processing .";

$percent = 0.1;

for ( $i=0; $i<$#log+1; $i++ ) {

    if ( ($i / ($#log+1) ) > $percent ) {

	print STDERR " .";
	$percent += 0.1;

    }

    $sdate = $log[$i][0];

    $stime = $log[$i][1];

    $edate = $log[$i][2];

    $etime = $log[$i][3];

    if ( ! defined( $simul{$sdate} ) ) {

	$simul{$sdate}{jobs}  = 1;
	$simul{$sdate}{users} = 1;
	$simul{$sdate}{stime} = $stime;
	$simul{$sdate}{edate} = $edate;
	$simul{$sdate}{etime} = $etime;
	
    }

    $endtime = $etime;
    
    $jobs    = 1;

    %users   = ();

    $users{$log[$i][6]} = 0;

    for ( $j=$i+1; $j<$#log+1; $j++ ) {

	$sdate2 = $log[$j][0];
	
	$stime2 = $log[$j][1];

	$edate2 = $log[$j][2];

	$etime2 = $log[$j][3];

	if ( date_compare( $sdate2, $edate ) == 0 ) {

	    if ( time_compare( $stime2, $endtime ) <= 0 ) {

		$jobs++;

		$users{$log[$j][6]} = 0;

		if ( time_compare($etime2, $endtime) < 0 ) {

		    $endtime = $etime2;

		}

	    }
	    else {
		break;
	    }

	}
	else {
	    break;
	}
	
    }

    $num_users = keys %users;

    if ( $simul{$sdate}{users} < $num_users ) {
	$simul{$sdate}{users} = $num_users;
	$simul{$sdate}{stime} = $stime;
	$simul{$sdate}{edate} = $edate;
	$simul{$sdate}{etime} = $etime;

    }

    if ( $simul{$sdate}{jobs} < $jobs ) {
	$simul{$sdate}{jobs}  = $jobs;
	$simul{$sdate}{stime} = $stime;
	$simul{$sdate}{edate} = $edate;
	$simul{$sdate}{etime} = $etime;
    }


}


print STDERR " Done\n";

format OUTPUT_TOP =

         Cplant Usage Statistics

  Date    Simultaneous Users  Simultaneous Jobs
--------  ------------------  -----------------
.

format OUTPUT =
@>>>>>>>  @>>>>>>>>>        @>>>>>>>>>>>
$sorted[$i],$users,$jobs
.

$~ = "OUTPUT";

sub date_sort {
    my ( $date1 ) = $a;
    my ( $date2 ) = $b;
    return date_compare( $date1, $date2 );
}

@sorted  = sort date_sort ( keys %simul );

for ( $i=0; $i<$#sorted+1; $i++ ) {

    $users = $simul{$sorted[$i]}{users};
    $jobs  = $simul{$sorted[$i]}{jobs};

    write;

}

$~ = "STDOUT";

print "\n";





