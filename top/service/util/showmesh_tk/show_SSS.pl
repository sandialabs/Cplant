#!/usr/bin/perl -w
use strict;

# Showmesh for perl/Tk -- this version runs on
# an SSS node and tries to rsh to service and
# compute nodes

# need Tk extension to perl where it runs

use Tk;
use Tk qw/ :eventtypes /;
use Pane;

my @sus;
my $service = $ENV{SERVICE_NODE} || 'c-7.su-0';	# Service node to use
my $total_nodes	= '?';		# Display values
my $total_busy	= '?';
my $total_free	= '?';

my $debug	= 1;		# Controls how much debugging info is output



#
# This hash is used both to generate the key description and
# to draw the node status reports.  Add states as necessary
# and they will be displayed in the key window, too.
#
my %states = (
	unknown		=> {
		-text		=> 'Node status is unknown',
		-color		=> 'slategray',
	},
	good		=> {
		-text		=> 'Node status OK',
		-color		=> 'green',
	},
	unavailable	=> {
		-text		=> 'Node is unavailable',
		-color		=> 'red',
	},
	stale		=> {
		-text		=> 'Node data is stale',
		-color		=> 'black',
	},
	busy		=> {
		-text		=> 'Node is busy',
		-color		=> 'blue',
	},
	pending		=> {
		-text		=> 'Node status is pending',
		-color		=> 'yellow',
	}
);
	
	



# main window
my $mw = new MainWindow(
	-title		=> 'Showmesh',
);

# frame for buttons
my $buttons = $mw->Frame(
	-relief		=> 'groove',
	-borderwidth	=> 2,
)->pack(
	-fill		=> 'both',
);

#
# Make the first pull down menu of functions to be performed
#
{
	my $menu = $buttons->Menubutton(
		-text		=> "pingd",
		-relief		=> "raised",
		-tearoff	=> 0,
	)->pack(
		-side		=> 'left',
		-anchor		=> 'n',
	);

	$menu->command(
		-label		=> 'pingd',
		-command	=> mk_cmd( \&do_pingd ),
	);
	$menu->command(
		-label		=> 'Display key',
		-command	=> \&do_key,
	);
	$menu->command(
		-label		=> 'Close window',
		-command	=> sub { $mw->destroy },
	);
}

#
# Make the operations menu
#
{
	my $menu = $buttons->Menubutton(
		-text		=> "Operations",
		-relief		=> "raised",
		-tearoff	=> 0,
	)->pack(
		-side		=> 'left',
		-anchor		=> 'n',
	);

	$menu->command(
		-label		=> 'Select all',
		-command	=> sub {
			for my $su (@sus) {
				next unless $su;
				for my $node (@{$su->{nodes}}) {
					$node->{selected} = 1;
				}
			}
		},
	);
	$menu->command(
		-label		=> 'Unselect all',
		-command	=> sub {
			for my $su (@sus) {
				next unless $su;
				for my $node (@{$su->{nodes}}) {
					$node->{selected} = 0;
				}
			}
		},
	);

	$menu->separator;
		
	$menu->command(
		-label		=> 'Node info',
		-command	=> mk_cmd( \&selected,
			mk_rsh( "cat /var/node_info" ) ),
	);
	$menu->command(
		-label		=> 'System log',
		-command	=> mk_cmd( \&selected,
			mk_rsh( "cat /var/log/messages" ) ),
	);
	$menu->command(
		-label		=> 'cplant log',
		-command	=> mk_cmd( \&selected,
			mk_rsh( "cat /var/log/cplant" ) ),
	);
	$menu->command(
		-label		=> 'ptlDebug log',
		-command	=> mk_cmd( \&selected,
			mk_rsh( "/cplant/sbin/ptlDebug" ) ),
	);

	$menu->command(
		-label		=> 'Mark as bad',
		-command	=> mk_cmd( \&selected,
		    sub {
			my $node = shift;
			die <<"" unless $service;
Please select a service node by right clicking
before attempting to mark a node as bad

			print "Marking $node->{name} ($node->{physical})\n";
			`rsh $service echo yes \\| /cplant/sbin/pingd -gone $node->{physical}`;
		    },
		),
	);

	$menu->command(
		-label		=> 'Start PCT',
		-command	=> mk_cmd( \&selected,
			mk_rsh( 'killall pct \\; nohup /cplant/sbin/pct >/dev/null 2>&1 &' ) ),
	);
}


#
# Help menu
#
{
	my $menu = $buttons->Menubutton(
		-text		=> 'Help',
		-relief		=> 'raised',
		-tearoff	=> 0,
	)->pack(
		-side		=> 'left',
		-anchor		=> 'n',
	);

	$menu->command(
		-label		=> 'Help',
		-command	=> mk_cmd( \&do_help ),
	);
	$menu->checkbutton(
		-label		=> 'Debug pingd',
		-variable	=> \$debug,
	);
}


# A service node entry widget
$buttons->Entry(
	-width		=> 15,
	-textvariable	=> \$service,
)->pack(
	-side		=> 'right',
	-anchor		=> 'n',
);

