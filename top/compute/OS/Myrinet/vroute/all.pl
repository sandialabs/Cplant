#! /usr/bin/perl

  # all.pl
  # verify routes from this node to all nodes in all SUS
  # RUN FROM service/compute node
  # usage: all.pl

  # verify nodes that show up in pingd
  $str = `/cplant/sbin/pingd`;
  @lines = split(/\n/, $str);

  foreach $i (@lines) {
    $_ = $i;
    if ( /SU/ ) {
      if ( /node/ ) {
        print("skipping: $i\n"); 
      }
      else {
        @words = split();
        $nid = $words[0];
        print("pinging $nid (doing /cplant/sbin/vping $nid)\n");
        $ec = `/cplant/sbin/vping $nid`;
        print("$ec\n");
      }
    }
  }
