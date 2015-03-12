/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: m37_drv.h
 *
 *       Author: ls
 *        $Date: 2010/04/23 14:08:00 $
 *    $Revision: 2.4 $
 *
 *  Description: Header file for M37 driver
 *               - M37 specific status codes
 *               - M37 function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m37_drv.h,v $
 * Revision 2.4  2010/04/23 14:08:00  amorbach
 * R: driver ported to MDIS5, new MDIS_API and men_typs
 * M: for backward compatibility to MDIS4 optionally define new types
 *
 * Revision 2.3  2004/04/15 16:38:04  cs
 * Minor modifications for MDIS4/2004 conformity
 *
 * Revision 2.2  1999/06/02 14:09:49  kp
 * cosmetics
 *
 * Revision 2.1  1999/05/11 14:31:46  Schoberl
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _M37_DRV_H
#define _M37_DRV_H

#ifdef __cplusplus
      extern "C" {
#endif

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* none */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* M37 specific status codes (STD) */        /* S,G: S=setstat, G=getstat */
#define M37_EXT_TRIG           M_DEV_OF+0x00 /* G,S: defines the sampling mode */
#define M37_PWR_SUPPL          M_DEV_OF+0x01 /* G  : power supply to analog circuit*/

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
#ifdef _LL_DRV_

#ifdef _ONE_NAMESPACE_PER_DRIVER_
#	define M37_GetEntry		LL_GetEntry
#else
	/* variant for swapped access */
#	ifdef ID_SW
#		define M37_GetEntry		M37_SW_GetEntry
#	endif
	extern void M37_GetEntry(LL_ENTRY* drvP);
#endif

#endif /* _LL_DRV_ */

/*-----------------------------------------+
|  BACKWARD COMPATIBILITY TO MDIS4         |
+-----------------------------------------*/
#ifndef U_INT32_OR_64
 /* we have an MDIS4 men_types.h and mdis_api.h included */
 /* only 32bit compatibility needed!                     */
 #define INT32_OR_64  int32
 #define U_INT32_OR_64 u_int32
 typedef INT32_OR_64  MDIS_PATH;
#endif /* U_INT32_OR_64 */

#ifdef __cplusplus
      }
#endif

#endif /* _M37_DRV_H */

