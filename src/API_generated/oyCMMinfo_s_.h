/** @file oyCMMinfo_s_.h

   [Template file inheritance graph]
   +-> oyCMMinfo_s_.template.h
   |
   +-- Base_s_.h

 *  Oyranos is an open source Colour Management System
 *
 *  @par Copyright:
 *            2004-2013 (C) Kai-Uwe Behrmann
 *
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD - see: http://www.opensource.org/licenses/bsd-license.php
 *  @date     2013/06/10
 */


#ifndef OY_CMM_INFO_S__H
#define OY_CMM_INFO_S__H

#ifdef __cplusplus
/*namespace oyranos {*/
  extern "C" {
#endif /* __cplusplus */


#define oyCMMinfoPriv_m( var ) ((oyCMMinfo_s_*) (var))

typedef struct oyCMMinfo_s_ oyCMMinfo_s_;

  
#include <oyranos_object.h>

#include "oyranos_module.h"
#include "oyCMMapi_s.h"
  

#include "oyCMMinfo_s.h"

/* Include "CMMinfo.private.h" { */
#include "oyranos_module_internal.h"

/* } Include "CMMinfo.private.h" */

/** @internal
 *  @struct   oyCMMinfo_s_
 *  @brief      The CMM API resources struct to implement and set by a CMM
 *  @ingroup  cmm_handling
 *  @extends  oyStruct_s
 */
struct oyCMMinfo_s_ {

/* Include "Struct.members.h" { */
oyOBJECT_e           type_;          /**< @private struct type */
oyStruct_Copy_f      copy;           /**< copy function */
oyStruct_Release_f   release;        /**< release function */
oyObject_s           oy_;            /**< @private features name and hash */

/* } Include "Struct.members.h" */



/* Include "CMMinfo.members.h" { */
  char             cmm[8];             /*!< ICC signature, eg 'lcms' */
  char           * backend_version;    /*!< non translatable, eg "v1.17" */
  /** translated, e.g. "name": "lcms" "little cms" "A CMM with 100k ..."
   *  supported should be "name", "copyright" and "manufacturer".
   *  Optional is "help".
   */
  oyCMMGetText_f   getText;
  char          ** texts;              /**< zero terminated list for getText */

  int              oy_compatibility;   /*!< last supported Oyranos CMM API : OYRANOS_VERSION */
  oyCMMapi_s     * api;                /**< must be casted to a according API, zero terminated list */

  oyIcon_s         icon;               /*!< zero terminated list of a icon pyramid */

  oyCMMinfoInit_f  init;      /**< should be called before any access to members of oyCMMinfo_s */

/* } Include "CMMinfo.members.h" */

};


oyCMMinfo_s_*
  oyCMMinfo_New_( oyObject_s object );
oyCMMinfo_s_*
  oyCMMinfo_Copy_( oyCMMinfo_s_ *cmminfo, oyObject_s object);
oyCMMinfo_s_*
  oyCMMinfo_Copy__( oyCMMinfo_s_ *cmminfo, oyObject_s object);
int
  oyCMMinfo_Release_( oyCMMinfo_s_ **cmminfo );



/* Include "CMMinfo.private_methods_declarations.h" { */

/* } Include "CMMinfo.private_methods_declarations.h" */



void oyCMMinfo_Release__Members( oyCMMinfo_s_ * cmminfo );
int oyCMMinfo_Init__Members( oyCMMinfo_s_ * cmminfo );
int oyCMMinfo_Copy__Members( oyCMMinfo_s_ * dst, oyCMMinfo_s_ * src);


#ifdef __cplusplus
} /* extern "C" */
/*}*/ /* namespace oyranos */
#endif /* __cplusplus */

#endif /* OY_CMM_INFO_S__H */
