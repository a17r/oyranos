/** @file oyranos_module_internal.h

   [Template file inheritance graph]
   +-- oyranos_module_internal.template.h

 *  Oyranos is an open source Colour Management System
 *
 *  @par Copyright:
 *            2004-2012 (C) Kai-Uwe Behrmann
 *
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD - see: http://www.opensource.org/licenses/bsd-license.php
 *  @date     2012/09/23
 */


#ifndef OYRANOS_MODULE_INTERNAL_H
#define OYRANOS_MODULE_INTERNAL_H

#include "oyranos.h"
#include "oyranos_module.h"

#include "oyHash_s.h"
#include "oyCMMapi3_s_.h"
#include "oyCMMapiFilters_s.h"
#include "oyCMMapis_s.h"
#include "oyFilterNodes_s.h"
#include "oyFilterPlug_s.h"
#include "oyFilterPlugs_s.h"

/**
 *  @internal
 *  @enum    oyREQUEST_e
 *  @brief   grade of request complying
 *
 *  @version Oyranos: 0.1.8
 *  @since   2008/00/00 (Oyranos: 0.1.8)
 *  @date    2008/00/00
 */
typedef enum {
  oyREQUEST_NONE,                      /**< deactives the request */
  oyREQUEST_SLIGTH,
  oyREQUEST_MID,
  oyREQUEST_MUCH,
  oyREQUEST_HARD                       /**< requirement, fail if not present */
} oyREQUEST_e;

/**
 *  @internal
 *  @struct oyCMMapiQuery_s
 *  @brief  check/prepare a module
 *
 *  @since  Oyranos: version 0.1.8
 *  @date   2008/01/03 (API 0.1.8)
 */
typedef struct {
  oyCMMQUERY_e         query;
  uint32_t             value;
  oyREQUEST_e          request;
} oyCMMapiQuery_s;

/**
 *  @internal
 *  @struct oyCMMapiQuerie_s
 *  @brief  check/prepare a module
 *
 *  @since  Oyranos: version 0.1.8
 *  @date   2008/01/03 (API 0.1.8)
 */
typedef struct {
  int                  n;
  oyCMMapiQuery_s   ** queries;
  char                 prefered_cmm[5];
} oyCMMapiQueries_s;

oyCMMapiFilters_s * oyCMMsGetFilterApis_(const char        * cmm_meta,
                                         const char        * cmm_required,
                                         const char        * registration,
                                         oyOBJECT_e          type,
                                         uint32_t            flags,
                                         uint32_t         ** rank_list,
                                         uint32_t          * count );
oyCMMapi_s *     oyCMMsGetApi__      ( oyOBJECT_e          type,
                                       const char        * lib_name,
                                       oyCMMapi_Check_f    apiCheck,
                                       oyPointer           check_pointer,
                                       int                 num );
oyCMMapi_s *     oyCMMsGetApi_       ( oyOBJECT_e          type,
                                       const char        * cmm_required,
                                       char             ** lib_used,
                                       oyCMMapi_Check_f    apiCheck,
                                       oyPointer           check_pointer );
oyHash_s *   oyCMMCacheListGetEntry_ ( const char        * hash_text );
oyCMMapis_s *  oyCMMGetMetaApis_     ( const char        * cmm );
int              oyCMMCanHandle_    ( oyCMMapi3_s        * api,
                                      oyCMMapiQueries_s  * queries );
oyCMMapis_s *    oyCMMsGetMetaApis_  ( const char        * cmm );
char **          oyCMMsGetNames_     ( uint32_t          * n,
                                       const char        * sub_path,
                                       const char        * ext,
                                       const char        * required_cmm,
                                       oyPATH_TYPE_e       path_type );
int          oyCMMdsoReference_      ( const char        * lib_name,
                                       oyPointer           ptr );
const char * oyStruct_GetTextFromModule (
                                       oyStruct_s        * obj,
                                       oyNAME_e            name_type,
                                       uint32_t            flags );
int    oyIsOfTypeCMMapiFilter        ( oyOBJECT_e          type );

int    oyAdjacencyListAdd_           ( oyFilterPlug_s    * plug,
                                       oyFilterNodes_s   * nodes,
                                       oyFilterPlugs_s   * edges,
                                       const char        * selector,
                                       int                 flags );
int          oyPointer_ConvertData   ( oyPointer_s       * cmm_ptr,
                                       oyPointer_s       * cmm_ptr_out,
                                       oyFilterNode_s    * node );

#endif /* OYRANOS_MODULE_INTERNAL_H */
