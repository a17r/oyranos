/** @file oyCMMapi7_s_.c

   [Template file inheritance graph]
   +-> oyCMMapi7_s_.template.c
   |
   +-> oyCMMapiFilter_s_.template.c
   |
   +-> oyCMMapi_s_.template.c
   |
   +-- Base_s_.c

 *  Oyranos is an open source Color Management System
 *
 *  @par Copyright:
 *            2004-2016 (C) Kai-Uwe Behrmann
 *
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD - see: http://www.opensource.org/licenses/BSD-3-Clause
 */



  
#include "oyCMMapi7_s.h"
#include "oyCMMapi7_s_.h"



#include "oyCMMapi_s_.h"
#include "oyCMMapiFilter_s_.h"



#include "oyObject_s.h"
#include "oyranos_object_internal.h"
#include "oyranos_module_internal.h"



#include "oyCMMapiFilter_s_.h"
  


/* Include "CMMapi7.private_custom_definitions.c" { */
/** Function    oyCMMapi7_Release__Members
 *  @memberof   oyCMMapi7_s
 *  @brief      Custom CMMapi7 destructor
 *  @internal
 *
 *  This function will free up all memmory allocated by the
 *  input object. First all object members witch have their
 *  own release method are deallocated. Then the deallocateFunc_
 *  of the oy_ object is used to release the rest of the members
 *  that were allocated with oy_->allocateFunc_.
 *
 *  @param[in]  cmmapi7  the CMMapi7 object
 *
 *  @version Oyranos: x.x.x
 *  @since   YYYY/MM/DD (Oyranos: x.x.x)
 *  @date    YYYY/MM/DD
 */
void oyCMMapi7_Release__Members( oyCMMapi7_s_ * cmmapi7 )
{
  /* Deallocate members here
   * E.g: oyXXX_Release( &cmmapi7->member );
   */

  if(cmmapi7->oy_->deallocateFunc_)
  {
#if 0
    oyDeAlloc_f deallocateFunc = cmmapi7->oy_->deallocateFunc_;
#endif

    /* Deallocate members of basic type here
     * E.g.: deallocateFunc( cmmapi7->member );
     */
  }
}

/** Function    oyCMMapi7_Init__Members
 *  @memberof   oyCMMapi7_s
 *  @brief      Custom CMMapi7 constructor 
 *  @internal
 *
 *  This function will allocate all memmory for the input object.
 *  For the basic member types this is done using the allocateFunc_
 *  of the attatced (oyObject_s)oy_ object.
 *
 *  @param[in]  cmmapi7  the CMMapi7 object
 *
 *  @version Oyranos: x.x.x
 *  @since   YYYY/MM/DD (Oyranos: x.x.x)
 *  @date    YYYY/MM/DD
 */
int oyCMMapi7_Init__Members( oyCMMapi7_s_ * cmmapi7 )
{
  return 0;
}

/** Function    oyCMMapi7_Copy__Members
 *  @memberof   oyCMMapi7_s
 *  @brief      Custom CMMapi7 copy constructor
 *  @internal
 *
 *  This function makes a copy of all values from the input
 *  to the output object. The destination object and all of its
 *  members should already be allocated.
 *
 *  @param[in]   src  the oyCMMapi7_s_ input object
 *  @param[out]  dst  the output oyCMMapi7_s_ object
 *
 *  @version Oyranos: x.x.x
 *  @since   YYYY/MM/DD (Oyranos: x.x.x)
 *  @date    YYYY/MM/DD
 */
int oyCMMapi7_Copy__Members( oyCMMapi7_s_ * dst, oyCMMapi7_s_ * src)
{
#if 0
  oyAlloc_f allocateFunc_ = 0;
  oyDeAlloc_f deallocateFunc_ = 0;
#endif

  if(!dst || !src)
    return 1;

#if 0
  allocateFunc_ = dst->oy_->allocateFunc_;
  deallocateFunc_ = dst->oy_->deallocateFunc_;
#endif

  /* Copy each value of src to dst here */

  return 0;
}

/* } Include "CMMapi7.private_custom_definitions.c" */



