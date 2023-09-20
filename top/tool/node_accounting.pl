#!/usr/bin/perl -w

#
# Program to execute pingd and qstat on a node, parse and
# collect data for accounting purposes.
# This was designed to ouput data to duplicate what the
# tflop system was outputting. We actually have potentially more
# data to report, so I will collect it to make it easier to get
# later if needed.
#
# JHL
#

$DEBUG=0;

use Net::Ping;

#
# Need vm, we want to use the bebopd node to obtain status info
# since a service node may be down and the system fine
#
($RELEASE=shift) or die "No vm parameter provided\n";

print "RELEASE $RELEASE\n" if $DEBUG;

$rsh="/usr/bin/rsh";
$pingd="/cplant/sbin/pingd";
$qstat="/bin/qstat";
$tflops_out="/usr/local/system/work/PINGD/tflops_out";
$lock_file="/usr/local/system/work/PINGD/stat_lock";
$system_time_lock="/usr/local/system/work/PINGD/system_time_lock";


# 
# this lock check probably is only really meaningful in the
# long run for system times but I will leave it in until 
# it looks like most of the bugs are out.
#
(&check_lock($system_time_lock,18000)) or 
    die "Lock file $system_time_lock present, in system time??\n";
(&check_lock($lock_file,600)) or 
    die "Lock file $lock_file present, previous run didn't complete??\n";

#
# put a lock file in place before we proceed.
#
`touch $lock_file`;

$bebopd_node=&get_bebopd_node($RELEASE);

#
# can we talk to the node?
#
$p = Net::Ping->new("tcp");

$p->ping($bebopd_node,3) or 
    mail_admin("Cannot ping bebopd node $bebopd_node",1);

#
# Think we can, but put in an eval/alarm just in case we get hung...
#
eval {
    local $SIG{ALRM} = sub { die "alarm\n" };
    alarm 60;
    @pingd_out=`$rsh -n $bebopd_node $pingd -summary`;
    @qstat_out=`$rsh -n $bebopd_node $qstat -a`;
    alarm 0;
};
if ($@) {
    die "error retuned $@\n"  unless $@ eq "alarm\n";
}

chomp($date=`date`);

#
# We should have the data we need, process it to prepare for output.
#

my ($tot,$tot_busy,$tot_pend_alloc,$tot_free)=(&process_pingd_output(\@pingd_out))[0,1,2,3];

($held_nodes, $username)=&process_qstat_output(\@qstat_out);

#
# We want to print our output in the fashion the TFLOPS system prints
# it at least for the time being....
#
# This is an example from the tflops output
#
# Thu Jan 13 00:00:40 MST 2000  Total Config: 3352 Total Free: 1146 NQS Block : disable (pri=59,uid=None,nodes=0)
#
#
# the pri will be printed as 0, the uid since for now not important will
# be the username, the nodes are the number of nodes blocked waiting 
# for enough to run the queued job.
#

open (FD,">>$tflops_out") or 
    die "Cannot open output $tflops_out for append ($!)";

print FD $date . "  Total Config: " . ($tot_busy + $tot_pend_alloc + $tot_free) .  " Total Free: " . $tot_free . " NQS Block : disable (pri=0,uid=". $username . ",nodes=" . $held_nodes . ") Full Configuration: " . $tot . "\n";

close FD;

# 
# get rid of lock file looks like we ran ok
#
`rm $lock_file`;

###############################################################################

