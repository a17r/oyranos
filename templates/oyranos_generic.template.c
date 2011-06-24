{% include "source_file_header.txt" %}

#include <string.h>

#include "oyranos_helper_macros.h"
#include "oyranos_helper.h"
#include "oyranos_generic.h"
#include "oyranos_object.h"

#include "oyObject_s.h"
#include "oyHash_s.h"

#include "oyStructList_s_.h"

/** Public function definitions { */
/** } Public function definitions */

/** Private function definitions { */

/** @internal
 *  @brief get always a Oyranos cache entry from a cache list
 *
 *  @param[in]     cache_list          the list to search in
 *  @param[in]     flags               0 - assume text, 1 - assume sized hash
 *  @param[in]     hash_text           the text to search for in the cache_list
 *  @return                            the cache entry may not have a entry
 *
 *  @version Oyranos: 0.1.10
 *  @since   2007/11/24 (Oyranos: 0.1.8)
 *  @date    2009/06/04
 */
oyHash_s *   oyCacheListGetEntry_    ( oyStructList_s    * cache_list,
                                       uint32_t            flags,
                                       const char        * hash_text )
{
  oyHash_s * entry = 0,
           * search_key = 0;
  int error = !(cache_list && hash_text);
  int n = 0, i;
  uint32_t search_int[8] = {0,0,0,0,0,0,0,0};
  char hash_text_copy[32];
  const char * search_ptr = (const char*)search_int;

  if(error <= 0 && cache_list->type_ != oyOBJECT_STRUCT_LIST_S)
    error = 1;

  if(error <= 0)
  {
    if(flags & 0x01)
      search_ptr = hash_text;
    else
    if(oyStrlen_(hash_text) < OY_HASH_SIZE*2-1)
    {
      memset( hash_text_copy, 0, OY_HASH_SIZE*2 );
      memcpy( hash_text_copy, hash_text, oyStrlen_(hash_text) );
      search_ptr = hash_text_copy;
    } else
      search_int[0] = oyMiscBlobGetL3_( (void*)hash_text, oyStrlen_(hash_text));
  }

  if(error <= 0)
    n = oyStructList_Count(cache_list);

  for(i = 0; i < n; ++i)
  {
    oyHash_s * compare = (oyHash_s*) oyStructList_GetType_( (oyStructList_s_*)cache_list, i,
                                                         oyOBJECT_HASH_S );

    if(compare )
    if(memcmp(search_ptr, compare->oy_->hash_ptr_, OY_HASH_SIZE*2) == 0)
    {
      entry = compare;
      return oyHash_Copy( entry, 0 );
    }
  }

  if(error <= 0 && !entry)
  {
    search_key = oyHash_Get(hash_text, 0);
    error = !search_key;

    if(error <= 0)
      entry = oyHash_Copy( search_key, 0 );

    if(error <= 0)
    {
      error = oyStructList_MoveIn(cache_list, (oyStruct_s**)&search_key, -1, 0);
      search_key = 0;
    }

    oyHash_Release( &search_key );
  }


  if(entry)
    return oyHash_Copy( entry, 0 );
  else
    return 0;
}

/** } Private function definitions */
