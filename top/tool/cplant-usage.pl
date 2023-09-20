#!/Net/local/bin/perl

# Perl script to compile usage data from Cplant user log files
# Here's what it figures out:
# - number of users per day
# - number of jobs per day
# - total time of jobs per day
# - average number of users
# - average number of jobs
# - average length of a job
# - average job size

%userstats = ();
%timestats = ();
%nodestats = ();

while(<>) {

    @tmp = split;

    ( $startdate, $starttime, $stopdate, $stoptime,
      $elapsed, $nodes, $user, $executable ) = @tmp; 

    push @log, [ @tmp ]; 

    $userstats{$startdate}{$user} = 0;
    $timestats{$startdate}        = 0;
    $nodestats{$startdate}        = 0;

}

for $line ( @log ) {
    ( $startdate, $starttime, $stopdate, $stoptime,
      $elapsed, $nodes, $user, $executable ) = @$line;

    $userstats{$startdate}{$user}++;

    ( $hours, $minutes, $seconds ) = split ':', $elapsed;

    $timestats{$startdate} +=
           ($hours * 3600) + ($minutes * 60) + $seconds;

    $nodestats{$startdate} += $nodes;

}

$totalusers = 0;
$totaljobs  = 0;
$totaltime  = 0;
$totalsize  = 0;
$date       = "";
$numusers   = 0;
$numjobs    = 0;
$time       = "";

format OUTPUT_TOP = 

                  Cplant Usage Statistics

  Date    Number of Users  Number of Jobs  Total Job Time
--------  ---------------  --------------  --------------
.

format OUTPUT =
@>>>>>>>  @>>>>>>>>>>>>   @>>>>>>>>>>>> @>>>>>>>>>>>>>>
$date,$numusers,$numjobs,$time
.

$~ = "OUTPUT";

foreach $date ( keys %userstats ) {
   
    $numjobs   = 0;
    $numusers  = keys %{$userstats{$date}};

    $daytime   = $timestats{$date};

    $totaltime += $daytime;

    $hours     = int $daytime / 3600;
    $daytime   = $daytime % 3600; 
    $minutes   = int $daytime / 60;
    $daytime   = $daytime % 60;
    $seconds   = $daytime;
    if ( $seconds < 10 ) { $seconds = "0$seconds" };
    if ( $minutes < 10 ) { $minutes = "0$minutes" };
    $time      = "$hours:$minutes:$seconds";

    foreach $user ( keys %{$userstats{$date}} ) {
        $numjobs += $userstats{$date}{$user};
    }

    $totalusers += $numusers;
    $totaljobs  += $numjobs;
    $totalsize  += $nodestats{$date};

#    print "$date" .
#          "         $numusers" .
#          "               $numjobs"  .
#          "             $hours:$minutes:$seconds\n";


    write;

}

$~ = STDOUT;

$numdays      = keys %userstats;
$averageusers = $totalusers / $numdays;
$averagejobs  = $totaljobs  / $numdays;
$averagetime  = $totaltime  / $numdays;
$averagesize  = $totalsize  / $totaljobs;

use integer;

$hours         = int $averagetime / 3600;
$averagetime   = $averagetime % 3600; 
$minutes       = int $averagetime / 60;
$averagetime   = $averagetime % 60;
$seconds       = $averagetime;

print "\n";
printf "Average number of users per day = %4.2f\n", $averageusers;
printf "Average number of jobs per day  = %4.2f\n",$averagejobs;
print  "Average amount of time per day  = $hours:$minutes:$seconds\n";
printf "Average job size                = %4.2f\n",$averagesize; 


