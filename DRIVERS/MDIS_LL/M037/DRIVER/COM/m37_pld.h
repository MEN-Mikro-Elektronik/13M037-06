/***********************  I n c l u d e  -  F i l e  ************************
 *  
 *         Name: m37_pld.h
 *
 *       Author: ls
 * 
 *  Description: Prototypes for PLD data array and ident function
 *                      
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 1998-2019, MEN Mikro Elektronik GmbH
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

#ifndef _M37_PLD_H
#define _M37_PLD_H

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef _ONE_NAMESPACE_PER_DRIVER_

        /* variant for swapped access */
#       ifdef ID_SW
#               define M37_PldData             M37_SW_PldData
#               define M37_PldIdent            M37_SW_PldIdent
#       endif

#endif
/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
extern const u_int8 M37_PldData[];

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
char* M37_PldIdent(void);


#ifdef __cplusplus
	}
#endif

#endif	/* _M37_PLD_H */
