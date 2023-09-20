#!/usr/local/bin/perl

# Edited ron brightwell's cplant-usage to report
# weekly usage, node hours, etc. for alaska
#
# also write a file suitable for gnuplot input
#
#    processes all userlog.YYYYMMDD.YYYYMMDD in cwd
#

#
# this capacity is approximate - you may see some
# days where % capacity calculated exceeds 100
#
$alaskaNodeHourCapacity = 232.0 * 24.0;

open(PLOTOUT, ">weekly-usage.plot") or die "can't open plot file: $!\n";
print PLOTOUT "#\n" ;
print PLOTOUT "# week - users/day - jobs/day  - nodehours/day - % capacity - av job size\n";
print PLOTOUT "#\n" ;
$weeknum = 6;

foreach $logFile (`ls userlog.all.*.*`){

    @fields = split '.',$logFile;
    ($fname, $weekstart, $weekend) = @fields; 

    open(logf, $logFile);
    
    %userstats = ();
    %timestats = ();
    %nodestats = ();
    %nodeseconds = ();

    @log = ();

    while ($line = <logf>){

        @tmp = split ' ',$line;

        ( $startdate, $starttime, $stopdate, $stoptime,
          $elapsed, $nodes, $user, $executable ) = @tmp; 

         push @log, [ @tmp ]; 

         $userstats{$startdate}{$user} = 0;
         $timestats{$startdate}        = 0;
         $nodestats{$startdate}        = 0;
         $nodeseconds{$startdate}        = 0;
    }
    close(logf);
    
    for $line ( @log ) {
        ( $startdate, $starttime, $stopdate, $stoptime,
          $elapsed, $nodes, $user, $executable ) = @$line;
    
        $userstats{$startdate}{$user}++;
    
        ( $hours, $minutes, $seconds ) = split ':', $elapsed;
    
        $seconds = 
               ($hours * 3600) + ($minutes * 60) + $seconds;
        
        $timestats{$startdate} += $seconds;
    
        $nodestats{$startdate} += $nodes;

        $nodeseconds{$startdate} += ($nodes * $seconds);
    
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
 
              Cplant Weekly Usage Statistics
 
    Date    Number of Users  Number of Jobs  Total Node Hours  Approx % Capacity
----------  ---------------  --------------  ----------------  -----------------
.
 
format OUTPUT =
@>>>>>>>>>  @>>>>>>>>>>>>   @>>>>>>>>>>>> @>>>>>>>>>>>>>>      @>>>>>>
$date,$numusers,$numjobs,$time,$percent
.
 
 
@lines = ();

foreach $date ( keys %userstats ) {

    ($mo,$dy,$yr) = split '/', $date;
 
    $numjobs   = 0;
    $numusers  = keys %{$userstats{$date}};
 
#    $daytime   = $timestats{$date};
    $daytime   = $nodeseconds{$date};

    $percent   = $daytime / 36.0 / $alaskaNodeHourCapacity;
    $percent   = int $percent;
 
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

    push @lines, "$mo/$dy/$yr $numusers $numjobs $time $percent ";
 
#    print "$date" .
#          "         $numusers" .
#          "               $numjobs"  .
#          "             $hours:$minutes:$seconds\n";
}
sub firstField
{
    ($n1, $theRest) = split(" ",$a);
    ($n2, $theRest) = split(" ",$b);
    ($m1,$d1,$y1) = split("/",$n1);
    ($m2,$d2,$y2) = split("/",$n2);

    $n1 = "$y1$m1$d1";
    $n2 = "$y2$m2$d2";

    $n1 <=> $n2;
}
@sortedLines = sort firstField @lines;

$nlines = @sortedLines;

$~ = "OUTPUT";

for ($i=0; $i < $nlines; $i++){

    ($date, $numusers, $numjobs, $time, $percent) = split(" " , $sortedLines[$i]);

     write;
}
    
    $~ = STDOUT;
    
    $numdays      = keys %userstats;
    $averageusers = $totalusers / $numdays;
    $averagejobs  = $totaljobs  / $numdays;
    $averagetime  = $totaltime  / $numdays;
    $averagesize  = $totalsize  / $totaljobs;

    $percent   = $averagetime / 36.0 / $alaskaNodeHourCapacity;
    $percent   = int $percent;
    
    $hours         = int $averagetime / 3600;
    $averagetime   = $averagetime % 3600; 
    $minutes       = int $averagetime / 60;
    $averagetime   = $averagetime % 60;
    $seconds       = $averagetime;

    print "\n";
    printf "Average number of users per day = %4.2f\n", $averageusers;
    printf "Average number of jobs per day  = %4.2f\n",$averagejobs;
    print  "Average node hours used per day = $hours:$minutes:$seconds (~ $percent%)\n";
    printf "Average job size                = %4.2f\n\n\n",$averagesize; 

    printf PLOTOUT "%02d   %4.2f   %4.2f   %7d    %5.2f   %4.2f\n",
          $weeknum,$averageusers,$averagejobs,$hours,$percent,$averagesize;
    $weeknum = $weeknum + 1;
}
