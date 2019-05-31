/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: m37_drv.c
 *      Project: M37 module driver 
 *
 *       Author: ls
 *
 *  Description: Low-level driver for M37 M-Modules
 *
 *               The M37 M-Module is a 16-bit D/A converter with interrupt
 *               capabilities.
 *
 *               The direction of each channel is out.
 *                
 *               An interrupt can be triggered at the end of a data
 *               transfer.(1)
 *               
 *               The transfer cycle can be triggered through an external clock
 *               signal.(1)(2) 
 *                
 *               The buffer method depends on the block mode.(1)(2) 
 *
 *                (1) ... definable through status call
 *                (2) ... definable through descriptor key
 *                
 *Known Problems: -
 *                
 *
 *                
 *
 *     Required: ---
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 2010-2019, MEN Mikro Elektronik GmbH
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

#define _NO_LL_HANDLE		/* ll_defs.h: don't define LL_HANDLE structure */

#include <MEN/men_typs.h>   /* system dependent definitions   */
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/dbg.h>        /* debug functions                */
#include <MEN/oss.h>        /* oss functions                  */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/mbuf.h>		/* buffer lib functions and types */
#include <MEN/modcom.h>     /* ID PROM functions              */
#include <MEN/mdis_api.h>   /* MDIS global defs               */
#include <MEN/mdis_com.h>   /* MDIS common defs               */
#include <MEN/mdis_err.h>   /* MDIS error codes               */
#include <MEN/ll_defs.h>    /* low-level driver definitions   */
#include "m37_pld.h"		/* PLD ident / data prototypes	  */    

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* general */
#define CH_NUMBER			4			/* number of device channels */
#define CH_BYTES			2			/* number of bytes per channel */
#define USE_IRQ				TRUE		/* interrupt required  */
#define ADDRSPACE_COUNT		1			/* number of required address spaces */
#define ADDRSPACE_SIZE		256			/* size of address space */
#define MOD_ID_MAGIC		0x5346      /* ID PROM magic word */
#define MOD_ID_SIZE			128			/* ID PROM size [bytes] */
#define MOD_ID				0x25		/* ID PROM M-Module ID (0x25 = 37)*/
#define MOD_ID_N			0x7d25		/* ID PROM M-Module ID for M37N */

/* debug settings */
#define DBG_MYLEVEL			llHdl->dbgLevel
#define DBH					llHdl->dbgHdl

/* register offsets */
#define DATA_REG(i) (0x00 + ((i)<<1))   /* data register 0..3 */
#define STAT_REG    0x40				/* status register (read)*/
#define CONF_REG    0x40				/* configuration register (write)*/
#define LOAD_REG    0xfe				/* FLEX load register */

#define UD		0x01	/* CONF_REG: update */
#define EE		0x02	/* CONF_REG: external enable */
#define IRQE	0x04	/* CONF_REG: irq enable */
#define OE		0x08	/* CONF_REG: output enable */
#define BUFRDY	0x01	/* STAT_REG: buffer ready */
#define PWR		0x10	/* STAT_REG: Power supply to analog circuit */

/* LOAD_REG bitmask */
#define TDO		0x01	/* data */
#define TCK		0x02	/* clock */
#define TMS		0x08	/* tms */

/* single bit setting macros */
#define bitset(byte,mask)		((byte) |=  (mask))   
#define bitclr(byte,mask)		((byte) &= ~(mask))   
#define bitmove(byte,mask,bit)	(bit ? bitset(byte,mask) : bitclr(byte,mask))

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* low-level handle */
typedef struct {
	/* general */
    int32           memAlloc;		/* size allocated for the handle */
    OSS_HANDLE      *osHdl;         /* oss handle */
    OSS_IRQ_HANDLE  *irqHdl;        /* irq handle */
    DESC_HANDLE     *descHdl;       /* desc handle */
    MACCESS         ma;             /* hw access handle */
	MDIS_IDENT_FUNCT_TBL idFuncTbl;	/* id function table */
	/* debug */
    u_int32         dbgLevel;		/* debug level */
	DBG_HANDLE      *dbgHdl;        /* debug handle */
	/* misc */
    u_int32         irqCount;       /* interrupt counter */
    u_int32         idCheck;		/* id check enabled */
	u_int32			extTrig;		/* external trigger */
	u_int32			irqEn;			/* interrupt flag enable */
	u_int32			irqOn;			/* irq on HW enable */
	/* buffers */
	MBUF_HANDLE		*bufHdl;		/* input buffer handle */
	/* channels */
	u_int16			chanVal[CH_NUMBER];/* storage for channels 0..3 */
} LL_HANDLE;

/* include files which need LL_HANDLE */
#include <MEN/ll_entry.h>   /* low-level driver jump table  */
#include <MEN/m37_drv.h>   /* M37 driver header file */


/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static int32 M37_Init(DESC_SPEC *descSpec, OSS_HANDLE *osHdl,
					   MACCESS *ma, OSS_SEM_HANDLE *devSemHdl,
					   OSS_IRQ_HANDLE *irqHdl, LL_HANDLE **llHdlP);
static int32 M37_Exit(LL_HANDLE **llHdlP );
static int32 M37_Read(LL_HANDLE *llHdl, int32 ch, int32 *value);
static int32 M37_Write(LL_HANDLE *llHdl, int32 ch, int32 value);
static int32 M37_SetStat(LL_HANDLE *llHdl,int32 ch, int32 code, INT32_OR_64 value32_or_64);
static int32 M37_GetStat(LL_HANDLE *llHdl, int32 ch, int32 code, INT32_OR_64 *value32_or_64P);
static int32 M37_BlockRead(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
							int32 *nbrRdBytesP);
static int32 M37_BlockWrite(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
							 int32 *nbrWrBytesP);
static int32 M37_Irq(LL_HANDLE *llHdl );
static int32 M37_Info(int32 infoType, ... );

static char* Ident( void );
static int32 Cleanup(LL_HANDLE *llHdl, int32 retCode);
static void PldLoad (LL_HANDLE *llHdl);						

/**************************** M37_GetEntry *********************************
 *
 *  Description:  Initialize driver's jump table
 *
 *---------------------------------------------------------------------------
 *  Input......:  ---
 *  Output.....:  drvP  pointer to the initialized jump table structure
 *  Globals....:  ---
 ****************************************************************************/