sub process_qstat_output {

    $output=shift or 
        die "Did not pass output array into process_qstat_output\n";
    $current_greatest_hours=$current_greatest_minutes=$current_reqd_nodes=0;
    $current_username="None";
    for (@$output) {
        if (/^.+:.+:.+$/) {
            my ($jobid,$username,$queue,$jobname,$sessid,$time_in_queue,$reqd_nodes,$reqd_time,$s,$elap_time) = split;

            next unless ($s eq "Q");
            my ($hours,$minutes) = split(/:/,$time_in_queue);
            if ($hours > 24) {
                if ($current_greatest_hours < $hours) {
                    $current_greatest_hours=$hours;
                    $current_greatest_minutes=$minutes;
                    $current_reqd_nodes=$reqd_nodes;
                    $current_username=$username;
                } elsif ($current_greatest_hours == $hours and 
                         $current_greatest_minutes < $minutes) {
                    $current_greatest_minutes=$minutes;
                    $current_reqd_nodes=$reqd_nodes;
                    $current_username=$username;
                }
            } 

        }
    }
    return ($current_reqd_nodes,$current_username);
}

sub process_pingd_output {

    $output=shift or 
        die "Did not pass output array into process_pingd_output\n";

    #
    # important that the value is 0 if it doesn't get assigned.
    #
    @ret_list=(0,0,0,0,0,0,0,0,0,0,0);
    #

    #
    # collecting a lot more data than required, may need it some day.
    #

    for (@$output) {
        if (/(Total:)\s+(\d+$)/) {  
            $ret_list[0]=$2; 
        } elsif (/(Total busy:)\s+(\d+$)/) { 
            $ret_list[1]=$2; 
        } elsif (/(Total pending allocation:)\s+(\d+$)/) { 
            $ret_list[2]=$2; 
        } elsif (/(Total free:)\s+(\d+$)/) { 
            $ret_list[3]=$2; 
        } elsif (/(Total not responding to ping \(try again\):)\s+(\d+$)/) { 
            $ret_list[4]=$2; 
        } elsif (/(Total nodes unavailable:)\s+(\d+$)/) { 
            $ret_list[5]=$2; 
        } elsif (/(Total reserved:)\s+(\d+$)/) { 
            $ret_list[6]=$2; 
        } elsif (/(Nodes currently hosting PBS jobs:)\s+(\d+$)/) { 
            $ret_list[7]=$2; 
        } elsif (/(Nodes currently hosting interactive jobs:)\s+(\d+$)/) { 
            $ret_list[8]=$2; 
        } elsif (/(Free nodes remaining for interactive jobs:)\s+(\d+$)/) { 
            $ret_list[9]=$2; 
        } 
    }

    return @ret_list;

}

sub get_bebopd_node {

    my $RELEASE=shift or 
        die "Did not pass release to get_bebopd_node\n";
    $VM_CONFIG="/cplant/vm/$RELEASE/nfs-cplant/etc/vm-config";
    open (FD, "<$VM_CONFIG") or die "Cannot open $VM_CONFIG for read ($!)";
    while (<FD>) {
        if (/^#/) { next; }
        if (/bebopd/) {
            ($bebopd_node)=(split(/:/))[0];
            print "Bebopd node $bebopd_node\n" if $DEBUG;
        }
    }
    close FD;
    return ($bebopd_node);
}

sub mail_admin {

    $error_message=shift;
    $exit_stage_left=shift;

    $admin_list="jhlaros\@sandia.gov,gmcgir\@sandia.gov";
    $system=`/bin/hostname`;

    open(MAIL, "|/bin/mail $admin_list -s 'Error with accounting run on $system'")
        or die "Cannot fork mail: $! \n";
    print MAIL $error_message;
    close(MAIL);
    die if $exit_stage_left;
}

sub check_lock {

    # 
    # check for passed in lock file, and how long it has been there, if surpasses 
    # time, mail administrators.
    #

    my $lock_file=shift or die "No lock file passed to check_lock\n";
    my $bitch_time=shift or die "No time passed to check_lock\n";

    if (-f $lock_file) {
        ($mtime)=(stat $lock_file)[9];
        $time_since_lock= time - $mtime;
        if ($time_since_lock > $bitch_time) {
            &mail_admin( "Stat lock in place for $time_since_lock seconds, this is ok if we are in system time, or otherwise down. If not you may want to check the system or manually remove the lock file $lock_file", 1);
        }
        return 0;
    } else {
        # no lock file
        return 1;
    }

}
