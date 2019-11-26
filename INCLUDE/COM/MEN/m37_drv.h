/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: m37_drv.h
 *
 *       Author: ls
 *
 *  Description: Header file for M37 driver
 *               - M37 specific status codes
 *               - M37 function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
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

