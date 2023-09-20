#! /usr/bin/perl

# showmesh for perl/Tk -- this version runs on the
# service node, but a quick mod and it runs on the
# SSS-X node also...

# need Tk extension to perl where it runs

use Tk;

#-----------------------------------------------------------------------
# configure either for service node or for SSS node

# SSS node
$service = "c-7.SU-3";
$cmd_prefix = "rsh $service";

# service node
#$cmd_prefix = "";
#-----------------------------------------------------------------------

#-------------------canvas configuration parameters----------------------
$NUM_SUS = 45;

# in displaying a grid of SUs, define the max no. of rows
# to use
$MAX_ROWS_SU = 8;

# in displaying a grid of nodes (eg. in a blow-up of an su),
# define the max no. of rows to use
$MAX_ROWS_N = 8;

$NODES_PER_SU = 16;

$delta_x = 45;
$delta_y = 25;

# base after which you can place sus or nodes -- leaves space for
# a label

$y_base_su = 0;
$x_base_su = 20;

$y_base_n = 20;
$x_base_n = 20;

$x_node_base = 20;
$x_node_curr = $x_node_base;
$x_node_incr = 50;

# max no. of digits in an SU "name" -- pad out shorter
# names to this length
#$MAX_LEN = length($NUM_SUS);
$MAX_LEN = 3;

# scrolling limits for grid of SUs
$maxy_su = $x_base_su + $MAX_ROWS_SU * ($delta_y + 2);
$maxx_su = $y_base_su + int($NUM_SUS/$MAX_ROWS_SU+1) * ($delta_x + 2);

# scrolling limits for grid of nodes
$maxy_n = $x_base_n + $MAX_ROWS_N * ($delta_y + 2);
$maxx_n = $y_base_n + int($NUM_SUS) * ($delta_x + 2);
#------------------------------------------------------------------

# main window
my $mw = MainWindow->new;
$mw->title(Showmesh);

# frame for buttons
$frame_butts1 = $mw->Frame(-relief => 'groove', -borderwidth => 2);
$frame_butts1->pack(-fill => 'both');

# a pingd button
$pbutt = $frame_butts1->Menubutton(-text => "pingd", -relief => "raised",
-tearoff => 0,
-menuitems => [[ "command" => "pingd", -command => \&do_pingd ],
               [ "command" => "erase", -command => \&do_erase ],
               [ "command" => "key", -command => \&do_key ],
               [ "command" => "close", -command => sub { exit }]
              ]);
$pbutt->pack(-side => "left", -anchor => "n");

# an operations menu button
$obutt = $frame_butts1->Menubutton(-text => "ops", -relief => "raised",
-tearoff => 0,
-menuitems => [[ "command" => "system log", -command => \&do_sys_log ],
               [ "command" => "cplant log", -command => \&do_cplant_log ],
               [ "command" => "ptlDebug", -command => \&do_ptlDebug ]
              ]);
$obutt->pack(-side => "left", -anchor => "n");

# a quit button
#$qbutt = $frame_butts1->Menubutton(-text => "quit", -relief => "raised",
#-tearoff => 0,
#-menuitems => [ [ "command" => "quit", -command => sub { exit }]
#              ]);
#$qbutt->pack(-side => "right", -anchor => "n");

# a frame above the su canvas
$frame_sus = $mw->Frame(-label => "SUs", -relief => 'groove', -borderwidth => 2);
$frame_sus->pack(-fill => 'both');

# a canvas for the grid of SUs
$canvas_sus = $mw->Scrolled('Canvas', -height => $maxy_su, -width => $maxy_su*1.5, -scrollregion => [ 0, 0, $maxx_su, $maxy_su] );
$canvas_sus->pack();

# a frame above the node canvas
$frame_nodes = $mw->Frame(-label => "Nodes", -relief => 'groove', -borderwidth => 2);
$frame_nodes->pack(-fill => 'both');

# a canvas for blow-ups of SUs which can be
# used to select individual nodes
$canvas_nodes = $mw->Scrolled('Canvas', -height => $maxy_n, -width => $maxy_n*1.5, -scrollregion => [ 0, 0, $maxx_n, $maxy_n] );
$canvas_nodes->pack();

# frame for totals
$frame_tots = $mw->Frame(-relief => 'groove', -borderwidth => 2);
$frame_tots->pack(-fill => 'both');

$label_total = $frame_tots->Label(-text => "Total:");
$label_total->pack(-side => "left", -anchor => "n");

$entry_total = $frame_tots->Entry(-width => 5, -state => "disabled");
$entry_total->pack(-side => "left", -anchor => "n");

$label_total_busy = $frame_tots->Label(-text => "     Total busy:");
$label_total_busy->pack(-side => "left", -anchor => "n");

$entry_total_busy = $frame_tots->Entry(-width => 5, -state => "disabled");
$entry_total_busy->pack(-side => "left", -anchor => "n");

