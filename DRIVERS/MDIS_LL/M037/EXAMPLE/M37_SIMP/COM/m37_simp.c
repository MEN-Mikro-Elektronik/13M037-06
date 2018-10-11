/****************************************************************************
 ************                                                    ************
 ************                   M37_SIMP                         ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ls
 *        $Date: 2010/04/23 14:06:30 $
 *    $Revision: 1.5 $
 *
 *  Description: Simple example program for the M37 driver 
 *                      
 *     Required: libraries: mdis_api, usr_oss
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m37_simp.c,v $
 * Revision 1.5  2010/04/23 14:06:30  amorbach
 * R: Porting to MDIS5
 * M: changed according to MDIS Porting Guide 0.8
 *
 * Revision 1.4  1999/07/21 14:11:45  Franke
 * cosmetics
 *
 * Revision 1.3  1999/06/02 14:08:54  kp
 * include stdlib.h
 *
 * Revision 1.2  1999/05/12 11:22:30  Franke
 * cosmetics
 *
 * Revision 1.1  1999/05/11 14:32:01  Schoberl
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
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

