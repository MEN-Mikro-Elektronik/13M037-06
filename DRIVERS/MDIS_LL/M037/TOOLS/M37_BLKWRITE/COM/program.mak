#***************************  M a k e f i l e  *******************************
#
#         Author: ls
#          $Date: 2004/04/15 16:37:57 $
#      $Revision: 1.2 $
#
#    Description: Makefile definitions for M37 tool
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.2  2004/04/15 16:37:57  cs
#   Minor modifications for MDIS4/2004 conformity
#         removed MAK_OPTIM=$(OPT_1)
#         added mdis_err.h to MAK_INC
#
#   Revision 1.1  1999/05/11 14:32:02  Schoberl
#   Initial Revision
#
#   Revision 1.1  1999/03/10 10:04:10  Schmidt
#   Initial Revision
#
#   Revision 1.1  1999/03/10 10:01:58  Schmidt
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 1998 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=m37_blkwrite

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)    \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)     \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_utl$(LIB_SUFFIX)     \

MAK_INCL=$(MEN_INC_DIR)/m37_drv.h     \
         $(MEN_INC_DIR)/men_typs.h    \
         $(MEN_INC_DIR)/mdis_api.h    \
         $(MEN_INC_DIR)/usr_oss.h     \
         $(MEN_INC_DIR)/usr_utl.h     \

MAK_INP1=m37_blkwrite$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