$label_total_free = $frame_tots->Label(-text => "     Total free:");
$label_total_free->pack(-side => "left", -anchor => "n");

$entry_total_free = $frame_tots->Entry(-width => 5, -state => "disabled");
$entry_total_free->pack(-side => "left", -anchor => "n");

MainLoop;

sub do_pingd {

  do_erase();

  $str = `$cmd_prefix /cplant/sbin/pingd`;

  # get lines of pingd output
  @lines = split(/\n/, $str);

  # for each line w/ "SU" add entry to list of SUs and list
  # of nodes -- also, add status tag to node. see below or
  # do_key() for
  # meaning of the status tags

  foreach $i (@lines) {
    $_ = $i;
    @words = split();
    if ( /SU/ ) {
      # split SU-00x on dash and get the latter part as the SU index
      @word2 = split(/-/, $words[2]);
      $index_su = int($word2[1]);

      # split n-00x on dash and get the latter part as the node index
      @word3 = split(/-/, $words[3]);
      $index_node = int($word3[1]);

      $su[$index_su]="green";
      $node[$index_su][$index_node]="green";

      # if there is more stuff after the node string assume it's the "is
      # busy" data; if this isn't correct, the correct status will be
      # determined below...

      if ( @words > 8 ) {
        $node[$index_su][$index_node]="blue";
      } 

      if ( /is stale/ ) {
        $node[$index_su][$index_node]="black";
      }
      if ( /possible allocation/ ) {
        $node[$index_su][$index_node]="yello";
      }
      if ( /is unavailable/ ) {
        $node[$index_su][$index_node]="red";
      }
    }

    if ( /Total:/ ) {
      $entry_total->configure(-state => "normal"); 
      $entry_total->insert(0, $words[1]);
      $entry_total->configure(-state => "disabled"); 
    }

    if ( /Total busy:/ ) {
      $entry_total_busy->configure(-state => "normal");
      $entry_total_busy->insert(0, $words[2]);
      $entry_total_busy->configure(-state => "disabled");
    }

    if ( /Total free:/ ) {
      $entry_total_free->configure(-state => "normal");
      $entry_total_free->insert(0, $words[2]);
      $entry_total_free->configure(-state => "disabled");
    }

  }

  for $i (0 .. $#su) {
    if ( $su[$i] ) {
      #print(STDOUT "SU $i: $su[$i]\n");
    }
  }


  for $j (0 .. $#node) {
    for $i (0 .. $#{$node[$j]}) {
      if ( $node[$j][$i] ) {
      # print(STDOUT "SU $j, node $i: $node[$j][$i]\n");
      }
    }
  }

  # now do graphical representation of SUs and nodes using the
  # super duper Cplant scalable interface (SD/CSI)


  $row = 1;
  $x = $x_base_su;

  $count_good_su = 0;
  $count_node= 0;
  for ($i=0; $i<=$NUM_SUS; $i++) {

    $su_blown_up[$i] = 0;

    if ($row > $MAX_ROWS_SU) {
      $row = 1;
      $x+= $delta_x;
    }

    $pad = $MAX_LEN - length($i);

    $name = "0" x $pad;

    $name .= $i;

    $y = $y_base_su + $row*$delta_y;

    if ( $su[$i] eq "green" ) {
      $chk_butt[$i] = $canvas_sus->Checkbutton(-text => "$name", 
      -indicatoron => 0, -variable => \$su_var[$i], -background => "green", 
      -selectcolor => "green");
    }
    else {
      $chk_butt[$i] = $canvas_sus->Checkbutton(-text => "$name", 
      -indicatoron => 0, -variable => \$su_var[$i], -selectcolor => "gray");
    }

    $id_sus[$i] = $canvas_sus->createWindow($x, $y, -window=>$chk_butt[$i]);

    $row++;

    # if appropriate, draw blow-up in nodes canvas
    if ( ($su[$i] eq "green") ) {

      # use nodes canvas to draw blow-up of SU (i.e., its nodes)

      $su_blown_up[$i] = 1;

      $delta_x_node= 30;

      $row_node = 1;
      $x_node = $x_node_curr;

      $id_text[$count_good_su++] = $canvas_nodes->createText(
                    $x_node_curr, $y_base_n, -text=>"SU $i", -anchor => 'w');

      for ($j=0; $j<$NODES_PER_SU; $j++) {

        if ($row_node > $MAX_ROWS_N) {
          $row_node = 1;
          $x_node += $delta_x_node;
        }

        # assumption that nodes per su <= 99

        $pad = 2 - length($j);

        $name_node = "0" x $pad;

        $name_node .= $j;

        $y_node = $y_base_n + $row_node*$delta_y;

        $color = $node[$i][$j];

        if ( $color ) {
          $chk_butt_n[$count_node] = $canvas_nodes->Checkbutton(
            -text => "$name_node", 
            -indicatoron => 0, -background => $color, -selectcolor => $color, 
            -variable => \$node_var[$count_node],
            -command => [\&node_click,$count_node,$i,$j] );
        }
        else {
          $chk_butt_n[$count_node] = $canvas_nodes->Checkbutton(
            -text => "$name_node", 
            -indicatoron => 0, -selectcolor => "gray", 
            -variable => \$node_var[$count_node],
            -command => [\&node_click,$count_node,$i,$j] );
        }
        $su_ind[$count_node] = $i;
        $node_ind[$count_node] = $j;
      

        $id_nodes[$count_node] = $canvas_nodes->createWindow($x_node, $y_node, 
                  -window=>$chk_butt_n[$count_node]);

        $count_node++;

        $row_node++;

      }

      $x_node_curr = $x_node + $x_node_incr;
    }
  }
}