void M37_GetEntry( LL_ENTRY* drvP )
{
    drvP->init        = M37_Init;
    drvP->exit        = M37_Exit;
    drvP->read        = M37_Read;
    drvP->write       = M37_Write;
    drvP->blockRead   = M37_BlockRead;
    drvP->blockWrite  = M37_BlockWrite;
    drvP->setStat     = M37_SetStat;
    drvP->getStat     = M37_GetStat;
    drvP->irq         = M37_Irq;
    drvP->info        = M37_Info;
}

/******************************** M37_Init ***********************************
 *
 *  Description:  Allocate and return low-level handle, initialize hardware
 * 
 *                The function initializes all channels with the
 *                definitions made in the descriptor. The interrupt
 *                is disabled. The values of the channels are set to 0V.
 *
 *                The following descriptor keys are used:
 *
 *                Descriptor key        Default          Range
 *                --------------------  ---------------  -------------
 *                DEBUG_LEVEL_DESC      OSS_DBG_DEFAULT  see dbg.h
 *                DEBUG_LEVEL_MBUF      OSS_DBG_DEFAULT  see dbg.h
 *                DEBUG_LEVEL           OSS_DBG_DEFAULT  see dbg.h
 *                ID_CHECK              1                0..1 
 *                PLD_LOAD              1                0..1 
 *                EXT_TRIG              0                0..1
 *                OUT_BUF/SIZE          160              8..max   
 *                OUT_BUF/MODE          0                0 | 2
 *                OUT_BUF/TIMEOUT       1000             0..max 
 *                OUT_BUF/LOWWATER      8                0..max
 *                
 *                PLD_LOAD defines if the PLD is loaded at INIT.
 *                With PLD_LOAD disabled, ID_CHECK is implicitly disabled.
 *                (This key is intended for test purposes and should always be
 *                set to 1.)
 *                
 *                EXT_TRIG defines if the transfer cycle is initiated by
 *                an internal or external trigger.
 *                   0 = internal trigger
 *                   1 = external trigger
 *                
 *                OUT_BUF/SIZE defines the size of the output buffer [bytes]
 *                (multiple of 8).
 *                
 *                OUT_BUF/MODE defines the buffer mode (see MDIS User Guide).
 *                   0 = M_BUF_USRCTRL
 *                   2 = M_BUF_RINGBUF
 *                
 *                OUT_BUF/TIMEOUT defines the buffer write timeout [msec].
 *                   (0 = no timeout)   (see MDIS User Guide)
 *                
 *                OUT_BUF/LOWWATER defines the buffer level [bytes] of the
 *                corresponding lowwater buffer event (0 or multiple of 8).
 *                   (see MDIS User Guide)
 *                
 *---------------------------------------------------------------------------
 *  Input......:  descSpec   pointer to descriptor data
 *                osHdl      oss handle
 *                ma         hw access handle
 *                devSemHdl  device semaphore handle
 *                irqHdl     irq handle
 *                llHdlP     pointer to the variable where low-level driver
 *                           handle will be stored
 *
 *  Output.....:  *llHdlP    low-level driver handle | NULL if fails
 *                return     success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M37_Init(
    DESC_SPEC       *descP,
    OSS_HANDLE      *osHdl,
    MACCESS         *ma,
    OSS_SEM_HANDLE  *devSemHdl,
    OSS_IRQ_HANDLE  *irqHdl,
    LL_HANDLE       **llHdlP
)
{
    DBGCMD( static const char functionName[] = "LL - M37_Init()"; )
    LL_HANDLE *llHdl = NULL;
    u_int32 gotsize, pldLoad,
			bufSize, bufMode, bufTout, bufLow, bufDbgLevel;
    u_int32 value,
			ch,						
			timeout;				
	u_int16	helpreg;
    int32	error;


    /*------------------------------+
    |  prepare the handle           |
    +------------------------------*/
	/* alloc */
    if ((llHdl = (LL_HANDLE*)
		 OSS_MemGet(osHdl, sizeof(LL_HANDLE), &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

	/* clear */
    OSS_MemFill(osHdl, gotsize, (char*)llHdl, 0x00);

	/* init */
    llHdl->memAlloc   = gotsize;
    llHdl->osHdl      = osHdl;
    llHdl->irqHdl     = irqHdl;
    llHdl->ma		  = *ma;

    /*------------------------------+
    |  init id function table       |
    +------------------------------*/
	/* driver's ident function */
	llHdl->idFuncTbl.idCall[0].identCall = Ident;
	llHdl->idFuncTbl.idCall[1].identCall = M37_PldIdent;	
	/* libraries' ident functions */
	llHdl->idFuncTbl.idCall[2].identCall = DESC_Ident;
	llHdl->idFuncTbl.idCall[3].identCall = OSS_Ident;
	llHdl->idFuncTbl.idCall[4].identCall = MBUF_Ident;
	/* terminator */
	llHdl->idFuncTbl.idCall[5].identCall = NULL;

    /*------------------------------+
    |  prepare debugging            |
    +------------------------------*/
	DBG_MYLEVEL = OSS_DBG_DEFAULT;	/* set OS specific debug level */
	DBGINIT((NULL,&DBH));

    DBGWRT_1((DBH, "%s\n", functionName));

    /*------------------------------+
    |  scan descriptor              |
    +------------------------------*/
	/* prepare access */
    if ((error = DESC_Init(descP, osHdl, &llHdl->descHdl)))
		return( Cleanup(llHdl,error) );

    /* DEBUG_LEVEL_DESC */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT, 
								&value, "DEBUG_LEVEL_DESC")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

	DESC_DbgLevelSet(llHdl->descHdl, value);	/* set level */

	/* DEBUG_LEVEL_MBUF */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT, 
                                &bufDbgLevel, "DEBUG_LEVEL_MBUF")) &&
        error != ERR_DESC_KEY_NOTFOUND)        
		return( Cleanup(llHdl,error) );

    /* DEBUG_LEVEL */
    if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT, 
								&llHdl->dbgLevel, "DEBUG_LEVEL")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

    /* ID_CHECK */
    if ((error = DESC_GetUInt32(llHdl->descHdl, TRUE, 
								&llHdl->idCheck, "ID_CHECK")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

    /* PLD_LOAD */														
    if ((error = DESC_GetUInt32(llHdl->descHdl, TRUE, 
								&pldLoad, "PLD_LOAD")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );
	
	if (pldLoad > 1)
		return( Cleanup(llHdl,ERR_LL_ILL_PARAM));
	
	if (pldLoad == FALSE)
		llHdl->idCheck = FALSE;

	/* EXT_TRIG */										
	if ((error = DESC_GetUInt32(llHdl->descHdl, FALSE,
								&llHdl->extTrig, "EXT_TRIG")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

	if (llHdl->extTrig > 1)
		return( Cleanup(llHdl,ERR_LL_ILL_PARAM));

	/* OUT_BUF/SIZE */
	if ( (error = DESC_GetUInt32(llHdl->descHdl, 160,
								&bufSize, "OUT_BUF/SIZE")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error));
	if( !bufSize || (bufSize%(CH_BYTES * CH_NUMBER)) )
		return (Cleanup(llHdl, ERR_LL_ILL_PARAM)) ;

	/* OUT_BUF/MODE */
	if ( (error = DESC_GetUInt32(llHdl->descHdl, M_BUF_USRCTRL, 
								&bufMode, "OUT_BUF/MODE")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return(Cleanup(llHdl, error) );
	if ( (bufMode == M_BUF_CURRBUF) || (bufMode == M_BUF_RINGBUF_OVERWR) )
		return (Cleanup(llHdl, ERR_LL_ILL_PARAM));

	/* OUT_BUF/TIMEOUT */
	if ( (error = DESC_GetUInt32(llHdl->descHdl, 1000, 
								&bufTout, "OUT_BUF/TIMEOUT")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return(Cleanup(llHdl, error) );

	/* OUT_BUF/LOWWATER */
	if ( (error = DESC_GetUInt32(llHdl->descHdl, 8, 
								&bufLow, "OUT_BUF/LOWWATER")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return(Cleanup(llHdl, error) );
	if(bufLow%(CH_BYTES * CH_NUMBER))
			return (Cleanup(llHdl, ERR_LL_ILL_PARAM)) ;

    /*------------------------------+
    |  install buffer               |
    +------------------------------*/
    if ((error = MBUF_Create(llHdl->osHdl, devSemHdl, llHdl, 
                             bufSize, CH_BYTES*CH_NUMBER, bufMode, MBUF_WR,
                             bufLow, bufTout, irqHdl, &llHdl->bufHdl)))
        return( Cleanup(llHdl,error) );
	
	/* set debug level */
    MBUF_SetStat(NULL, llHdl->bufHdl, M_BUF_WR_DEBUG_LEVEL, bufDbgLevel);

    /*------------------------------+
    |  check M-Module ID            |
    +------------------------------*/
	if (llHdl->idCheck) {
		int modIdMagic = m_read((U_INT32_OR_64)llHdl->ma, 0);
		int modId      = m_read((U_INT32_OR_64)llHdl->ma, 1);

		if (modIdMagic != MOD_ID_MAGIC) {
			DBGWRT_ERR((DBH," *** %s: illegal magic=0x%04x\n",
						functionName, modIdMagic));
			error = ERR_LL_ILL_ID;
			return( Cleanup(llHdl,error) );
		}
		
		switch (modId) {
		/* accept M37 & M37N */
		case MOD_ID:
		case MOD_ID_N:
			DBGWRT_1((DBH, "M37_Init - ID check: found modId 0x%04x\n", modId));
			break;

		default:
			DBGWRT_ERR((DBH," *** %s: illegal id=%d\n", functionName, modId));
			error = ERR_LL_ILL_ID;
			return(Cleanup(llHdl,error));
		}
	}

    /*------------------------------+	
    |  load PLD                     |
    +------------------------------*/
	if (pldLoad)
	{
	    DBGWRT_2((DBH, "%s: load PLD\n", functionName));
		PldLoad (llHdl);
	}

    /*------------------------------+
    |  init hardware                |
    +------------------------------*/
	MWRITE_D16 (llHdl->ma, CONF_REG, OE);	/* output enable, */
											/*  disable IRQE & EE */
	/* clear irq flag */
	llHdl->irqEn = FALSE;
	llHdl->irqOn = FALSE;

    DBGWRT_2((DBH, "%s: reset channels\n", functionName));
	/* clear first part of hardware buffer */
	for (ch=0; ch<CH_NUMBER; ch++) {
		MWRITE_D16 (llHdl->ma, DATA_REG(ch), 0x0000); /* channels are set to zero */	
		llHdl->chanVal[ch] = 0x0000;			/* set the channel store to zero */
	}
	MSETMASK_D16 ( llHdl->ma, CONF_REG, UD);	/* update */	
	timeout = 0;
	do  {	/* wait for buffer ready or break if power supply fails or timeout occurs */
		OSS_Delay(llHdl->osHdl,10);
		helpreg = MREAD_D16(llHdl->ma,STAT_REG);
		if (!(helpreg & PWR))  {	/* check power supply to analog part */
			error = ERR_LL_DEV_NOTRDY;
			DBGWRT_ERR((DBH," *** %s: PWR fails\n", functionName));
			return(Cleanup(llHdl,error));
		}
		if (timeout++ >= 10)  {		/* buffer ready timeout */
			error = ERR_LL_DEV_NOTRDY;
			DBGWRT_ERR((DBH," *** %s: timeout\n", functionName));
			return (Cleanup(llHdl,error));
		}
	} while (!(helpreg & BUFRDY));

	/* clear 2nd part of hardware buffer */
	for (ch=0; ch<CH_NUMBER; ch++) {
		MWRITE_D16 (llHdl->ma, DATA_REG(ch), 0x0000);  	/* channels are set to zero */
	}
	MSETMASK_D16 ( llHdl->ma, CONF_REG, UD);	/* update */	
	timeout = 0;
	do  {	/* wait for buffer ready or break if power supply fails or timeout occurs */
		OSS_Delay(llHdl->osHdl,10);
		helpreg = MREAD_D16(llHdl->ma,STAT_REG);
		if (!(helpreg & PWR))  {	/* check power supply to analog part */
			error = ERR_LL_DEV_NOTRDY;
			DBGWRT_ERR((DBH," *** %s: PWR fails\n", functionName));
			return(Cleanup(llHdl,error));
		}
		if (timeout++ >= 10)  {		/* buffer ready timeout */
			error = ERR_LL_DEV_NOTRDY;
			DBGWRT_ERR((DBH," *** %s: timeout\n", functionName));
			return (Cleanup(llHdl,error));
		}
	} while (!(helpreg & BUFRDY));
	
	/* config the trigger mode (int/ext) */
	if (llHdl->extTrig){
	    DBGWRT_2((DBH, "%s: set extTrig\n", functionName));
		helpreg = MREAD_D16(llHdl->ma, STAT_REG) & ~UD;
		MWRITE_D16(llHdl->ma, CONF_REG, (helpreg | EE));
	}

	*llHdlP = llHdl;
	return(ERR_SUCCESS);
}

/****************************** M37_Exit *************************************
 *
 *  Description:  De-initialize hardware and clean up memory
 *
 *                The interrupt and trigger are disabled.
 *                The function deinitializes all channels by setting them
 *                to 0V.
 *                The configuration register is set to 0.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdlP   pointer to low-level driver handle
 *
 *  Output.....:  *llHdlP  NULL
 *                return   success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M37_Exit(
   LL_HANDLE    **llHdlP
)
{
    DBGCMD( static const char functionName[] = "LL - M37_Exit"; )
    LL_HANDLE *llHdl = *llHdlP;
	int32 error = 0;
	u_int32 ch;
	u_int16	helpreg;

    DBGWRT_1((DBH, "%s\n", functionName));

    /*------------------------------+
    |  de-init hardware             |
    +------------------------------*/
	MCLRMASK_D16(llHdl->ma, CONF_REG, (IRQE | EE | UD) );/* disable interrupt and trigger */
	llHdl->irqEn = FALSE;
	llHdl->extTrig = FALSE;

	for (ch=0; ch<CH_NUMBER; ch++) {
		MWRITE_D16 (llHdl->ma, DATA_REG(ch), 0x0000);	/* channels are set to zero */
		llHdl->chanVal[ch] = 0x0000;					/* set the channel store to zero */
	}
	MSETMASK_D16 ( llHdl->ma, CONF_REG, UD);			/* update */	
	do  {   /* wait for buffer ready or break if power supply fails */
		helpreg = MREAD_D16(llHdl->ma,STAT_REG);
		if (!(helpreg & PWR))  {	/* check power supply to analog part */
			DBGWRT_ERR((DBH," *** %s: PWR fails\n", functionName));
			error= ERR_LL_DEV_NOTRDY;
			break;
		}
	} while (!(helpreg & BUFRDY));

	MWRITE_D16(llHdl->ma, CONF_REG, 0x00);				/* disable all */

    /*------------------------------+
    |  cleanup memory               |
    +------------------------------*/
	*llHdlP = 0;
	error = Cleanup(llHdl,error);

	return(error);
}

/****************************** M37_Read *************************************
 *
 *  Description:  Read a value from the device
 *
 *                The function is not supported and always returns an 
 *                ERR_LL_ILL_FUNC error.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *                ch       current channel
 *  Output.....:  valueP   read value
 *                return   success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M37_Read(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 *valueP
)
{
    DBGCMD( static const char functionName[] = "LL - M37_Read"; )
    DBGWRT_1((DBH, "%s: ch=%d\n", functionName,ch));
	
	return(ERR_LL_ILL_FUNC);
}

/****************************** M37_Write ************************************
 *
 *  Description:  Write a value to the device
 *
 *                The function writes the lower word of the value to the 
 *                current channel. The external trigger must be disabled.
 *
 *                The alternating buffer principle on the hardware is not 
 *                transparent to the application. Changing one channel doesn't 
 *                affect the others. The values are stored in llHdl.
 *
 *                When power supply to the analog circuit fails while waiting 
 *                for BUFRDY, an error is reported.
 *                
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *                ch       current channel
 *                value    value to write 
 *  Output.....:  return   success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M37_Write(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 value
)
{
    DBGCMD( static const char functionName[] = "LL - M37_Write"; )
	u_int32	n;
	u_int16 helpreg;

    DBGWRT_1((DBH, "%s: ch=%d val=0x%04x\n", functionName,ch, value));

	/* check if ext. trig. */
	if (llHdl->extTrig)
	{
		DBGWRT_ERR((DBH," *** %s: extTrig\n", functionName));
		return (ERR_LL_ILL_PARAM);
	}

	/* write value */
	llHdl->chanVal[ch] = (u_int16)value;	/* update value for current channel storage */
	for (n=0; n<CH_NUMBER; n++) {
		MWRITE_D16 (llHdl->ma, DATA_REG(n),llHdl->chanVal[n]);	
	}
	MSETMASK_D16 ( llHdl->ma, CONF_REG, UD);			/*update*/	
	do  {	/* wait for buffer ready or break if power supply fails */
		helpreg = MREAD_D16(llHdl->ma,STAT_REG);
		if (!(helpreg & PWR))  {	/* check power supply to analog part */
			DBGWRT_ERR((DBH," *** %s: PWR fails\n", functionName));
			return(ERR_LL_DEV_NOTRDY);
		}
	} while (!(helpreg & BUFRDY));

	return(ERR_SUCCESS);
}

/****************************** M37_SetStat **********************************
 *
 *  Description:  Set the driver status
 *
 *                The following status codes are supported:
 *
 *                Code                 Description                Values
 *                -------------------  -------------------------  ----------
 *                M_LL_DEBUG_LEVEL     driver debug level         see dbg.h
 *                M_MK_IRQ_ENABLE      interrupt enable           0..1
 *                M_LL_IRQ_COUNT       interrupt counter          0..max
 *                M_LL_CH_DIR          direction of current       M_CH_OUT
 *                                     channel
 *                M_BUF_WR_MODE        mode of write buffer       M_BUF_USRCTRL,
 *                                                                M_BUF_RINGBUF
 *                -------------------  -------------------------  ----------
 *                M37_EXT_TRIG         defines the trigger mode	  0..1
 *
 *
 *                M_MK_IRQ_ENABLE enables/disables the interrupt.
 *
 *                    0 = interrupt disable
 *                    1 = interrupt enable
 *
 *                This function only enables a flag. The interrupt on the
 *                hardware is enabled before data is written to the output
 *                buffer (M_BUF_RINGBUF) through M37_BlockWrite. The interrupt
 *                on hardware is disabled in the ISR when all values have been
 *                written to the M37 M-Module. The flag remains enabled.
 *                Disabling the flag in SetStat also disables the interrupt
 *                on the hardware.
 *
 *                The interrupt can only be enabled when the trigger is in external
 *                mode and M_BUF_WR_MODE is in M_BUF_RINGBUF mode.
 *  
 *
 *                M_BUF_WR_MODE sets the mode of the write buffer.
 *                    M_BUF_USRCTRL      0
 *                    M_BUF_RINGBUF      2
 *                For setting M_BUF_WR_MODE to M_BUF_USRCTRL the interrupt 
 *                must be disabled.
 *
 *
 *                M37_EXT_TRIG defines the trigger mode:
 *                    0 = internal trigger
 *                    1 = external trigger
 *                The trigger mode can only be disabled when the interrupt is
 *                disabled.
 * 
 *---------------------------------------------------------------------------
 *  Input......:  llHdl             low-level handle
 *                code              status code
 *                ch                current channel
 *                value32_or_64     data or
 *                                  pointer to block data structure (M_SG_BLOCK)  (*)
 *                (*) = for block status codes
 *  Output.....:  return     success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M37_SetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 value32_or_64
)
{
    int32 value = (int32)value32_or_64;	    /* 32bit value */
    /* INT32_OR_64 valueP = value32_or_64;     stores 32/64bit pointer */
    DBGCMD( static const char functionName[] = "LL - M37_SetStat"; )
	int32 error = ERR_SUCCESS;
	int32 bufMode;
	u_int16	helpreg;


    DBGWRT_1((DBH, "%s: ch=%d code=0x%04x value=0x%x\n",
			  functionName,ch,code,value));

    switch(code) {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            llHdl->dbgLevel = value;
            break;
        /*--------------------------+
        |  enable interrupt flag    |
		|  (if external trigger and |
		|  M_BUF_RINGBUF)			|
        +--------------------------*/
        case M_MK_IRQ_ENABLE:
			if ( (value < 0) || (value > 1) )  {  /* range of value */
				error = ERR_LL_ILL_PARAM;							 
				break;
			}
			/* get current buffer mode */
			if  (( error = MBUF_GetBufferMode(llHdl->bufHdl, &bufMode)))
				break;

			/* enable irq flag ...*/
			if (value) {	/* ... only if external trigger on and M_BUF_RINGBUF */
				if (!llHdl->extTrig || (bufMode != M_BUF_RINGBUF))  {	
					error = ERR_LL_ILL_PARAM;
					break;
				}
				llHdl->irqEn = TRUE;  /* set interrupt enable flag */
			}   
			/* disable irq and interrupt flags*/
			else {											
				MCLRMASK_D16(llHdl->ma, CONF_REG, (IRQE | UD) );
				llHdl->irqEn = FALSE;
				llHdl->irqOn = FALSE;		
			}
            break;
        /*--------------------------+
        |  set irq counter          |
        +--------------------------*/
        case M_MK_IRQ_COUNT:
            llHdl->irqCount = value;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
			if (value != M_CH_OUT)
				error = ERR_LL_ILL_DIR; 
            break;
        /*-------------------------------------+
        |  trigger mode (int/ext)	           |
        +-------------------------------------*/
		case M37_EXT_TRIG:
			if ( (value < 0) || (value > 1) ) {			/* range of value */
				error = ERR_LL_ILL_PARAM;
				break;
			}
			/* enable external */
			if (value){						
				llHdl->extTrig = TRUE;
				helpreg = MREAD_D16(llHdl->ma, STAT_REG) & ~UD;
				MWRITE_D16(llHdl->ma, CONF_REG, (helpreg | EE) );				
			}
			/* disable external trigger ... */
			else {							
				if (llHdl->irqEn)  {   /* ... only if irq is disabled */
					error = ERR_LL_ILL_PARAM;
					break;
				}
				MCLRMASK_D16(llHdl->ma, CONF_REG, ( EE | UD) );
				llHdl->extTrig = FALSE;
			}
			break;
		/*------------------------------------------+
        |  not supportet MBUF modes and MBUF values |
		+------------------------------------------*/
        case M_BUF_WR_MODE:
			if ( (llHdl->irqEn) && (value == M_BUF_USRCTRL) )  {
				error = ERR_LL_ILL_PARAM;  /* M_BUF_USRCTRL only if interrupt is disabled */
				break;
			}
			if ( (value == M_BUF_CURRBUF) || (value == M_BUF_RINGBUF_OVERWR) )  {
				error = ERR_LL_ILL_PARAM;
				break;
			}
			error = MBUF_SetStat(NULL, llHdl->bufHdl, code, value);
			break;
		case M_BUF_WR_LOWWATER:
			if(value%(CH_BYTES * CH_NUMBER))  {
				error =  ERR_LL_ILL_PARAM;
				break;
			}
			error = MBUF_SetStat(NULL, llHdl->bufHdl, code, value);
			break;
        /*--------------------------+
        |  MBUF + (unknown)         |
        +--------------------------*/
        default:
			if ( M_BUF_CODE(code) )  {
				error = MBUF_SetStat(NULL, llHdl->bufHdl, code, value);
			}
			else
                error = ERR_LL_UNK_CODE;
    }

	return(error);
}

/****************************** M37_GetStat **********************************
 *
 *  Description:  Get the driver status
 *
 *                The following status codes are supported:
 *
 *                Code                 Description                Values
 *                -------------------  -------------------------  ----------
 *                M_LL_DEBUG_LEVEL     driver debug level         see dbg.h
 *                M_LL_CH_NUMBER       number of channels         4
 *                M_LL_CH_DIR          direction of curr. ch.     M_CH_OUT
 *                M_LL_CH_LEN          length of curr. ch. [bits] 16
 *                M_LL_CH_TYP          description of curr. ch.   M_CH_ANALOG
 *                M_LL_IRQ_COUNT       interrupt counter          0..max
 *                M_LL_ID_CHECK        EEPROM is checked          0..1
 *                M_LL_ID_SIZE         EEPROM size [bytes]        128
 *                M_LL_BLK_ID_DATA     EEPROM raw data            -
 *                M_MK_BLK_REV_ID      ident function table ptr   -
 *                -------------------  -------------------------  ----------
 *                M37_EXT_TRIG         defines the trigger mode	  0..1
 *                                      0 = internal trigger
 *                                      1 = external trigger
 *                M37_PWR_SUPPL        power supply to analog     0..1 
 *                                     circuit
 *                                      0 = analog part is not supplied
 *                                      1 = analog part is supplied
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl             low-level handle
 *                code              status code
 *                ch                current channel
 *                value32_or_64P    pointer to block data structure (M_SG_BLOCK)  (*) 
 *                (*) = for block status codes
 *  Output.....:  value32_or_64P    data pointer or
 *                                  pointer to block data structure (M_SG_BLOCK)  (*) 
 *                return            success (0) or error code
 *                (*) = for block status codes
 *  Globals....:  ---
 ****************************************************************************/
static int32 M37_GetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 *value32_or_64P
)
{
    DBGCMD( static const char functionName[] = "LL - M37_GetStat"; )
    int32		*valueP		  = (int32*)value32_or_64P;	      /* pointer to 32bit value  */
    INT32_OR_64	*value64P	  = value32_or_64P;		 		  /* stores 32/64bit pointer  */
    M_SG_BLOCK	*blk 		  = (M_SG_BLOCK*)value32_or_64P;  /* stores block struct pointer */

	int32 error = ERR_SUCCESS;

    DBGWRT_1((DBH, "%s: ch=%d code=0x%04x\n",functionName,ch,code));

    switch(code)
    {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            *valueP = llHdl->dbgLevel;
            break;
        /*--------------------------+
        |  number of channels       |
        +--------------------------*/
        case M_LL_CH_NUMBER:
            *valueP = CH_NUMBER;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            *valueP = M_CH_OUT;
            break;
        /*--------------------------+
        |  channel length [bits]    |
        +--------------------------*/
        case M_LL_CH_LEN:
            *valueP = 16;
            break;
        /*--------------------------+
        |  channel type info        |
        +--------------------------*/
        case M_LL_CH_TYP:
            *valueP = M_CH_ANALOG;
            break;
        /*--------------------------+
        |  irq counter              |
        +--------------------------*/
        case M_LL_IRQ_COUNT:
            *valueP = llHdl->irqCount;
            break;
        /*--------------------------+
        |  ID PROM check enabled    |
        +--------------------------*/
        case M_LL_ID_CHECK:
            *valueP = llHdl->idCheck;
            break;
        /*--------------------------+
        |   ID PROM size            |
        +--------------------------*/
        case M_LL_ID_SIZE:
            *valueP = MOD_ID_SIZE;
            break;
        /*--------------------------+
        |   ID PROM data            |
        +--------------------------*/
        case M_LL_BLK_ID_DATA:
		{
			u_int8 n;
			u_int16 *dataP = (u_int16*)blk->data;

			if (blk->size < MOD_ID_SIZE)		/* check buf size */
				return(ERR_LL_USERBUF);

			for (n=0; n<MOD_ID_SIZE/2; n++)		/* read MOD_ID_SIZE/2 words */
				*dataP++ = (int16)m_read((U_INT32_OR_64)llHdl->ma, n);

			break;
		}
        /*--------------------------+
        |   ident table pointer     |
        |   (treat as non-block!)   |
        +--------------------------*/
        case M_MK_BLK_REV_ID:
           *value64P = (INT32_OR_64)&llHdl->idFuncTbl;
           break;
        /*--------------------------+
        |  trigger mode (int/ext)   |
        +--------------------------*/
		case M37_EXT_TRIG:
			*valueP = (int32) llHdl->extTrig;
			break;
        /*--------------------------+
        |  power supply             |
        +--------------------------*/
		case M37_PWR_SUPPL:
			*valueP =(( MREAD_D16(llHdl->ma,STAT_REG) & PWR ) ? 1 : 0 );
			break;
		/*--------------------------+
        |  MBUF + (unknown)         |
        +--------------------------*/
        default:
			if ( M_BUF_CODE(code) )
				error = MBUF_GetStat(NULL, llHdl->bufHdl, code, valueP);
			else
				error = ERR_LL_UNK_CODE;
    }
	return(error);
}

/******************************* M37_BlockRead *******************************
 *
 *  Description:  Read a data block from the device
 *
 *                The function is not supported and always returns an 
 *                ERR_LL_ILL_FUNC error.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        low-level handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size
 *  Output.....:  nbrRdBytesP  number of read bytes
 *                return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M37_BlockRead(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrRdBytesP
)
{
    DBGCMD( static const char functionName[] = "LL - M37_BlockRead"; )
    DBGWRT_1((DBH, "%s: ch=%d, size=%d\n", functionName,ch,size));

	/* return number of read bytes */
	*nbrRdBytesP = 0;
	
	return(ERR_LL_ILL_FUNC);
}

/****************************** M37_BlockWrite *******************************
 *
 *  Description:  Write a data block to the device
 *                
 *                The following block modes are supported:
 *                  M_BUF_USRCTRL     direct output
 *                  M_BUF_RINGBUF     buffered output via ISR
 *                (can be defined through M_BUF_WR_MODE setstat)
 *                
 *                Direct Output (M_BUF_USRCTRL)
 *                -----------------------------
 *                The function writes all channels out of the given data
 *                buffer (8 bytes). The external trigger must be disabled.
 *                When power supply to the analog circuit fails while waiting 
 *                for BUFRDY, an error is reported.
 *                
 *                +---------------+
 *                | word 0 chan 0 |
 *                +---------------+
 *                | word 1 chan 1 |
 *                +---------------+
 *                | word 2 chan 2 |
 *                +---------------+
 *                | word 3 chan 3 |
 *                +---------------+
 *                
 *                
 *                Buffered Output (M_BUF_RINGBUF)
 *                -------------------------------
 *                The function copies the given data buffer to an output
 *                buffer when the interrupt is enabled.
 *                The buffer is written to the channels in ISR.
 *                The power supply to the analog circuit is not verified.   
 *                
 *                +---------------+
 *                | word 0 chan 0 |
 *                +---------------+
 *                | word 1 chan 1 |
 *                +---------------+
 *                | word 2 chan 2 |
 *                +---------------+
 *                | word 3 chan 3 |
 *                +---------------+
 *                | word 4 chan 0 |
 *                +---------------+
 *                |      ...      |
 *                +---------------+
 *                | word n chan 3 |
 *                +---------------+
 *                
 *---------------------------------------------------------------------------
 *  Input......:  llHdl        low-level handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size
 *  Output.....:  nbrWrBytesP  number of written bytes
 *                return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M37_BlockWrite(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrWrBytesP
)
{
    DBGCMD( static const char functionName[] = "LL - M37_BlockWrite"; )
	u_int32		n;
	int32		error, bufMode;
	u_int16		*bufP = (u_int16*) buf;
	u_int16		helpreg;

	DBGWRT_1((DBH, "%s: ch=%d, size=%d\n", functionName,ch,size));

	/* get current buffer mode */
	if  (( error = MBUF_GetBufferMode(llHdl->bufHdl, &bufMode)))
		return(error);

	/*----------------------+
	| write to hardware     |
	+----------------------*/
	if (bufMode == M_BUF_USRCTRL) {			/* user controlled */
		/* check if ext. trig */
		if (llHdl->extTrig)
			return (ERR_LL_ILL_PARAM);

		/* check size */
		if( size != (CH_BYTES * CH_NUMBER) )
			return (ERR_LL_USERBUF);
		
		/* write to channels */
		for (n=0; n<CH_NUMBER; n++) {
			llHdl->chanVal[n] = *bufP++;		/* update value for current channel */
			MWRITE_D16(llHdl->ma, DATA_REG(n), llHdl->chanVal[n]);
		}
		MSETMASK_D16 ( llHdl->ma, CONF_REG, UD);			/*update*/	
		do  { 	/* wait for buffer ready or break if power supply fails */   
			helpreg = MREAD_D16(llHdl->ma,STAT_REG);
			if (!(helpreg & PWR))  {	/* check power supply to analog part */
				DBGWRT_ERR((DBH," *** %s: PWR fails\n", functionName));
				return(ERR_LL_DEV_NOTRDY);
			}
		} while (!(helpreg & BUFRDY));
		*nbrWrBytesP = (int32)(bufP - (u_int16*)buf);
	}
	/*-------------------------+
	| fill output buffer       |
	+-------------------------*/
	else {
		if (!llHdl->irqEn) /* break when interrupt flag is disabled */
			return (ERR_LL_ILL_PARAM);		  	
		
		/* check size */
		if( !size || (size%(CH_BYTES * CH_NUMBER)) )
			return (ERR_LL_USERBUF);

		/* enable interrupt on hardware */
		llHdl->irqOn = TRUE;		/* until all values are written */
		helpreg = MREAD_D16(llHdl->ma, STAT_REG) & ~UD;		
		MWRITE_D16(llHdl->ma, CONF_REG, (helpreg | IRQE) );

		if ((error = MBUF_Write(llHdl->bufHdl, (u_int8*)bufP, size,
										nbrWrBytesP)))     {
			llHdl->irqOn = FALSE;		/* disable irq in isr */
			return(error);		
		}
		llHdl->irqOn = FALSE;		/* disable irq in isr */
	}
	return(ERR_SUCCESS);
}

/****************************** M37_Irq *************************************
 *
 *  Description:  Interrupt service routine
 *
 *                When the interrupt is enabled, it is triggered at the end 
 *                of conversion cycle.
 *                
 *                4 words from the output buffer are written to the 4 channels:
 *                
 *                +---------------+
 *                | word 0 chan 0 |
 *                +---------------+
 *                | word 1 chan 1 |
 *                +---------------+
 *                | word 2 chan 2 |
 *                +---------------+
 *                | word 3 chan 3 |
 *                +---------------+
 *                
 *                When all values have been written, the last values are
 *                written again and the interrupt is disabled on the hardware.
 *                If the output buffer is empty and not all of the user data passed
 *                to M37_BlockWrite was written, the last values are written until
 *                the output buffer is filled again.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl    low-level handle
 *  Output.....:  return   LL_IRQ_DEVICE    irq caused by device
 *                         LL_IRQ_DEV_NOT   irq not caused by device
 *  Globals....:  ---
 ****************************************************************************/
static int32 M37_Irq(
   LL_HANDLE *llHdl
)
{
    DBGCMD( static const char functionName[] = ">>> LL - M37_Irq"; )
	u_int32 ch;
    int32	got;
	u_int16	*bufP;

	IDBGWRT_1((DBH, "%s:\n",functionName));
	
	/*---------------------------+ 
	| interrupt activated by M37 |
	+---------------------------*/
	if (!( MREAD_D16(llHdl->ma, STAT_REG) && (IRQE || BUFRDY) ))
		return(LL_IRQ_DEV_NOT);		/* say: not */

	/*----------------------+
	| push buffer			|
	+----------------------*/
	bufP = (u_int16*)MBUF_GetNextBuf(llHdl->bufHdl, 1, &got);
	if (bufP != 0)  {
		for (ch=0; ch<CH_NUMBER; ch++)  {
			/* push buffer entry */
			llHdl->chanVal[ch] = *bufP++;
			MWRITE_D16(llHdl->ma, DATA_REG(ch), llHdl->chanVal[ch]);
		}
		MSETMASK_D16 ( llHdl->ma, CONF_REG, UD);			/* update */
		MBUF_ReadyBuf( llHdl->bufHdl );
	}
	/* no valid data in buffer (buffer empty) */
	else  {
		if (!llHdl->irqOn)  {   /* not the first time in ISR and MBUF buffer empty */
			/* disable interrupt on hardware */
			MCLRMASK_D16(llHdl->ma, CONF_REG, (IRQE | UD) );
		}
		for (ch=0; ch<CH_NUMBER; ch++)  {		/* write the last values again */
				MWRITE_D16(llHdl->ma, DATA_REG(ch), llHdl->chanVal[ch]);
		}
		MSETMASK_D16 ( llHdl->ma, CONF_REG, UD);			/* update */
	}
	llHdl->irqCount++;

	return(LL_IRQ_DEVICE);		/* say: known */
}

/****************************** M37_Info ************************************
 *
 *  Description:  Get information about hardware and driver requirements.
 *
 *                The following info codes are supported:
 *
 *                Code                      Description
 *                ------------------------  -----------------------------
 *                LL_INFO_HW_CHARACTER      hardware characteristics
 *                LL_INFO_ADDRSPACE_COUNT   nr of required address spaces
 *                LL_INFO_ADDRSPACE         address space information
 *                LL_INFO_IRQ               interrupt required
 *                LL_INFO_LOCKMODE          process lock mode required
 *
 *                The LL_INFO_HW_CHARACTER code returns all address and 
 *                data modes (ORed) which are supported by the hardware
 *                (MDIS_MAxx, MDIS_MDxx).
 *
 *                The LL_INFO_ADDRSPACE_COUNT code returns the number
 *                of address spaces used by the driver.
 *
 *                The LL_INFO_ADDRSPACE code returns information about one
 *                specific address space (MDIS_MAxx, MDIS_MDxx). The returned
 *                data mode represents the widest hardware access used by
 *                the driver.
 *
 *                The LL_INFO_IRQ code returns whether the driver supports an
 *                interrupt routine (TRUE or FALSE).
 *
 *                The LL_INFO_LOCKMODE code returns which process locking
 *                mode is required by the driver (LL_LOCK_xxx).
 *
 *---------------------------------------------------------------------------
 *  Input......:  infoType	   info code
 *                ...          argument(s)
 *  Output.....:  return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 M37_Info(
   int32  infoType,
   ...
)
{
    int32   error = ERR_SUCCESS;
    va_list argptr;

    va_start(argptr, infoType );

    switch(infoType) {
		/*-------------------------------+
        |  hardware characteristics      |
        |  (all addr/data modes ORed)   |
        +-------------------------------*/
        case LL_INFO_HW_CHARACTER:
		{
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);

			*addrModeP = MDIS_MA08;
			*dataModeP = MDIS_MD08 | MDIS_MD16;
			break;
	    }
		/*-------------------------------+
        |  nr of required address spaces |
        |  (total spaces used)           |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE_COUNT:
		{
			u_int32 *nbrOfAddrSpaceP = va_arg(argptr, u_int32*);

			*nbrOfAddrSpaceP = ADDRSPACE_COUNT;
			break;
	    }
		/*-------------------------------+
        |  address space type            |
        |  (widest used data mode)       |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE:
		{
			u_int32 addrSpaceIndex = va_arg(argptr, u_int32);
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);
			u_int32 *addrSizeP = va_arg(argptr, u_int32*);

			if (addrSpaceIndex >= ADDRSPACE_COUNT)
				error = ERR_LL_ILL_PARAM;
			else {
				*addrModeP = MDIS_MA08;
				*dataModeP = MDIS_MD16;
				*addrSizeP = ADDRSPACE_SIZE;
			}

			break;
	    }
		/*-------------------------------+
        |   interrupt required           |
        +-------------------------------*/
        case LL_INFO_IRQ:
		{
			u_int32 *useIrqP = va_arg(argptr, u_int32*);

			*useIrqP = USE_IRQ;
			break;
	    }
		/*-------------------------------+
        |   process lock mode            |
        +-------------------------------*/
        case LL_INFO_LOCKMODE:
		{
			u_int32 *lockModeP = va_arg(argptr, u_int32*);

			*lockModeP = LL_LOCK_CALL;
			break;
	    }
		/*-------------------------------+
        |   (unknown)                    |
        +-------------------------------*/
        default:
          error = ERR_LL_ILL_PARAM;
    }

    va_end(argptr);
    return(error);
}

/*******************************  Ident  ************************************
 *
 *  Description:  Return ident string
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return  pointer to ident string
 *  Globals....:  -
 ****************************************************************************/
static char* Ident( void )
{
    return( "M37 - M37 low level driver: $Id: m37_drv.c,v 1.9 2014/07/14 16:18:03 MRoth Exp $" );
}

/********************************* Cleanup **********************************
 *
 *  Description: Close all handles, free memory and return error code
 *		         NOTE: The low-level handle is invalid after this function is
 *                     called.
 *			   
 *---------------------------------------------------------------------------
 *  Input......: llHdl		low-level handle
 *               retCode    return value
 *  Output.....: return	    retCode
 *  Globals....: -
 ****************************************************************************/
static int32 Cleanup(
   LL_HANDLE    *llHdl,
   int32        retCode
)
{
    /*------------------------------+
    |  close handles                |
    +------------------------------*/
	/* clean up desc */
	if (llHdl->descHdl)
		DESC_Exit(&llHdl->descHdl);

	/* clean up buffer */
	if (llHdl->bufHdl)
		MBUF_Remove(&llHdl->bufHdl);
	
	/* clean up debug */
	DBGEXIT((&DBH));

    /*------------------------------+
    |  free memory                  |
    +------------------------------*/
    /* free my handle */
    OSS_MemFree(llHdl->osHdl, (int8*)llHdl, llHdl->memAlloc);

    /*------------------------------+
    |  return error code            |
    +------------------------------*/
	return(retCode);
}

/******************************** PldLoad ***********************************
 *
 *  Description:  Loading PLD with binary data.
 *                - binary data is stored in field 'M37_PldData'
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl		low-level handle
 *  Output.....:  ---
 *  Globals....:  ---
 ****************************************************************************/
static void PldLoad(
	LL_HANDLE *llHdl
)
{
	u_int8	ctrl = 0x00;					/* control word */
	u_int8  *dataP = (u_int8*)M37_PldData;	/* point to binary data */
	u_int8	byte;							/* current byte */
	u_int8	n;								/* count */
	u_int32	size;							/* size of binary data */

	DBGWRT_1((DBH, "LL - M37: PldLoad\n"));

	/* read+skip size */
	size  = (u_int32)(*dataP++) << 24;   		
	size |= (u_int32)(*dataP++) << 16;
	size |= (u_int32)(*dataP++) <<  8;
	size |= (u_int32)(*dataP++);

	/* for all bytes */
	while(size--) {
		byte = *dataP++;	/* get next data byte */      
		n = 4;			/* data byte: 4*2 bit */

		/* write data 2 bits */
		while(n--) {
			/* clear TCK */
      		bitclr (ctrl,TCK);	
			MWRITE_D16(llHdl->ma, LOAD_REG, ctrl);

			/* write TDO/TMS bits */
			bitmove(ctrl,TDO,byte & 0x01);       
			bitmove(ctrl,TMS,byte & 0x02); 
			MWRITE_D16(llHdl->ma, LOAD_REG, ctrl);        

			/* set TCK (pulse) */
			bitset (ctrl,TCK);
			MWRITE_D16(llHdl->ma, LOAD_REG, ctrl);        

			/* shift byte */
			byte >>= 2;
		}
	} 
}



