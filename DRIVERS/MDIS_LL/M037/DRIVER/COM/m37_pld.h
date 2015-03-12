/***********************  I n c l u d e  -  F i l e  ************************
 *  
 *         Name: m37_pld.h
 *
 *       Author: ls
 *        $Date: 2004/04/15 16:37:45 $
 *    $Revision: 1.2 $
 * 
 *  Description: Prototypes for PLD data array and ident function
 *                      
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: m37_pld.h,v $
 * Revision 1.2  2004/04/15 16:37:45  cs
 * Minor modifications for MDIS4/2004 conformity
 *       added swapped access variant
 *
 * Revision 1.1  1999/05/11 14:31:55  Schoberl
 * Initial Revision
 *
 * Revision 
 * 
 *
  *---------------------------------------------------------------------------
 * (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

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