static int oy_cmmapi7_init_ = 0;
static const char * oyCMMapi7_StaticMessageFunc_ (
                                       oyPointer           obj,
                                       oyNAME_e            type,
                                       int                 flags )
{
  oyCMMapi7_s_ * s = (oyCMMapi7_s_*) obj;
  static char * text = 0;
  static int text_n = 0;
  oyAlloc_f alloc = oyAllocateFunc_;

  /* silently fail */
  if(!s)
   return "";

  if(s->oy_ && s->oy_->allocateFunc_)
    alloc = s->oy_->allocateFunc_;

  if( text == NULL || text_n == 0 )
  {
    text_n = 128;
    text = (char*) alloc( text_n );
    if(text)
      memset( text, 0, text_n );
  }

  if( text == NULL || text_n == 0 )
    return "Memory problem";

  text[0] = '\000';

  if(!(flags & 0x01))
    sprintf(text, "%s%s", oyStructTypeToText( s->type_ ), type != oyNAME_NICK?" ":"");

  

  
  /* allocate enough space */
  if(text_n < 1000)
  {
    oyDeAlloc_f dealloc = oyDeAllocateFunc_;
    if(s->oy_ && s->oy_->deallocateFunc_)
      dealloc = s->oy_->deallocateFunc_;
    if(text && text_n)
      dealloc( text );
    text_n = 1024;
    text = alloc(text_n);
    if(text)
      text[0] = '\000';
    else
      return "Memory Error";

    if(!(flags & 0x01))
      sprintf(text, "%s%s", oyStructTypeToText( s->type_ ), type != oyNAME_NICK?" ":"");
  }

  if(type == oyNAME_NICK && (flags & 0x01))
  {
    sprintf( &text[strlen(text)], "%s",
             s->registration
           );
  } else
  if(type == oyNAME_NAME)
    sprintf( &text[strlen(text)], "%s %d/%d",
             s->registration, s->plugs_n, s->sockets_n
           );
  else
  if((int)type >= oyNAME_DESCRIPTION)
  {
    int i = 0;
    sprintf( &text[strlen(text)], "reg: %s\nid: %s\nplugs: %d sockets: %d context: %s",
             s->registration, s->id_,
             s->plugs_n, s->sockets_n, s->context_type
           );
    if(s->properties)
    while(s->properties[i])
    {
      if(i == 0)
        sprintf( &text[strlen(text)], "\nproperties: "
           );
      sprintf( &text[strlen(text)], "%s ",
               s->properties[i++]
           );
    }
  }


  return text;
}
/** @internal
 *  Function oyCMMapi7_New_
 *  @memberof oyCMMapi7_s_
 *  @brief   allocate a new oyCMMapi7_s_  object
 *
 *  @version Oyranos: 
 *  @since   2010/04/26 (Oyranos: 0.1.10)
 *  @date    2010/04/26
 */
oyCMMapi7_s_ * oyCMMapi7_New_ ( oyObject_s object )
{
  /* ---- start of common object constructor ----- */
  oyOBJECT_e type = oyOBJECT_CMM_API7_S;
  int error = 0;
  oyObject_s    s_obj = oyObject_NewFrom( object );
  oyCMMapi7_s_ * s = 0;

  if(s_obj)
    s = (oyCMMapi7_s_*)s_obj->allocateFunc_(sizeof(oyCMMapi7_s_));

  if(!s || !s_obj)
  {
    WARNc_S(_("MEM Error."));
    return NULL;
  }

  error = !memset( s, 0, sizeof(oyCMMapi7_s_) );
  if(error)
    WARNc_S( "memset failed" );

  memcpy( s, &type, sizeof(oyOBJECT_e) );
  s->copy = (oyStruct_Copy_f) oyCMMapi7_Copy;
  s->release = (oyStruct_Release_f) oyCMMapi7_Release;

  s->oy_ = s_obj;

  
  
  
  /* ---- start of custom CMMapi constructor ----- */
  error += !oyObject_SetParent( s_obj, oyOBJECT_CMM_API_S, (oyPointer)s );
  /* ---- end of custom CMMapi constructor ------- */
  /* ---- start of custom CMMapiFilter constructor ----- */
  error += !oyObject_SetParent( s_obj, oyOBJECT_CMM_API_FILTER_S, (oyPointer)s );
  /* ---- end of custom CMMapiFilter constructor ------- */
  /* ---- start of custom CMMapi7 constructor ----- */
  error += !oyObject_SetParent( s_obj, oyOBJECT_CMM_API7_S, (oyPointer)s );
  /* ---- end of custom CMMapi7 constructor ------- */
  
  
  /* ---- end of common object constructor ------- */
  if(error)
    WARNc_S( "oyObject_SetParent failed" );


  
  

  
  
  
  /* ---- start of custom CMMapi constructor ----- */
  error += oyCMMapi_Init__Members( (oyCMMapi_s_*)s );
  /* ---- end of custom CMMapi constructor ------- */
  /* ---- start of custom CMMapiFilter constructor ----- */
  error += oyCMMapiFilter_Init__Members( (oyCMMapiFilter_s_*)s );
  /* ---- end of custom CMMapiFilter constructor ------- */
  /* ---- start of custom CMMapi7 constructor ----- */
  error += oyCMMapi7_Init__Members( s );
  /* ---- end of custom CMMapi7 constructor ------- */
  
  

  if(!oy_cmmapi7_init_)
  {
    oy_cmmapi7_init_ = 1;
    oyStruct_RegisterStaticMessageFunc( type,
                                        oyCMMapi7_StaticMessageFunc_ );
  }

  if(error)
    WARNc1_S("%d", error);

  if(oy_debug)
    oyObject_GetId( s->oy_ );

  return s;
}

