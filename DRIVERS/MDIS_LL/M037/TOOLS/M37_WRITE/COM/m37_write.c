/****************************************************************************
 ************                                                    ************
 ************                    M37_WRITE                       ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ls
 *        $Date: 2010/04/23 14:06:36 $
 *    $Revision: 1.4 $
 *
 *  Description: Configure and write one value to one M37 channel
 *
 *     Required: libraries: mdis_api, usr_oss, usr_utl
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
 
#include <stdio.h>
#include <stdlib.h>

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_api.h>
#include <MEN/m37_drv.h>

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void usage(void);
static void PrintError(char *info);

/********************************* usage ************************************
 *
 *  Description: Print program usage
 *			   
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void usage(void)
{
	printf("Usage: m37_write [<opts>] <device> <value> [<opts>]\n");
	printf("Function: Configure and write to M37 channel\n");
	printf("Options:\n");
	printf("    device       device name                 [none]\n");
	printf("    -v=<Volts>   Voltage output              [0]\n");
	printf("    -c=<chan>    channel number (0..3)       [0]\n");
	printf("    -l           loop mode                   [no]\n");
	printf("\n");
	printf("(c) 1999 by MEN mikro elektronik GmbH\n\n");
}

/********************************* main *************************************
 *
 *  Description: Program main function
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc,argv	argument counter, data ..
 *  Output.....: return	    success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int main(int argc, char *argv[])
{
	MDIS_PATH	path=0;
	int32	chan,loopmode,n;
	int32   value;
	char	*device,*str,*errstr,buf[40];
	double	volt, calc;
	
	/*--------------------+
    |  check arguments    |
    +--------------------*/
	if ((errstr = UTL_ILLIOPT("v=c=l?", buf))) {	/* check args */
		printf("*** %s\n", errstr);
		return(1);
	}

	if (UTL_TSTOPT("?")) {						/* help requested ? */
		usage();
		return(1);
	}

	/*--------------------+
    |  get arguments      |
    +--------------------*/
	for (device=NULL, n=1; n<argc; n++)
		if (*argv[n] != '-') {
			device = argv[n];
			break;
		}
		
	if (!device) {
		usage();
		return(1);
	}

    volt     = ((str = UTL_TSTOPT("v=")) ? atof(str) : 0);
	chan     = ((str = UTL_TSTOPT("c=")) ? atoi(str) : 0);
	loopmode = (UTL_TSTOPT("l") ? 1 : 0);

	/*--------------------+
    |  calculate value    |
    +--------------------*/
	if ((volt >= 10) || (volt < -10))  {
		printf("illegal value %f",volt);
		return (1);
	}

	calc = volt*(0xffff/20.0);
	if (calc >= 0)
		calc += 0.5;
	else
		calc -= 0.5;
    value = (int32)(calc);

	/*--------------------+
    |  open path          |
    +--------------------*/
	if ((path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}

	/*--------------------+
    |  config             |
    +--------------------*/
	/* set current channel */
	if ((M_setstat(path, M_MK_CH_CURRENT, chan)) < 0) {
		PrintError("setstat M_MK_CH_CURRENT");
		goto abort;
	}
    /*--------------------+
    |  print info         |
    +--------------------*/
	printf("channel number      : %ld\n",chan);

	/*--------------------+
    |  write              |
    +--------------------*/
	do {
		if ((M_write(path,value)) < 0) {
			PrintError("write");
			goto abort;
		}
		printf("write: 0x%02lx = %s\n", value, UTL_Bindump(value,16,buf));

		UOS_Delay(100);
	} while(loopmode && UOS_KeyPressed() == -1);

	/*--------------------+
    |  cleanup            |
    +--------------------*/
	abort:
	if (M_close(path) < 0)
		PrintError("close");

	return(0);
}

/********************************* PrintError ********************************
 *
 *  Description: Print MDIS error message
 *			   
 *---------------------------------------------------------------------------
 *  Input......: info	info string
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/

static void PrintError(char *info)
{
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}

