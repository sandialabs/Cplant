/*************************************************************************
Cplant Release Version 2.0.1.10
Release Date: Nov 5, 2002 
#############################################################################
#
#     This Cplant(TM) source code is the property of Sandia National
#     Laboratories.
#
#     This Cplant(TM) source code is copyrighted by Sandia National
#     Laboratories.
#
#     The redistribution of this Cplant(TM) source code is subject to the
#     terms of the GNU Lesser General Public License
#     (see cit/LGPL or http://www.gnu.org/licenses/lgpl.html)
#
#     Cplant(TM) Copyright 1998, 1999, 2000, 2001, 2002 Sandia Corporation. 
#     Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
#     license for use of this work by or on behalf of the US Government.
#     Export of this program may require a license from the United States
#     Government.
#
#############################################################################
**************************************************************************/
/*
** $Id: power.c,v 1.3 1998/02/10 23:20:49 dwvandr Exp $
**
**   Power.c - Serial line control program for the EECI AR-16 relay
** controller on a serial device.
**
**   The AR-16 is a relay controller card that can control 16 relays via a
** serial device.   Optional hardware allows multiple AR-16s (up to 8) to be
** chained onto the same device for a maximum of 128 relays on a single
** device.
**
**   The command interface is simple: a byte is sent out the device;
** The upper 7 bits addresses the AR-16 and relay, and the low bit turns the
** relay on or off.   Each AR-16 on the line must be configured to accept a
** unique range of 16 addresses.
**
**   On the first box sent by EECI (now controlling the machine in the
** Puma trailer), a command with the low bit set powers the corresponding
** outlet OFF but turns the led on the case ON.   The production boxes are
** wired such that a 1 bit turns the power and the led on.   The def
** REVERSE_CMD_BITS defines which protocol to use.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <termios.h>

/*
**   If not using a makefile or command line that defines one of these,
** choose a working envoronment for the executable by uncommenting one of
** the lines below.   One of these must be defined.
*/
/* #define PUMA_TRAILER */
/* #define CPLANT_1 */

/*
**   The power control box we have in the puma trailer is wired backwards - 
** a command with the low bit set powers the outlet off instead of on, so
** we need to have a special image for that environment.   The box and
** image use a different baud rate so the executable won't work on the
** production machines.
*/
#ifdef PUMA_TRAILER
    #define INVERT_CMD_BIT
    #define DEVICE "/dev/ttyS0"

    /* Baud rates defined in termios.h, or in a file included therein. */
    #define BAUD_RATE B19200

    /* DELAY is the default delay time (in seconds) between commands.  */
    #define DELAY .2
#endif

/*
**   CPLANT_1 is the environment for the production machines in building
** 880.   The power control boxes run at 600 baud on device ttyS1 and are
** wired so that a command bit of 1 turns the corresponding outlet on.
*/
#ifdef CPLANT_1
    #undef INVERT_CMD_BIT
    #define DEVICE "/dev/ttyS1"

    /* Baud rates defined in termios.h, or in a file included therein. */
    #define BAUD_RATE B600

    /* DELAY is the default delay time (in seconds) between commands.  */
    #define DELAY .2
#endif


/* These defs determine what is sent to turn an outlet on. */
#ifdef INVERT_CMD_BIT
    #define CMD_ON 0x00
    #define CMD_OFF 0x01
#else
    #define CMD_ON 0x01
    #define CMD_OFF 0x00
#endif /* INVERT_CMD_BIT */

#define CMD_LIST_LEN 1024

/*     
**    ADDR_START and ADDR_END defines the range of addresses allowed on the
** command line.   Note that addresses on the command line are considered
** one based and adjusted (decremented) before use.
*/
#define ADDR_START 1
#define ADDR_END 16

#define TRUE 	(1==1)
#define FALSE 	(1!=1)

#define on_off_str( cmd_code ) ((cmd_code) == CMD_ON ? "on" : "off")
#define fsleep( fdelay ) (usleep( (int)((fdelay) * 1000000)))

#ifndef DEVICE
    #error DEVICE not defined.   Did you choose an environment?
#endif

typedef struct {
    unsigned short addr;	/* Relay address - 0 to 127 */
    unsigned short cmd;		/* Relay command - on or off */
} Command;

int Verbose;
int Very_Verbose;
int Test_only;

void usage( char * );
void parse_cmd_line( char**, int, Command *, int * );
int baud_val( int baud_code );