/** @internal
 *  Function oyCMMapi7_Copy__
 *  @memberof oyCMMapi7_s_
 *  @brief   real copy a CMMapi7 object
 *
 *  @param[in]     cmmapi7                 CMMapi7 struct object
 *  @param         object              the optional object
 *
 *  @version Oyranos: 
 *  @since   2010/04/26 (Oyranos: 0.1.10)
 *  @date    2010/04/26
 */
oyCMMapi7_s_ * oyCMMapi7_Copy__ ( oyCMMapi7_s_ *cmmapi7, oyObject_s object )
{
  oyCMMapi7_s_ *s = 0;
  int error = 0;

  if(!cmmapi7 || !object)
    return s;

  s = (oyCMMapi7_s_*) oyCMMapi7_New( object );
  error = !s;

  if(!error) {
    
    
    
    /* ---- start of custom CMMapi copy constructor ----- */
    error = oyCMMapi_Copy__Members( (oyCMMapi_s_*)s, (oyCMMapi_s_*)cmmapi7 );
    /* ---- end of custom CMMapi copy constructor ------- */
    /* ---- start of custom CMMapiFilter copy constructor ----- */
    error = oyCMMapiFilter_Copy__Members( (oyCMMapiFilter_s_*)s, (oyCMMapiFilter_s_*)cmmapi7 );
    /* ---- end of custom CMMapiFilter copy constructor ------- */
    /* ---- start of custom CMMapi7 copy constructor ----- */
    error = oyCMMapi7_Copy__Members( s, cmmapi7 );
    /* ---- end of custom CMMapi7 copy constructor ------- */
    
    
    
    
  }

  if(error)
    oyCMMapi7_Release_( &s );

  return s;
}

/** @internal
 *  Function oyCMMapi7_Copy_
 *  @memberof oyCMMapi7_s_
 *  @brief   copy or reference a CMMapi7 object
 *
 *  @param[in]     cmmapi7                 CMMapi7 struct object
 *  @param         object              the optional object
 *
 *  @version Oyranos: 
 *  @since   2010/04/26 (Oyranos: 0.1.10)
 *  @date    2010/04/26
 */
oyCMMapi7_s_ * oyCMMapi7_Copy_ ( oyCMMapi7_s_ *cmmapi7, oyObject_s object )
{
  oyCMMapi7_s_ *s = cmmapi7;

  if(!cmmapi7)
    return 0;

  if(cmmapi7 && !object)
  {
    s = cmmapi7;
    
    oyObject_Copy( s->oy_ );
    return s;
  }

  s = oyCMMapi7_Copy__( cmmapi7, object );

  return s;
}
 
/** @internal
 *  Function oyCMMapi7_Release_
 *  @memberof oyCMMapi7_s_
 *  @brief   release and possibly deallocate a CMMapi7 object
 *
 *  @param[in,out] cmmapi7                 CMMapi7 struct object
 *
 *  @version Oyranos: 
 *  @since   2010/04/26 (Oyranos: 0.1.10)
 *  @date    2010/04/26
 */
int oyCMMapi7_Release_( oyCMMapi7_s_ **cmmapi7 )
{
  /* ---- start of common object destructor ----- */
  oyCMMapi7_s_ *s = 0;

  if(!cmmapi7 || !*cmmapi7)
    return 0;

  s = *cmmapi7;

  *cmmapi7 = 0;

  if(oyObject_UnRef(s->oy_))
    return 0;
  /* ---- end of common object destructor ------- */

  
  
  
  /* ---- start of custom CMMapi destructor ----- */
  oyCMMapi_Release__Members( (oyCMMapi_s_*)s );
  /* ---- end of custom CMMapi destructor ------- */
  /* ---- start of custom CMMapiFilter destructor ----- */
  oyCMMapiFilter_Release__Members( (oyCMMapiFilter_s_*)s );
  /* ---- end of custom CMMapiFilter destructor ------- */
  /* ---- start of custom CMMapi7 destructor ----- */
  oyCMMapi7_Release__Members( s );
  /* ---- end of custom CMMapi7 destructor ------- */
  
  



  if(s->oy_->deallocateFunc_)
  {
    oyDeAlloc_f deallocateFunc = s->oy_->deallocateFunc_;

    oyObject_Release( &s->oy_ );

    deallocateFunc( s );
  }

  return 0;
}



/* Include "CMMapi7.private_methods_definitions.c" { */

/* } Include "CMMapi7.private_methods_definitions.c" */

