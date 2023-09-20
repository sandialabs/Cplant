#! /usr/bin/perl

# mping

# for each machine specified on the command line, convert
# canonical machine name such as c-X.SU-Y to physical node
# id and run "$CPLANT/vping nid" to ping the destination

$cplant_path = "CPLANT_PATH";

$CPLANT = "$cplant_path/sbin";

if ( @ARGV == 0 ) {
  print("mping[1]: machine list is empty\n");
  print("usage: mping machine-list\n");
  print("usage: where machine names are of the form c-X.SU-Y\n");
  exit(-1);
}

$hostname = `hostname`;
$_ = $hostname;

if ( /alaska/ ) {
  $system = alaska;
}

if ( /siberia/ ) {
  $system = siberia;
}

if ( /iceberg/ ) {
  $system = iceberg;
}

foreach $machine (@ARGV) {

  $nid = `$CPLANT/hname2pnid $machine $system`;

  $out = `$CPLANT/vping $nid`;

  print("$out");
  print("\n");
  
}
