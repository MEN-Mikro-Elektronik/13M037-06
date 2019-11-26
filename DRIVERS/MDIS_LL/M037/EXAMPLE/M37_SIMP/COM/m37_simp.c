/****************************************************************************
 ************                                                    ************
 ************                   M37_SIMP                         ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ls
 *
 *  Description: Simple example program for the M37 driver 
 *                      
 *     Required: libraries: mdis_api, usr_oss
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * Copyright 2010-2019, MEN Mikro Elektronik GmbH
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
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>
#include <MEN/usr_oss.h>
#include <MEN/m37_drv.h>


/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static int32 _m37_simp(char *device, int32 chan);
static void PrintError(char *info);

/********************************* main *************************************
 *
 *  Description: MAIN entry (not used in systems with one namespace)
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc, argv		command line arguments/counter
 *  Output.....: return			success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int main(int argc, char *argv[])
{
	if (argc < 3 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: m37_simp <device> <chan>\n");
		printf("Function: M37 simple example\n");
		printf("Options:\n");
		printf("    device       device name\n");
		printf("    chan         channel number (0..n)\n");
		printf("\n");
		return(1);
	}

    return(_m37_simp(argv[1], atoi(argv[2])));
}


/******************************* _m37_simp **********************************
 *
 *  Description: Example (directly called in systems with one namespace)
 *               Writes two values to one channel.
 *
 *---------------------------------------------------------------------------
 *  Input......: device    device name
 *               chan      channel number
 *  Output.....: return    success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
static int32 _m37_simp(char *device, int32 chan)
{
	MDIS_PATH path=0;
	int32 chNbr;

    printf("\n");
	printf("m37_simp - simple example program for the M37 module\n");
    printf("====================================================\n\n");

	/*--------------------+
    |  open path          |
    +--------------------*/
    printf("M_open(\" %s \")\n", device);
	if ((path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}

	/*--------------------+
    |  config             |
    +--------------------*/
	/* channel number */
	if ((M_setstat(path, M_MK_CH_CURRENT, chan)) < 0) {
		PrintError("setstat M_MK_CH_CURRENT");
		goto ABORT;
	}

	/* get number of channels */
	if ((M_getstat(path, M_LL_CH_NUMBER, &chNbr)) < 0) {
		PrintError("getstat M_LL_CH_NUMBER");
		goto ABORT;
	}

	/*--------------------+
    | print info          |
    +--------------------*/
	printf("channel number      : %ld\n",chan);
	printf("number of channels  : %ld\n",chNbr);

	/*--------------------+
    |  write              |
    +--------------------*/
	printf("set channel %ld to -10.0V\n",chan);
	if ((M_write(path,0x8000)) < 0) {
		PrintError("write");
		goto ABORT;
	}
	UOS_Delay(2000);	/* wait */ 

	printf("set channel %ld to +9.99..V\n",chan);
	if ((M_write(path,0x7fff)) < 0) {
		PrintError("write");
		goto ABORT;
	}
	UOS_Delay(2000);	/* wait */ 

	/*--------------------+
    |  cleanup            |
    +--------------------*/
ABORT:
    printf("M_close\n");
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