sub do_erase 
{

  # first, clear canvases
  for $i ( 0 .. $#id_nodes ) {
    $canvas_nodes->delete( $id_nodes[$i] );
  }

  for $i ( 0 .. $#id_text ) {
    $canvas_nodes->delete( $id_text[$i] );
  }

  for $i ( 0 .. $#id_sus ) {
    $canvas_sus->delete( $id_sus[$i] );
  }

  # reinit pos. of blow-ups
  $x_node_curr = $x_node_base;

  # clear total entry windows
  $entry_total->configure(-state => "normal");
  $entry_total->delete(0, "end");
  $entry_total->configure(-state => "disabled");

  $entry_total_busy->configure(-state => "normal");
  $entry_total_busy->delete(0, "end");
  $entry_total_busy->configure(-state => "disabled");

  $entry_total_free->configure(-state => "normal");
  $entry_total_free->delete(0, "end");
  $entry_total_free->configure(-state => "disabled");

  # collapse $node_var -- it contains the select status
  # of nodes
  $#node_var = -1;
}

sub do_key 
{
  if (!Exists($kw)) {
    $kw = $mw->Toplevel();
    $kw->title("Showmesh Key");
    $clbutt = $kw->Button(-text => "Close", -command => sub { $kw->withdraw });
    $clbutt->pack(-side => "top", -anchor => "w");
    
    $canvas_k = $kw->Canvas(-height => "160", -width => "180");
    $canvas_k->pack;

    $canvas_k->createRectangle( 15, 15, 25, 25, -fill => "green");
    $canvas_k->createRectangle( 15, 45, 25, 55, -fill => "red");
    $canvas_k->createRectangle( 15, 75, 25, 85, -fill => "black");
    $canvas_k->createRectangle( 15, 105, 25, 115, -fill => "blue");
    $canvas_k->createRectangle( 15, 135, 25, 145, -fill => "yellow");

    $canvas_k->createText( 33, 20, -anchor => "w", -text => "-- node is available");
    $canvas_k->createText( 33, 50, -anchor => "w", -text => "-- node is unavailable");
    $canvas_k->createText( 33, 80, -anchor => "w", -text => "-- node data is stale");
    $canvas_k->createText( 33, 110, -anchor => "w", -text => "-- node is busy");
    $canvas_k->createText( 33, 140, -anchor => "w", -text => "-- node status pending");

  }
  else {
    $kw->deiconify();
    $kw->raise();
  }
}

sub node_click
{
  my $index = $_[0];
#  print("hi from c-$node_ind[$index].SU-$su_ind[$index]\n");
#  print("my status is: $node_var[$index]\n");
}

sub do_sys_log
{
  # for each selected node, do the indicated operation
  for ($i=0; $i<$count_node; $i++) {
    if ( $node_var[$i] == 1 ) {
#      print("hi from c-$node_ind[$i].SU-$su_ind[$i]\n");
      $rc = `rsh c-$node_ind[$i].SU-$su_ind[$i] cat /var/log/messages`;
      print("$rc\n");
    }
  }
}

sub do_cplant_log
{
  # for each selected node, do the indicated operation
  for ($i=0; $i<$count_node; $i++) {
    if ( $node_var[$i] == 1 ) {
#      print("hi from c-$node_ind[$i].SU-$su_ind[$i]\n");
      $rc = `rsh c-$node_ind[$i].SU-$su_ind[$i] cat /var/log/cplant`;
      print("$rc\n");
    }
  }
}

sub do_ptlDebug
{
  # for each selected node, do the indicated operation
  for ($i=0; $i<$count_node; $i++) {
    if ( $node_var[$i] == 1 ) {
#      print("hi from c-$node_ind[$i].SU-$su_ind[$i]\n");
      $rc = `rsh c-$node_ind[$i].SU-$su_ind[$i] /cplant/sbin/ptlDebug`;
      print("$rc\n");
    }
  }
}
