/****************************************************************************
 ************                                                    ************
 ************                    M37_BLKWRITE                    ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: ls
 *
 *  Description: Configure and write M37 output channels (blockwise)
 *
 *     Required: libraries: mdis_api, usr_oss, usr_utl
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

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_api.h>
#include <MEN/m37_drv.h>

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define CH_NUMBER			4			/* nr of device channels */
#define PERIOD_SIZE			22
/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void usage(void);
static void PrintError(char *info);
static void PrintUosError(char *info);
static void  __MAPILIB SigHandler(u_int32 sigCode);
/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static u_int32		G_sigHdlErr_lowWater = 0;
static u_int32		G_sigHdlErr_other    = 0;

/********************************* usage ************************************
 *
 *  Description: Print program usage
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc, argv		argument counter, data ...
 *  Output.....: return			success (0) or error (1)
 *  Globals....: G_sigHdlErr_lowWater, G_sigHdlErr_other
 ****************************************************************************/
static void usage(void)
{
	printf("Usage: m37_blkwrite [<opts>] <device> [<opts>]\n"               );
	printf("Function: Configure and write M37 channels\n"						);
	printf("Options:\n"														);
	printf("    device       device name .......................... [none]\n");
	printf("    -b=<mode>    block i/o mode	....................... [0]\n"	);
	printf("                  0 = M_BUF_USRCTRL (without -i option)\n");
	printf("                  2 = M_BUF_RINGBUF (whith -t and -i option)\n");
	printf("    -o=<msec>    block write timeout [msec] (0=none) .. [Default->Descriptor]\n");
	printf("    -d=<val>	 Volt output for channel 0 ............ [0]\n");			
	printf("    -e=<val>	 Volt output for channel 1 ............ [0]\n");			
	printf("    -f=<val>	 Volt output for channel 2 ............ [0]\n");			
	printf("    -g=<val>	 Volt output for channel 3 ............ [0]\n");			
	printf("    -s           generate waveform for enabled channels [no]\n");
	printf("    -t           extern trigger mode .................. [intern]\n");			
	printf("    -i           interrupt enable (whith -b=2 and -t option) [no]\n");
	printf("    -h	         install buffer lowwater signal ....... [no]\n");			
	printf("    -l           loop mode ............................ [no]\n");
	printf("    -w           wait for close path .................. [no]\n");
	printf("\n");
	printf("Copyright 2010-2019, MEN Mikro Elektronik GmbH\n%s\n", IdentString);
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
	int32	blkmode, tout=0;
	int32   signal, loopmode, chNbr, blksize,n, ch ;
	int32	bufMode, trig, intEn, wait, setTime;
	u_int16	period[] = {0x7fff, 0x732c, 0x6660, 0x5994, 0x4cc8, 
						0x3ffc, 0x3330, 0x2664, 0x1998, 0x0ccc, 0x0000,
						0x8000, 0x8cd4, 0x99a0, 0xa66c, 0xb338,
						0xc004, 0xccd0, 0xd99c, 0xe668, 0xf334, 0xffff};
	u_int16 *bp;
	u_int16 *blkbuf = NULL;
	u_int16 valCh0 = 0, valCh1 = 0, valCh2 = 0, valCh3 = 0;
	double  voltCh0 = 0, voltCh1 = 0, voltCh2 = 0, voltCh3 = 0, calc;
    char	*device,*str,*errstr, buf[40];

	/*--------------------+
    |  check arguments    |
    +--------------------*/
	if ((errstr = UTL_ILLIOPT("a=z=b=o=d=e=f=g=stihlw?", buf))) {	/* check args */
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

	blkmode  = ((str = UTL_TSTOPT("b=")) ? atoi(str) : M_BUF_USRCTRL);
	if (UTL_TSTOPT("o="))  {
		tout = atoi(UTL_TSTOPT("o="));
		setTime = TRUE;
	}
	else  {
		setTime = FALSE;
	}
	bufMode  = (UTL_TSTOPT("s") ? 1 : 0);
	trig     = (UTL_TSTOPT("t") ? 1 : 0);
	intEn    = (UTL_TSTOPT("i") ? 1 : 0);
	signal   = (UTL_TSTOPT("h") ? 1 : 0);
	loopmode = (UTL_TSTOPT("l") ? 1 : 0);
	wait     = (UTL_TSTOPT("w") ? 1 : 0);

    if ((str = UTL_TSTOPT("d="))) {
        voltCh0 = atof(str);
		
		if ((voltCh0 >= 10) || (voltCh0 < -10))  {
			printf("illegal value for Channel0: %fV",voltCh0);
			return (1);
		}
		calc = voltCh0*(0xffff/20.0);
		if (calc >= 0)
			calc += 0.5;
		else
			calc -= 0.5;
		valCh0 = (u_int16)(calc);
    }

    if ((str = UTL_TSTOPT("e="))) {
        voltCh1 = atof(str);
 		
		if ((voltCh1 >= 10) || (voltCh1 < -10))  {
			printf("illegal value for Channel1: %fV",voltCh1);
			return (1);
		}
		calc = voltCh1*(0xffff/20.0);
		if (calc >= 0)
			calc += 0.5;
		else
			calc -= 0.5;
		valCh1 = (u_int16)(calc);
   }

    if ((str = UTL_TSTOPT("f="))) {
        voltCh2 = atof(str);
 		if ((voltCh2 >= 10) || (voltCh2 < -10))  {
			printf("illegal value for Channel2: %fV",voltCh2);
			return (1);
		}
		calc = voltCh2*(0xffff/20.0);
		if (calc >= 0)
			calc += 0.5;
		else
			calc -= 0.5;
		valCh2 = (u_int16)(calc);
    }
 
	if ((str = UTL_TSTOPT("g="))) {
        voltCh3 = atof(str);
 		if ((voltCh3 >= 10) || (voltCh3 < -10))  {
			printf("illegal value for Channel3: %fV",voltCh3);
			return (1);
		}
		calc = voltCh3*(0xffff/20.0);
		if (calc >= 0)
			calc += 0.5;
		else
			calc -= 0.5;
		valCh3 = (u_int16)(calc);
    }

	if (signal) {
		/*-------------------+
		| install signal     |
		+-------------------*/
		/* install signal handler */
		if (UOS_SigInit(SigHandler)) {
			PrintUosError("SigInit");
			return(1);
		}
		/* instal signal */
		if (UOS_SigInstall(UOS_SIG_USR1)) {
			PrintUosError("SigInstall");
			goto abort;
		}
	}

	/*--------------------+
	| create buffer       |
	+--------------------*/
	if (bufMode) {			/* write waveform */
		blksize = CH_NUMBER * 2 * PERIOD_SIZE;  /*channels * word * size for 1 period */
		if ((blkbuf = (u_int16*)malloc(blksize)) == NULL) {
			printf("*** can't alloc %ld bytes\n",blksize);
			return(1);
		}
		bp = blkbuf;
		n = 0;
		while (n < PERIOD_SIZE)  {
			for (ch=0; ch<CH_NUMBER; ch++) 
				*bp++ = period[n];
			n++;
		}
	} 
	else {					/* write 4 user values to channels */
		blksize = CH_NUMBER * 2;
		if ((blkbuf = (u_int16*)malloc(blksize)) == NULL) {
			printf("*** can't alloc %ld bytes\n",blksize);
			return(1);
		}
		bp = blkbuf;
		bp[0] = valCh0;
		bp[1] = valCh1;
		bp[2] = valCh2;
		bp[3] = valCh3;
	}

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
	/* set block i/o mode */
	if ((M_setstat(path, M_BUF_WR_MODE, blkmode)) < 0) {
			PrintError("setstat M_BUF_RD_MODE"); 	
		goto abort;
	}
	/* set block write timeout */
	if (setTime) {
		if ((M_setstat(path, M_BUF_WR_TIMEOUT, tout)) < 0) {
			PrintError("setstat M_BUF_RD_TIMEOUT");
			goto abort;
		}
	}
	/* set trigger mode */
	if ((M_setstat(path, M37_EXT_TRIG, trig)) < 0) {
		PrintError("setstat M37_EXT_TRIG");
		goto abort;
	}
	/* get number of channels */
	if ((M_getstat(path, M_LL_CH_NUMBER, &chNbr)) < 0) {
		PrintError("getstat M_LL_CH_NUMBER");
		goto abort;
	}
	if (signal) {
		/* enable buffer lowwater signal */
		if ((M_setstat(path, M_BUF_WR_SIGSET_LOW, UOS_SIG_USR1)) < 0) {
				PrintError("setstat M_BUF_WR_SIGSET_LOW");
			goto abort;
		}
	}
	/* set irq enable */  
	if ((M_setstat(path, M_MK_IRQ_ENABLE, intEn)) < 0) {
		PrintError("setstat M_MK_IRQ_ENABLE");
		goto abort;
	}

	/*--------------------+
    |  write block        |
    +--------------------*/
	do {
		if ( M_setblock(path, (u_int8*)blkbuf, blksize) < 0)  {
			PrintError ("setblock");
		}
	} while (loopmode && UOS_KeyPressed() == -1);
		
	if (wait)
		while (UOS_KeyPressed() == -1);

	/*--------------------+
    |  cleanup            |
    +--------------------*/
	abort:
	if (signal)  {
		printf(">>> Signal handler calls: %ld\n",G_sigHdlErr_lowWater + G_sigHdlErr_other);
		printf("    buffer lowwater notifications: %ld\n",G_sigHdlErr_lowWater);
		printf("    other signal handler calls:    %ld\n",G_sigHdlErr_other);
	}
	if (blkmode)		/* disable interrupt */
		if ( (M_setstat(path, M_MK_IRQ_ENABLE, 0)) <0 )  
			PrintError("setstat M_MK_IRQ_ENABLE");

	if (signal)			/* terminate signal handling */
		UOS_SigExit();

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


/********************************* PrintUosError ****************************
 *
 *  Description: Print MDIS error message
 *			   
 *---------------------------------------------------------------------------
 *  Input......: info	info string
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void PrintUosError(char *info)
{
	printf("*** can't %s: %s\n", info, UOS_ErrString(UOS_ErrnoGet()));
}



/********************************* SigHandler ********************************
 *
 *  Description: Signal handler
 *			   
 *---------------------------------------------------------------------------
 *  Input......: sigCode	signal code recieved
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void __MAPILIB SigHandler (u_int32 sigCode)
{
	switch(sigCode) {
		case UOS_SIG_USR1:
			G_sigHdlErr_lowWater++;
			break;
		default:
			G_sigHdlErr_other++;
	}
}