main( int argc, char **argv )
{
    int i;
    int dev_fd;
    unsigned char cmd;
    unsigned int relay;
    double delay = DELAY;
    char *device = DEVICE;
    char err_str[256];

    char c, *optstr;
    extern char *optarg;
    extern int optind, opterr;
    Command cmd_list[CMD_LIST_LEN]; 
    int num_cmds;

    Verbose = FALSE;
    Very_Verbose = FALSE;
    Test_only = FALSE;


    /*
    **   Process the command line.
    */
    while ((c = getopt(argc, argv, "D:d:hvVT")) != -1) {
	switch (c) {
	  case 'D':
	      device = optarg;
	      break;

	  case 'd':
	      delay = strtod( optarg, NULL );
	      break;

	  case 'v':
	      Verbose = TRUE;
	      break;

	  case 'V':
	      Verbose = TRUE;
	      Very_Verbose = TRUE;
	      break;

	  case 'T':
	      Test_only = TRUE;
	      Verbose = TRUE;
	      Very_Verbose = TRUE;
	      break;

	  case 'h':
	      usage( argv[0] );

	  default:
	      usage( argv[0] );
	}
    }

    if( Verbose ) {
	if( Test_only ) {
	    printf( 
"*** TEST ONLY - The port will be opened, configured, and closed, but no\n"
"    writes will be attempted. ***\n" );
	}
	printf( "Using device %s, baud rate %d, delay %4.1f seconds.\n",
	    device, baud_val( BAUD_RATE ), delay );
	printf( "Legal addresses must be in the range %d to %d\n",
	    ADDR_START, ADDR_END );
	    
    }

    if( argv[optind] == NULL || argv[optind+1] == NULL ) {
	printf( "No commands found.  Use %s -h for usage info.\n", argv[0] );
	exit( 1 );
    }

    parse_cmd_line( argv, optind, cmd_list, &num_cmds );

    /*
    **   Open the serial device.
    */
    if( (dev_fd = open( device, O_RDWR | O_NOCTTY | O_NDELAY )) == -1 ) {
	sprintf( err_str, "open( %s )", device );
	perror( err_str );
	exit( 1 );
    }

    /*
    **   Set up the serial device.   Function exits on error.
    */
    set_serial( dev_fd );

    /*
    **   Delay before issuing first command.
    */
    fsleep( delay );

    for( i = 0; i < num_cmds; i++ ) {

		/*
		**   Build and write out the command.
		*/
		cmd = cmd_list[i].cmd | cmd_list[i].addr << 1;

		if( Very_Verbose ) {
		    printf( "Sending command %d = 0x%x\n", i, cmd );
		}

		if( !Test_only ) {
		    if( write( dev_fd, &cmd, 1 ) != 1 ) {
			    perror( "write()" );
			    fprintf( stderr, "Couldn't write to device.\n" );
			    exit( 1 );
		    }
		}

		/*
		**   Delay before issuing next command.
		*/
		fsleep( delay );
    }

    close( dev_fd );
}

/*
**    Set_serial sets up the serial device to match what's expected on the
** other end.
*/
int
set_serial( int dev_fd )
{
    struct serial_struct serial_info;
    struct termios options;

    /*
    **   Get the current device options.
    */
    if( tcgetattr( dev_fd, &options ) < 0 ) {
        perror( "tcgetattr()" );
        exit( 1 );
    }


    /*
    **   Reset device options that (may) need changing.
    */
    cfsetospeed(&options, BAUD_RATE); /*  Select output baud rate   */

    options.c_cflag &= ~CSIZE;    /* Mask the character size bits */
    options.c_cflag |= CS8;       /* Select 8 data bits */

    options.c_cflag |= CSTOPB;    /* One stop bit */
 
    options.c_cflag &= ~CREAD;    /* Disable receiver */
 
    options.c_cflag &= ~PARENB;   /* No parity */
    options.c_cflag |= PARODD;    /* Odd parity */
 
    options.c_oflag &= ~OPOST;    /* Raw output */


    /*
    **   Send the new options to the device.
    */
    if( tcsetattr( dev_fd, TCSANOW, &options ) < 0 ) {
        perror( "tcsetattr()" );
        exit( 1 );
    }
}


/*
**   Parse_range() - parses a string for a valid integer range.
** Ranges are specified as numbers or as a pair of number separated by '-'.
** The function returns the start and end values of the range in reference
** variables.   The string is also passed by reference so that the function
** can update it to point to the next element in the range.
*/
int
parse_range( char *range_str, char **range_next_str, int *start, int *end )
{
    char *end_str;
    int tmp;


    if( strcasecmp( range_str, "all" ) == 0 ) {
	*start = ADDR_START;
	*end = ADDR_END;
	*range_next_str = range_str + 3;
        return( 1 );
    }

    *start = *end = strtol( range_str, &end_str, 0 );
    if( end_str == range_str  ||  end_str == NULL ) {
        return( -1 );
    }

    if( *end < ADDR_START  ||  *end > ADDR_END  ||
        *start < ADDR_START  ||  *start > ADDR_END ) {
	return( -1 );
    }

    if( end_str[0] == '-' ) {
	range_str = end_str + 1;
        *end = strtol( range_str, &end_str, 0 );
	if( end_str == range_str  ||  end_str == NULL ) {
	    return( -1 );
	}
    }

    if( end_str[0]== ','  &&  end_str[1] != '\0' ) {
	*range_next_str = end_str + 1;
    } else {
	*range_next_str = end_str;
    }


    return( 1 );
}

