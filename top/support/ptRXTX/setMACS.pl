#! /usr/bin/perl
#
# $Id: setMACS.pl,v 1.1 2001/05/09 16:55:41 pumatst Exp $
#

  # setMACS.pl -- read address from $CPLANT_PATH/etc/cplant-mac
  # for each one, call $CPLANT_PATH/sbin/setMac nid mac_addr

  # usage: setMACS.pl

$CPLANT_PATH = $ENV{CPLANT_PATH};
$file = "$CPLANT_PATH/etc/cplant-mac";

if ( !open(MACS,"$file") ) {
  print("setMACS.pl: cant open $file\n");
  exit 0;
}

$i = 0;
while(<MACS>) {
  # crude test for comment line
  if ( !/#/ ) {
    $line = $_;
    chop($line);
    system("$CPLANT_PATH/sbin/setMac $i $line");
    $i++;
  }
}
