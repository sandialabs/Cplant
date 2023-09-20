#!/usr/bin/perl -w

#
# Program to produce a short report on usage
# for cplant...
# 
# JHL

#
# Read in all available data
#

use Time::Local;
#use Time::localtime;

$|=1;

#$data="/usr/local/system/work/PINGD/test_data";
$data="/usr/local/system/work/PINGD/tflops_out";
#$data="/usr/local/jhlaros/work/PINGD/tflops_out";

#
# Dynamically get system name
# This only works correctly with names in the format of our admin nodes!!!
#
$system=(split(/\-/,`hostname`))[0];

open (FD, "< $data") or die "Could not open $data for read ($!)";

while (<FD>) {
    my ($dow,$mth,$day,$tod,$year,$tot_config,$tot_free,$disable,$full_config)
    = (split)[0,1,2,3,5,8,11,15,18];
    
    #
    # convert to epoch seconds.
    #
    my($hours,$minutes,$seconds)=split(/:/,$tod);
    my($node_disable)=(split(/\=/,(split(/\,/,$disable))[2]))[1];
    chop($node_disable);
    #
    # hmm, don't see a perl way of doing this other than a map
    # date is pretty standard... do the mth name to number conversion
    # that timelocal needs.
    # remember months start at 0!!!
    #
    chomp($num_mth=`date +%-m -d "$mth 1"` - 1 );
    $etime=timelocal($seconds, $minutes, $hours,$day,$num_mth,$year-1900);
    #
    # Epoch time is a good unique number to use as a hash key.
    #
    $epoch_hash{$etime}=[$tot_config, $tot_free, $node_disable, $full_config];
}
    

#
# So we have a hash table keyed by the epoch time which is unique for
# all the samples, we want to be able to produce a variety of reports
#

($curr_yday) = (localtime())[7];

$tot_config_day=$tot_free_day=$node_disable_day=$full_config_day=$nos_day=0;
$tot_config_week=$tot_free_week=$node_disable_week=$full_config_week=$nos_week=0;
$tot_config_mth=$tot_free_mth=$node_disable_mth=$full_config_mth=$nos_mth=0;
$tot_config_all=$tot_free_all=$node_disable_all=$full_config_all=$nos_all=0;

for $entry (keys(%epoch_hash)) {
    next if @{$epoch_hash{$entry}}[0] == 0; # bad entry ignore!!!
    if ( $curr_yday == (localtime($entry))[7] ) {
        $tot_config_day=$tot_config_day + @{$epoch_hash{$entry}}[0];
        $tot_free_day=$tot_free_day + @{$epoch_hash{$entry}}[1];
        $node_disable_day=$node_disable_day + @{$epoch_hash{$entry}}[2];
        $full_config_day=$full_config_day + @{$epoch_hash{$entry}}[3];
        $nos_day++;
    } 
    if ((localtime($entry))[7] <= $curr_yday and
        (localtime($entry))[7] > $curr_yday-7) {
        $tot_config_week=$tot_config_week + @{$epoch_hash{$entry}}[0];
        $tot_free_week=$tot_free_week + @{$epoch_hash{$entry}}[1];
        $node_disable_week=$node_disable_week + @{$epoch_hash{$entry}}[2];
        $full_config_week=$full_config_week + @{$epoch_hash{$entry}}[3];
        $nos_week++;
    }
    if ((localtime($entry))[7] <= $curr_yday and
        (localtime($entry))[7] > $curr_yday-30) {
        $tot_config_mth=$tot_config_mth + @{$epoch_hash{$entry}}[0];
        $tot_free_mth=$tot_free_mth + @{$epoch_hash{$entry}}[1];
        $node_disable_mth=$node_disable_mth + @{$epoch_hash{$entry}}[2];
        $full_config_mth=$full_config_mth + @{$epoch_hash{$entry}}[3];
        $nos_mth++;
    }
    $tot_config_all=$tot_config_all + @{$epoch_hash{$entry}}[0];
    $tot_free_all=$tot_free_all + @{$epoch_hash{$entry}}[1];
    $node_disable_all=$node_disable_all + @{$epoch_hash{$entry}}[2];
    $full_config_all=$full_config_all + @{$epoch_hash{$entry}}[3];
    $nos_all++;
} 

