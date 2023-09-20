#! /usr/bin/perl

  # getNID.pl -- based on given hostname string, indicate 
  # via output assigned node id by examining 
  # entries of file $CPLANT_PATH/etc/cplant-map and seeing if
  # some entry is a substring of hostname. if so, output entry
  # (position in file) as node id. else, output -1.

  # usage: getNID.pl hostname

if ( $ARGV[0] eq "" ) {
  print("usage: getNID.pl hostname\n");
  exit 0;
}
$hostname = $ARGV[0];

$file = "$ENV{CPLANT_PATH}/etc/cplant-map";
#$file = "/cplant/etc/cplant-map";

if ( !open(CMAP,"$file") ) {
  print("getNID.pl: cant open $file\n");
  exit 0;
}

$i = 0;
while(<CMAP>) {
    if (/"/ ) {
      @words = split(/"/);
      $nmstr = $words[1];
    }
    else {
      @words = split();
      $nmstr = $words[0];
    }
    if ( $nmstr eq $hostname ) {
      print("$i");
      exit 0;
    }
   $i++;        
}
print("-1");
