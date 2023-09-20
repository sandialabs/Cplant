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
** $Id: myrinet_routes2cplant_routes.c,v 1.1.2.1 2002/10/02 16:37:58 jrstear Exp $
** This is a program that converts a route file generated by the GM mapper
** or the Myricom off-line tools into several file that the Sandia MCP
** can understand. The Myricom route file contains all routes from all
** nodes in a single file. We use one file per node which contains only
** the routes from this node to all other node.
**
** This is a hack. It should be rewritten using flex and yacc. It is
** not specific to a topology or a particualr machine. However, it does
** assume the routes are no longer than 15 bytes (hops).
*/
#include <stdio.h>
#include <string.h>

static int line_num = 0;
int process_host(FILE *in_fp, int nodes, char *machine_name, int verbose);

int
main(int argc, char *argv[])
{

FILE *in_fp;
int verbose;
int rc;
int num_hosts, max_host_routes;
int hosts;
char *machine_name;


    verbose= 0;
    if (argc == 2)   {
	/* Use stdin */
	in_fp= stdin;
	machine_name= argv[1];
    } else if (argc == 3)   {
	/* Use input file */
	in_fp= fopen(argv[1], "r");
	if (in_fp == NULL)   {
	    fprintf(stderr, "Input file \"%s\": ", argv[1]);
	    perror("");
	    exit(-1);
	}
	machine_name= argv[2];
    } else   {
	fprintf(stderr, "Usage: %s [in_file] machine_name\n",
	    argv[0]);
	fprintf(stderr, "    If no infile is specified, stdin is assumed\n");
	fprintf(stderr, "    machine_name is alaska, siberia, etc\n");
	fprintf(stderr, "    If machine_name == noname, nothing is appended\n");
	exit(-1);
    }

    rc= fscanf(in_fp, "%d %d\n", &num_hosts, &max_host_routes);
    line_num = 0;
    if (rc != 2)   {
	if (rc == EOF)   {
	    fprintf(stderr, "ERROR %s: Reached early EOF in input file\n",
		argv[0]);
	    exit(-1);
	} else   {
	    fprintf(stderr, "ERROR %s: Syntax error on line 1\n",
		argv[0]);
	    exit(-1);
	}
    }
    if (max_host_routes != 1)   {
	fprintf(stderr, "ERROR %s: Cannot handle more than 1 route per host\n",
	    argv[0]);
	exit(-1);
    }

    if (verbose)   {
	fprintf(stderr, "    Route file contains %d hosts, with %d route(s) per"
	    " host\n", num_hosts, max_host_routes);
    }

    for (hosts= 0; hosts < num_hosts; hosts++)   {
	if (process_host(in_fp, num_hosts, machine_name, verbose) != 0)   {
	    fprintf(stderr, "ERROR %s: Syntax errors in the input file\n",
		argv[0]);
	    exit(-1);
	}
    }
}  /* end of main() */


int
process_host(FILE *in_fp, int nodes, char *machine_name, int verbose)
{

FILE *out_fp;
int rc;
int num_routes;
int route_len;
char name[256];
char fname[256];
int len;
int i;


    rc= fscanf(in_fp, "\"%s (%d %d)\n", name, &num_routes, &route_len);
    ++line_num;
    len= strlen(name);
    len--;
    if (name[len] != '\"')   {
	fprintf(stderr, "process_host(): last name char is not \", it is %c\n",
	    name[len]);
	fprintf(stderr, "%s\n", name);
	return 1;
    }
    name[len]= '\0';
    if (rc != 3)   {
	if (rc == EOF)   {
	    fprintf(stderr, "Early EOF\n");
	} else   {
	    fprintf(stderr, "Syntax error, rc is %d\n", rc);
	}
	return 1;
    }
    if (verbose)   {
	fprintf(stderr, "Beginning new section\n");
	fprintf(stderr, "    name \"%s\"\n", name);
	fprintf(stderr, "    num routes %d\n", num_routes);
	fprintf(stderr, "    route len %d\n", route_len);
    }

    if (strcmp(machine_name,"noname"))
      sprintf(fname, "%s.SM-0.%s", name, machine_name);
    else
      sprintf(fname, "%s", name);
    out_fp= fopen(fname, "w");
    if (out_fp == NULL)   {
	fprintf(stderr, "Could not open output file \"%s\"\n", fname);
	return 1;
    }

    /* Now we should see num_routes lines */
    for (i= 0; i < num_routes; i++)   {
	if (process_line(in_fp, out_fp) != 0)   {
	    fclose(out_fp);
	    return 1;
	}
    }

    fclose(out_fp);
    return 0;

}  /* end of process_host() */

int
process_line(FILE *in_fp, FILE *out_fp)
{

int rc;
int num_bytes;
int len;
int i;
char name[256];
int route[32];


    num_bytes= fscanf(in_fp, "\"%s 
        %d,%d,%d,%d,%d,%d,%d,%d,
        %d,%d,%d,%d,%d,%d,%d,%d,
        %d,%d,%d,%d,%d,%d,%d,%d,
        %d,%d,%d,%d,%d,%d,%d,%d\n",
	name, &route[0], &route[1], &route[2], &route[3], &route[4], &route[5],
	&route[6], &route[7], &route[8], &route[9], &route[10], &route[11],
	&route[12], &route[13], &route[14], &route[15], &route[16], 
	&route[17], &route[18], &route[19], &route[20], &route[21], 
	&route[22], &route[23], &route[24], &route[25], &route[26], 
	&route[27], &route[28], &route[29], &route[30], &route[31]);
    ++line_num;
    len= strlen(name);
    len--;
    if (name[len] != '\"')   {
	fprintf(stderr, "process_line(): last name char is not \", it is %c\n",
	    name[len]);
	fprintf(stderr, "line %d: %s\n", line_num, name);
	return 1;
    }
    name[len]= '\0';
    if (num_bytes == EOF)   {
	fprintf(stderr, "Early EOF\n");
	return 1;
    }
    rc= fscanf(in_fp, "\n");

    num_bytes--;
    for (i= 0; i < num_bytes; i++)   {
	if (route[i] >= 0)   {
	    fprintf(out_fp, "0x%2x ", route[i] + 0x80);
	} else   {
	    fprintf(out_fp, "0x%2x ", 0xc0 + route[i]);
	}
    }
    fprintf(out_fp, "\n");

    return 0;
}  /* end of process_line() */