$buttons->Label(
	-text		=> 'Service Node: ',
)->pack(
	-side		=> 'right',
	-anchor		=> 'n',
);


# Scrolled frame for displaying the list of SU's
my $su_frame = $mw->Scrolled(
	'Pane',
	-width		=> 640,
	-height		=> 200,
	-relief		=> 'groove',
	-borderwidth	=> 2,
	-scrollbars	=> 'soe',
	-sticky		=> 'we',
	-gridded	=> 'y',
)->pack(
	-expand		=> 'both',
	-fill		=> 'both',
	-side		=> 'top',
);


# a frame and canvas in which to draw the nodes
my $node_frame = $mw->Scrolled(
	'Pane',
	-width		=> 640,
	-height		=> 300,
	-relief		=> 'groove',
	-borderwidth	=> 2,
	-scrollbars	=> 'soe',
	-sticky		=> 'we',
	-gridded	=> 'y',
)->pack(
	-fill		=> 'both',
	-expand		=> 'both',
	-side		=> 'top',
);


# frame for totals
my $frame_tots = $mw->Frame(
	-relief		=> 'groove',
	-borderwidth	=> 2,
)->pack(
	-fill		=> 'both',
);


#
# Make the status display at the bottom of the window.
# These are variables are tied to the label -- any changes
# in the values will be immediately updated on the display
#
for(
	['Total:',	\$total_nodes],
	['Total busy:',	\$total_busy],
	['Total free:',	\$total_free],
) {
	my ($name,$var_ref) = @$_;
	$frame_tots->Label(
		-text		=> $name,
		-width		=> 15,
	)->pack(
		-side		=> 'left',
		-anchor		=> 'n',
	);

	$frame_tots->Label(
		-textvariable	=> $var_ref,
		-width		=> 5,
	)->pack(
		-side		=> 'left',
		-anchor		=> 'n',
	);
}

#show_sus();
#do_erase();
do_pingd();
MainLoop;