push @results, "Accounting output for system $system on " .  `date` . "\n\n";
push @results, "Average Total Nodes Configured for day:\t\t\t\t" . $tot_config_day/$nos_day . "\n";
push @results, "Average Total Nodes Free for day:\t\t\t\t" . $tot_free_day/$nos_day . "\n";
push @results, "Average Total Nodes Blocked for day:\t\t\t\t" . $node_disable_day/$nos_day . "\n";
push @results, "Average Total (Full) Nodes Configured for day:\t\t\t" . $full_config_day/$nos_day . "\n";
push @results, "Average Percent Utilized:\t\t\t\t\t" . 
      (($tot_config_day/$nos_day-$tot_free_day/$nos_day)/($tot_config_day/$nos_day))*100 . "\n";
push @results, "Number of samples for day:\t\t\t\t\t" . $nos_day . "\n\n\n";


push @results, "Average Total Nodes Configured for 7 days:\t\t\t" . $tot_config_week/$nos_week . "\n";
push @results, "Average Total Nodes Free for 7 days:\t\t\t\t" . $tot_free_week/$nos_week . "\n";
push @results, "Average Total Nodes Blocked for 7 days:\t\t\t\t" . $node_disable_week/$nos_week . "\n";
push @results, "Average Total (Full) Nodes Configured for 7 days:\t\t" . $full_config_week/$nos_week . "\n";
push @results, "Average Percent Utilized:\t\t\t\t\t" . 
      (($tot_config_week/$nos_week-$tot_free_week/$nos_week)/($tot_config_week/$nos_week))*100 . "\n";
push @results, "Number of samples for 7 days:\t\t\t\t\t" . $nos_week . "\n\n\n";


push @results, "Average Total Nodes Configured for 30 days:\t\t\t" . $tot_config_mth/$nos_mth . "\n";
push @results, "Average Total Nodes Free for 30 days:\t\t\t\t" . $tot_free_mth/$nos_mth . "\n";
push @results, "Average Total Nodes Blocked for 30 days:\t\t\t" . $node_disable_mth/$nos_mth . "\n";
push @results, "Average Total (Full) Nodes Configured for 30 days:\t\t" . $full_config_mth/$nos_mth . "\n";
push @results, "Average Percent Utilized:\t\t\t\t\t" . 
      (($tot_config_mth/$nos_mth-$tot_free_mth/$nos_mth)/($tot_config_mth/$nos_mth))*100 . "\n";
push @results, "Number of samples for 30 days:\t\t\t\t\t" . $nos_mth . "\n\n\n";


push @results, "Average Total Nodes Configured for available data:\t\t" . $tot_config_all/$nos_all . "\n";
push @results, "Average Total Nodes Free for available data:\t\t\t" . $tot_free_all/$nos_all . "\n";
push @results, "Average Total Nodes Blocked for available data:\t\t\t" . $node_disable_all/$nos_all . "\n";
push @results, "Average Total (Full) Nodes Configured for available data:\t" . $full_config_all/$nos_all . "\n";
push @results, "Average Percent Utilized:\t\t\t\t\t" . 
      (($tot_config_all/$nos_all-$tot_free_all/$nos_all)/($tot_config_all/$nos_all))*100 . "\n";
push @results, "Number of samples for available data:\t\t\t\t" . $nos_all . "\n";

&mail_results(@results);
#print @results;

sub mail_results {
    #$receipt_list="$system-help\@sandia.gov";
    $receipt_list="$system-help\@sandia.gov,alhale\@sandia.gov,jpnoe\@sandia.gov,ndpundi\@sandia.gov,dwdoerf\@sandia.gov,lafisk\@sandia.gov";

    open(MAIL, "|/bin/mail $receipt_list -s 'Accounting run for $system'")
        or die "Cannot fork mail: $! \n";
    print MAIL @_;
    close(MAIL);
}

