#! /usr/bin/perl

# crawl

# usage -- % crawl dest
# where dst is a node id

# crawl the route to the specified destination. i.e.,
# for each byte of the route, do a self-ping along
# the segment of the route terminated by that byte

# the route is parsed from this node's route file in
# the installed routes dir...

# for a given byte, this table holds the byte for the
# reverse direction...

$cplant_path = "CPLANT_PATH";

$usage = machine;

%switch = ("0x80", " 0x80",
           "0x81", " 0xbf",
           "0x82", " 0xbe",
           "0x83", " 0xbd",
           "0x84", " 0xbc",
           "0x85", " 0xbb",
           "0x86", " 0xba",
           "0x87", " 0xb9",
           "0x88", " 0xb8",
           "0x89", " 0xb7",
           "0x8a", " 0xb6",
           "0x8b", " 0xb5",
           "0x8c", " 0xb4",
           "0x8d", " 0xb3",
           "0x8e", " 0xb2",
           "0x8f", " 0xb1",
           "0xbf", " 0x81",
           "0xbe", " 0x82",
           "0xbd", " 0x83",
           "0xbc", " 0x84",
           "0xbb", " 0x85",
           "0xba", " 0x86",
           "0xb9", " 0x87",
           "0xb8", " 0x88",
           "0xb7", " 0x89",
           "0xb6", " 0x8a",
           "0xb5", " 0x8b",
           "0xb4", " 0x8c",
           "0xb3", " 0x8d",
           "0xb2", " 0x8e",
           "0xb1", " 0x8f"
          );

if ( $ARGV[0] eq "" ) {
  print(" Usage: % crawl dest\n");
  print(" where dest is a physical node id\n");
  print("OR\n");
  print("        % crawl route-byte-sequence\n");
  print(" where the sequence is a space-separated list of\n");
  print(" bytes, 0xST, with S and T hex digits\n");
  exit(-1);
}

if ( $ARGV[0] eq "-h" ) {
  print(" Usage: % crawl dest\n");
  print(" where dest is a physical node id\n");
  print("OR\n");
  print("        % crawl route-byte-sequence\n");
  print(" where the sequence is a space-separated list of\n");
  print(" bytes, 0xST, with S and T hex digits\n");
  exit(0);
}

$_ = $ARGV[0];
if (/0x/) {
  $usage = route;
}

if ( $usage eq machine ) {
  $dst = $ARGV[0];

  if ( ! -e "$cplant_path/sbin/getroute" ) {
     print("crawl: ERROR -- cant find $cplant_path/sbin/getroute\n");
     exit(-1);
  }

  $line = `$cplant_path/sbin/getroute $dst`;

  chop($line);

  $line .= " 0x00";

  $_ = $line;

  @bytes = split();
}
else {
  @bytes = @ARGV;
  $bytes[@ARGV] = "0x00";
}

$i = 0;
while (1) {
  if ($bytes[$i] eq "0x00") {
    last;
  }
  else {
    $proute = $froute . " 0x80";
    $froute .= " $bytes[$i]";
    print("--------------------------------------------------------\n");
    print("forward route so far= $froute\n");

    for ($j=$i-1; $j>=0; $j--) {
      $proute .= $switch{$bytes[$j]};
    }
    print("doing self-ping along corr. reverse route:\n"); 
    print("$proute\n");
    $rc = `$cplant_path/sbin/troute $proute`;
    print("$rc\n");
    $i++;
  }
}