sub do_pingd
{

	die <<"" unless $service;
The service node is not set.  Choose a service
node by right clicking on the desired node.

	warn "rsh to $service to run pingd:  This may take a while\n";
	local $_  = `rsh $service /cplant/sbin/pingd 2>&1 &`;

	# This doesn't do anything -- the script will block until the
	# rsh returns.  We should use popen to do this instead.
	for (1..8) {
		sleep(1);
		last unless $_;
	}

	print "pingd: $_" if $debug;

	die "rsh to $service timed out\n" unless $_;

  	# look for problems
	if(	(/Can not register myself as PPID=/m)		||
		(/can't locate bebopd registry file name/m)	||
		(/Sorry, unable to locate bebopd/m)		||
		(/No reponse with data from bebopd, sorry/m)	||
		(/Unknown host/m)				||
		(/Permission denied/m)
	) {
		die "Internal error: $_\n";
	}

	# for each line w/ "SU" add entry to list of SUs and list
	# of nodes -- also, add status tag to node. see below or
	# do_key() for meaning of the status tags
	#
	# Need documentation on what the input line should look like
	# This could be done more easily with regular expressions and
	# much more simply.

	for(split /\n/) {
		chomp;
		print "pingd: Line returned: $_\n" if $debug;

		if ( /SU/ ) {
			my ($physical,$su_num,$node_num) = /
				#
				# Output from pingd should be of the form:
				# something SU-ddd n-ddd status_message	
				#
				(\d+)		# Physical node id
				.*		#
				SU-(\d+)	# SU number
				.*		# Separator
				n-(\d+)		# Node number
			/x or do {
				warn "Unable to parse SU or node number: $_\n";
				next;
			};

			for( $su_num, $node_num, $physical ){ $_ = int $_ }

			print "c-$node_num.su-$su_num has reported\n";

			my $su = $sus[$su_num] ||= {
				name		=> "SU-".int($su_num),
				button		=> undef,
				state		=> 'unknown',
				label		=> undef,
				selected	=> 0,
				nodes		=> [],
			};

			my $node = $su->{nodes}->[$node_num] ||= {
				name		=> "c-".int($node_num).".$su->{name}",
				physical	=> $physical,
				state		=> 'good',	# We hope!
				data		=> '',
				selected	=> 0,
				button		=> undef,
			};

			$su->{state}	= 'good';
			$node->{state}	= 'good';	# Default
			$node->{data}	= $_;

			# if there is more stuff after the node string
			# assume it's # the "is busy" data; if this isn't
			# correct, the correct status will be determined
			# below...

			#$node->{state}	= 'bad'		if split > 8;
			$node->{state}	= 'stale'	if /is stale/;
			$node->{state}	= 'alloc'	if /allocation/;
			$node->{state}	= 'unavailable'	if /is unavailable/;
			print "state=$node->{state}\n";
    		}

		# Update the display -- these are tied to the display labels
		$total_nodes	= $1	if /Total:\s*(\d*)/;
		$total_busy	= $1	if /Total busy:\s*(\d*)/;
		$total_free	= $1	if /Total free:\s*(\d*)/;
	}

	draw_sus();
}


sub draw_sus
{
	my $su_num = 0;
	for my $su (@sus) {
		next unless $su;

		unless( $su->{frame} ) {
			print "Creating frame for $su->{name} ($su_num)\n"
				if $debug;

			$su->{frame} = $node_frame->Frame(
				-relief		=> 'groove',
				-borderwidth	=> 2,
			)->grid(
				-row		=> 0,
				-column		=> $su_num,
			);

			$su->{frame}->Label(
				-text		=> $su->{name},
				-width		=> 12,
			)->pack(
				-side		=> 'top',
			)->bind(
				'<ButtonPress-1>',
				sub {
					$su->{selected} = !$su->{selected};
					for my $node (@{$su->{nodes}}) {
						next unless $node;
						$node->{selected} = $su->{selected};
					}
				}
			);
		}

		my $frame = $su->{frame};

		my $node_num = 0;
		for my $node (@{$su->{nodes}}) {
			next unless $node;

			my $state = $states{$node->{state}};
			unless( $node->{button} ) {
				print "Creating button for $node->{name}\n"
					if $debug;

				$node->{button} = $frame->Checkbutton(
					-width		=> 2,
					-text		=> sprintf( "%02d", $node_num ),
					-variable	=> \$node->{selected},
				)->pack(
					-side		=> 'top',
				);

				$node->{button}->bind(
					'<ButtonPress-3>',
					sub { $service = $node->{name} },
				);
			}

			$node->{button}->configure(
				-background	=> $state->{-color},
			)
		} continue { $node_num++ }
	} continue { $su_num++ }
}

my $kw;
sub do_key 
{
	if( defined $kw ) {
		$kw->deiconify;
		$kw->raise;
		return;
	}

	$kw = $mw->Toplevel(
		-title		=> 'Showmesh key',
	);

	$kw->Button(
		-text		=> "Close",
		-command	=> sub { $kw->withdraw },
	)->pack(
		-side		=> 'top',
		-anchor		=> 'w',
	);
    
    	my $frame = $kw->Frame->pack(
		-side		=> 'top',
		-fill		=> 'both',
		-expand		=> 'both',
	);

	for my $state (sort keys %states) {
		my ($text,$color) = @{$states{$state}}{-text,-color};
		$frame->Button(
			-text		=> '',
			-background	=> $color,
		)->grid(
			$frame->Label(
				-textvariable	=> \$text,
			)
		);
	}
}

#
# Makes a menu command that will pop-up a message box on errors
#
sub mk_cmd
{
	my $callback = shift;
	my @args = @_;
	sub { tk_error( $callback, @args ) };
}

sub tk_error
{
	my $callback = shift;
	eval { $callback->( @_ ) };

	return unless $@;
	my $error_message = "$@";	# Copy!

	my $error = new MainWindow(
		-title		=> 'Error!',
	);

	$error->Label(
		-textvariable	=> \$error_message,
	)->pack(
		-side		=> 'top',
		-expand		=> 'both',
	);

	$error->Button(
		-text		=> 'Ok',
		-command	=> sub { $error->destroy },
	)->pack(
		-side		=> 'top',
	);

	print "callback returned: $error_message\n" if $debug;
	return;
}
		
sub do_help
{
	die <<END_OF_HELP;
To get started w/ Showmesh, set the service node by
first selecting an SU, and then selecting a service node.
This should allow one to run pingd...

If pingd is successful, then one may use the "ops" menu
in conjunction with single or multiple node selection to
perform a variety of operations on desired nodes...

To change the service node, choose "reset" from the
"pingd" menu and proceed as in starting up.
END_OF_HELP

}

sub mk_rsh
{
	my @args = @_;
	sub {
		my $node = shift;
		print "$node->{name}: rsh $node->{name} @args\n" if $debug;
		my $rc = `rsh $node->{name} @args`;
	};
}

sub selected
{
	my $callback = shift;
	my $mw = new MainWindow(
		-title	=> 'Processing selected nodes',
	);
	my $value = 'Processing output:';

	$mw->Scrolled(
		'Pane',
		-scrollbars	=> 'se',
	)->pack(
		-side		=> 'top',
		-expand		=> 'both',
		-fill		=> 'both',
	)->Label(
		-textvariable	=> \$value,
		-anchor		=> 'w',
		-justify	=> 'left',
	)->pack(
		-side		=> 'top',
		-expand		=> 'both',
		-fill		=> 'both',
	);
	
	for my $su (@sus) {
		next unless $su;
		for my $node (@{$su->{nodes}}) {
			next unless $node->{selected};
			my $rc = $callback->( $node );
			print "$node->{name}: $rc\n" if $debug;
			$value .= "\n$node->{name}: $rc\n";
			Tk::DoOneEvent( ALL_EVENTS );
		}
	}

	$mw->Button(
		-text		=> 'Dismiss',
		-command	=> sub { $mw->destroy },
	)->pack(
		-side		=> 'bottom',
	);
}