void 
parse_cmd_line( char **arg_list, int arg_index,
    Command *cmd_list, int *cmd_list_len )
{
    /*
    **    The format of the command line is a series of space separated
    ** range-command pairs.   
    **
    **    Ranges may consist of a single number, a comma separated
    ** list of numbers, a pair of numbers separated by '-', or a comma
    ** separated list of number pairs separated by '-'.
    **
    **    Commands should be "on" or "off", case is ignored.
    **
    **    Any incorrect or incomplete range-command pairs will cause the
    ** program to exit, as will an overflow of the command list.
    */
    unsigned short cmd;
    int list_index;
    char *range_arg, *cmd_arg, *range_next;
    int start, end, incr;

    if( Very_Verbose ) {
	printf( "Command list:\n" );
    }

    list_index = 0;
    while( arg_list[arg_index] != NULL ) {
	range_arg = arg_list[arg_index];
	cmd_arg = arg_list[arg_index+1];

	if( cmd_arg == NULL ) {
	    fprintf( stderr, "%s: Missing command\n", arg_list[0] );
	    usage( arg_list[0] );
	}

	if( strcasecmp( "on", cmd_arg ) == 0 ) {
	    cmd = CMD_ON;
	} else if( strcasecmp( "off", cmd_arg ) == 0 ) {
	    cmd = CMD_OFF;
	} else {
	    fprintf( stderr, "%s: Invalid command %s.\n",
		arg_list[0], cmd_arg );
	    usage( arg_list[0] );
	}

        while( *range_arg != '\0' ) {
	    /*
	    **   Get the range string.   Parse_range() returns the address
	    ** of the next character in the range string to allow repeat
	    ** calls on the same string.
	    */
	    if( parse_range( range_arg, &range_next, &start, &end ) == -1 ) {
	        fprintf( stderr, "%s: Bad address or range.\n", arg_list[0] );
		usage( arg_list[0] );
	    }
	    range_arg = range_next;

	    /*
	    **   Create the command list entries for the range.
	    */
            if( start <= end ) {
	        incr = 1; 
	    } else {
	        incr = -1;
	    }
	    while( 1 ) {
		/*  Set address, convert physical to logical address.  */
		cmd_list[list_index].addr = start - 1;
		cmd_list[list_index].cmd = cmd;
		list_index++;
		if( list_index > CMD_LIST_LEN ) {
		    fprintf( stderr, 
	    "parse_cmd_line(): Command list overflow, too many commands.\n" );
		    exit( 1 );
		}

		if( Very_Verbose ) {
		    printf( "%d: Load %d %s\n", list_index, start, 
			on_off_str( cmd ) );
		}

		if( start == end ) break;
		start += incr;
	    }
	}

	arg_index += 2;
    }

    *cmd_list_len = list_index;
}

int 
baud_val( int baud_code )
{
    int bval;

    switch( baud_code ) {
	case B0: bval = 0;
	    break;
	case B50: bval = 50;
	    break;
	case B75: bval = 75;
	    break;
	case B110: bval = 110;
	    break;
	case B134: bval = 134;
	    break;
	case B150: bval = 150;
	    break;
	case B200: bval = 200;
	    break;
	case B300: bval = 300;
	    break;
	case B600: bval = 600;
	    break;
	case B1200: bval = 1200;
	    break;
	case B1800: bval = 1800;
	    break;
	case B2400: bval = 2400;
	    break;
	case B4800: bval = 4800;
	    break;
	case B9600: bval = 9600;
	    break;
	case B19200: bval = 19200;
	    break;
	case B38400: bval = 38400;
	    break;
	case B57600: bval = 57600;
	    break;
	case B115200: bval = 115200;
	    break;
	case B230400: bval = 230400;
	    break;
	case B460800: bval = 460800;
	    break;
	default: 
	    fprintf( stderr, "Unknown baud code %x\n", baud_code );
	    bval = -1;
	    break;
    }

    return bval;
}

void
usage( char *name )
{
    fprintf( stderr, "Usage:\n", name );
    fprintf( stderr, 
"%s [-v] [-V] [-T] [-D device] [-d delay] list cmd [list cmd...]\n", name );
    fprintf( stderr, 
"    <List> specifies a list of addresses to which to apply <cmd>.   The\n"
"list format may be comma separated numbers such as \"1,2,3,4\", a range such\n"
"as \"1-4\", or a comma separated combination of numbers and ranges.\n"
"Note that addresses are one based, so 1 refers to the first outlet on the\n"
"power controller.   Addresses must be in the range %d to %d.\n",
	ADDR_START, ADDR_END );

    fprintf( stderr,
"\n    <Cmd> may be either \"on\" or \"off\".\n"
"\n    <Device> should specify the full path to the device file.\n"
"\n    <Delay> is the time in seconds (floating point) to wait between\n"
"sending commands to the power controller.   This may be used to avoid\n"
"powering up equipment too quickly.\n" 
"\n    -v - Verbose.  Prints diagnostic information.\n"
"\n    -V - Even more verbose.\n"
"\n    -T - Test only, does not send commands out port.  Sets -V.\n"
    );

    exit( 1 );
}
