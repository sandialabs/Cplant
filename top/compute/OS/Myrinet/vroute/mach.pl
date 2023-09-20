#! /usr/bin/perl

  # mach.pl
  # verify routes from all nodes
  # to "all" other nodes
  # RUN FROM ADMIN NODE as a foreground process
  # or as a cron job BY ROOT
  # usage: mach.pl [no command line options]

  # Valid nodes are determined by rsh-ing to the
  # first entry in @svc_nodes and doing "pingd"

  # need to configure in service nodes (this can be thought of
  # as an auxiliary set of nodes to check beyond the ones pingd
  # will inform us about) and who to notify w/ results...

  # CONFIGURATION ########################################################
  @whotocontact = ("jhlaros\@sandia.gov", "jsotto\@sandia.gov", 
                   "gmcgir\@sandia.gov", "redavis\@sandia.gov");
  @svc_nodes = ("c-0.SU-0", "c-0.SU-2", "c-0.SU-4", "c-0.SU-6", "c-0.SU-8",
               "c-0.SU-10", "c-0.SU-12", "c-0.SU-14", "c-0.SU-16", 
               "c-0.SU-18", "c-0.SU-21");
  ########################################################################

  $failures = 0;

  for ($i=1; $i<@whotocontact; $i++) {
    $contact_str .= " -c $whotocontact[$i]";
  }
  $contact_str .= " $whotocontact[0]";

  $file_loc= "/tmp/routing";

  print("removing files in $file_loc\n");
  system("rm $file_loc/*"); 

  $admin = `hostname`;
  chop($admin);

  $service = $svc_nodes[0];

  $msg  = "mach.pl (low-level myrinet diagnostic) starting\n";
  $msg .= "on admin node $admin\n\n";
  $msg .= "another msg will be sent on successful completion...\n\n";
  $msg .= "progress can be monitored in $file_loc/mach.log\n\n";
  system("echo '$msg' | mail -s 'Start of Periodic Myrinet Diagnostic' $contact_str"); 

  # test ability to rsh to service node
  $ec = `rsh $service ps aux 2>&1 &`;

  sleep(15);

  $_ = $ec;
  if ( ! /init/ ) {
    $msg  = "mach.pl (low-level myrinet diagnostic) failed\n";
    $msg .= "on admin node $admin\n\n";
    $msg .= "there seems to be a problem in rsh-ing to service\n";
    $msg .= "node $service\n\n";
    $msg .= "mach.pl is normally run as a cron job from the script\n";
    $msg .= "located in nfs-cplant/sbin of the active vm. it starts\n";
    $msg .= "out by rsh-ing to a service node and running pingd in\n";
    $msg .= "order to find out what nodes are active. it then rsh-es\n";
    $msg .= "to each active nodes and runs all.pl\n\n";
    system("echo '$msg' | mail -s 'Failure of Periodic Myrinet Diagnostic' $contact_str"); 
    print("$msg");
    open(LOG,">>$file_loc/mach.log\n");
    print(LOG "$msg");
    close(LOG);
    exit(-1);
  }

  # obtain pingd output from service node
  # and use it to determine valid set of nodes
  # -- run all.pl on each one
  $pingd = `rsh $service /cplant/sbin/pingd`;
  @lines = split(/\n/, $pingd);
  foreach $i (@lines) {
    $_ = $i;
    if (/SU/) {
      if ( ! /node/ ) {   # if no problem indicated 
        @words = split();
        $sustr = $words[2];
        $nodestr = $words[3];
        @fields = split(/-/, $sustr);
        $sunum = int($fields[1]);
        @fields = split(/-/, $nodestr);
        $nodenum = int($fields[1]);
        $hname = "c-$nodenum.SU-$sunum";

        open(ERRORS,">$file_loc/$hname");

        print("doing all.pl on $hname...\n");
        open(LOG,">>$file_loc/mach.log\n");
        print(LOG "doing all.pl on $hname...\n");
        close(LOG);
        $ec = `rsh $hname /cplant/sbin/all.pl`;
        @cut = split(/\n/, $ec);
        foreach $j (@cut) {
          $_ = $j;
          if (/FAILED/ || /skipping/ || /pinging/) {
            print(ERRORS "$j\n");
            if (/FAILED/) {
              $failures++;
            }
          }
        }
        close(ERRORS);
      }
    }
  }

  # now run all.pl on each of the service nodes -- since there
  # are fewer nodes in this set we can be a little more careful,
  # in particular, check ability to rsh to each...

  open(LOG,">>$file_loc/mach.log\n");
  print(LOG "doing all.pl on service nodes...\n");
  close(LOG);
  print("doing all.pl on service nodes...\n");

  foreach $svc (@svc_nodes) {

    $ec = `rsh $svc ps aux 2>&1 &`;

    sleep(15);

    $_ = $ec;
    if ( ! /init/ ) {
      $msg  = "\nmach.pl (low-level myrinet diagnostic) had a\n";
      $msg .= "problem (non-fatal) on admin node $admin\n\n";
      $msg .= "there seems to be a problem in rsh-ing to service\n";
      $msg .= "node $svc:\n\n";
      $msg .= "$ec\n";
      system("echo '$msg' | mail -s 'Minor Problem with Periodic Myrinet Diagnostic' $contact_str"); 
      print("$msg");
      open(LOG,">>$file_loc/mach.log\n");
      print(LOG "$msg");
      close(LOG);
    }
    else {

      open(ERRORS,">$file_loc/$svc");

      print("doing all.pl on $svc...\n");
      open(LOG,">>$file_loc/mach.log\n");
      print(LOG "doing all.pl on $svc...\n");
      close(LOG);
      $ec = `rsh $svc /cplant/sbin/all.pl`;
      @cut = split(/\n/, $ec);
      foreach $j (@cut) {
        $_ = $j;
        if (/FAILED/ || /skipping/ || /pinging/) {
          print(ERRORS "$j\n");
          if (/FAILED/) {
            $failures++;
          }
        }
      }
      close(ERRORS);
    }
  }

  $msg  = "mach.pl (low-level myrinet diagnostic) completing\n";
  $msg .= "its checkout on $admin of the compute\n";
  $msg .= "nodes in the vm associated w/ service node $service\n\n"; 
  $msg .= "it looks like there were $failures failures recorded in\n";
  $msg .= "$file_loc (grep for FAILED on the files there\n";
  $msg .= "for specific failures)\n";
  system("echo '$msg' | mail -s 'Completion of Periodic Myrinet Diagnostic' $contact_str"); 
  print("$msg");
  open(LOG,">>$file_loc/mach.log\n");
  print(LOG "$msg");
  close(LOG);


