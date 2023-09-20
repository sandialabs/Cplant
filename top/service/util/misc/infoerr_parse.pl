#!/usr/bin/perl -w
#
# $Id: infoerr_parse.pl,v 1.2.2.1 2002/06/18 18:37:27 jbogden Exp $
#
# Wraps the infoerr utility and does post processing so that meaningful
# data is easily seen.


use Getopt::Long;

GetOptions( 'h|U|help' => \$help,
            'sleep=i' => \$sleep_time,
          );

if (defined $help) {
    print "USAGE: $0 [-sleep <seconds>]\n";
    print "           -sleep    number of seconds to sleep between queries\n";
    exit 1;
}

if (!defined $sleep_time) {
    # use default of 10 seconds
    $sleep_time = 10;
}

# init some static config stuff
$num_summary_lines = 6;
$num_node_info_lines = 4;

# setup the spec of what node ids to run the util against
# figure out how many nodes the spec translates to
#$node_spec = "100..101";
#$node_spec = "200,365..367,1698";
$node_spec = "0..1535,1792..1815,1536..1791";
$num_nodes = parse_node_spec($node_spec);

# form the command line we'll use
$util = "infoerr";
$utilopts = "-v -v";
$cmd = "$util $utilopts $node_spec|";

# setup the text names of the counter fields
$num_fields = 9;
%field_to_name = (
    0 => "Node",
    1 => "Packet send errors",
    2 => "Pkt receive errors",
    3 => "Module send errors",
    4 => "Failed user memcpy",
    5 => "Protocol errors",
    6 => "Outstanding packets",
    7 => "Network errors",
    8 => "Module recv errors"
);

# This array maps the numeric counter fields in a 'split' line of output
# from the util to the corresponding text field index (i.e. field_to_name)
#
# -1 indicates the end of a text line we will parse
@field_to_index = (0,4,8,-1, 4,8,-1, 3,6,-1, 3,7,-1);

print "Querying $num_nodes nodes ($node_spec)\n";

grab_and_parse(\%hash1,\@linebuf1,\$num_lines1);

# Wait a bit before grabbing the info again
print "\nSleeping for $sleep_time seconds\n";
print "\n";
sleep $sleep_time;

grab_and_parse(\%hash2,\@linebuf2,\$num_lines2);


print "\n$linebuf1[$num_lines1 - $num_summary_lines]";
$num_nodes_missing = $num_nodes - (($num_lines1-$num_summary_lines)/$num_node_info_lines);
print "num_nodes_missing1 = $num_nodes_missing\n";

print "$linebuf2[$num_lines2 - $num_summary_lines]";
$num_nodes_missing2 = $num_nodes - (($num_lines2-$num_summary_lines)/$num_node_info_lines);
print "num_nodes_missing2 = $num_nodes_missing2\n";


$num_bad_nodes = 0;
 
# Now we want to compare all the keys in %hash1 with the corresponding
# keys in $hash2 and see which values changed, if any.
for $node (sort { $a <=> $b } (keys(%hash1)) ) {
    #print "$hash1{$node}[0] = @hash1{$node}\n";
    for ($i = 1; $i < $num_fields; $i++) {
        if ($i == 6) {
            # ignore outstanding packets
            next;
        }
        
        if ( ($hash2{$node}[$i] > $hash1{$node}[$i]) ) {
            $num_bad_nodes++;
            $diff = $hash2{$node}[$i] - $hash1{$node}[$i];
            print "$node \- $field_to_name{$i}: $hash1{$node}[$i] => $hash2{$node}[$i] (change of $diff)\n";
        }
    }
}
 
print "There seems to be $num_bad_nodes nodes propagating errors.\n";



sub grab_and_parse {
    # get the reference to the hash passed in
    my $hash = shift;
    my $linebuf = shift;
    my $num_lines = shift;
    
    # declare local variables
    my $i;
    my $j;
    my $k;
    my @fields;
    my @val;
    my $num_field_indexes = @field_to_index;
    my @data_array;
    
    #printf "num_field_indexes = $num_field_indexes\n";
    
    # Run the command line and save all the output
    open (INFOERR, $cmd)
    #open (INFOERR, "output1.txt")
        or die "Could not open infoerr for read ($!)";
    @$linebuf = <INFOERR>;
    $$num_lines = @$linebuf;
    close(INFOERR);

    $k = $$num_lines - $num_summary_lines;
    #printf "lines to parse = $k\n";

    # Parse the data for each node and watch out for the summary information
    # at the end of the buffer.
    #for ($i=0; $i < $num_lines - $num_summary_lines;) {
    $i = 0;
    while ($i < $$num_lines - $num_summary_lines - 1) {

        #init the val array
        for ($k=0; $k < $num_fields; $k++) {
            $val[$k] = 0;
        }
        
        # The following block should parse out all data fields for a single
        # node.        
        #for ($j=0; $j < $num_field_indexes;) {
        $j = 0;
        $k = 0;
        while ($j < $num_field_indexes) {
            if ($i > $$num_lines) {return};
            
            #printf "line $i = $$linebuf[$i]";
            @fields = split(" ",$$linebuf[$i]);
            #printf "@fields \n";
            while ($field_to_index[$j] != -1) {
                #printf "j=$j field_to_index = $field_to_index[$j]\n";
                ($val[$k++] = $fields[$field_to_index[$j]]) =~ tr/\://d;
                $j++;
            }
            #printf "val = @val \n";
            $i++;
            $j++;
        }
        
        #printf "i=$i j=$j @val\n";
        
        # insert all the data for a node into the hash table
        for ($k=0; $k < $num_fields; $k++) {
            $$hash{$val[0]}[$k] = $val[$k];
        }
    }
}



sub parse_node_spec {
    my @orig = @_;    
    #printf "@_ \n";
    
    my $spec = shift;
    #print "spec = $spec\n";
    
    my $num_nodes = 0;
    my $temp = 0;
    my $i;
    my @range_field;
    
    # split based on ","
    my @comma_fields = split(",",$spec);
    my $num_commas = @comma_fields;
    #printf "@comma_fields \n";
    #printf "$num_commas\n";
    
    # split each comma field at ".."
    for ($i = 0; $i < $num_commas; $i++) {
        $comma_fields[$i] =~ tr/\.\./-/s;
        @range_fields = split("-",$comma_fields[$i]);
        
        #printf "$comma_fields[$i]\n";
        #printf "$range_fields[1] $range_fields[0]\n";
        if (defined $range_fields[1]) {
            $temp = $range_fields[1] - $range_fields[0] + 1;
            $num_nodes += $temp;
        }
        else {
            $num_nodes++;
        }
    }
    
    #printf "num_nodes = $num_nodes\n";   
    return $num_nodes;
}
