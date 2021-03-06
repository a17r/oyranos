/** @file oyRectangle_s_.c

   [Template file inheritance graph]
   +-> oyRectangle_s_.template.c
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



  
#include "oyRectangle_s.h"
#include "oyRectangle_s_.h"





#include "oyObject_s.h"
#include "oyranos_object_internal.h"


  


/* Include "Rectangle.private_custom_definitions.c" { */
/** @internal
 *  Function    oyRectangle_Release__Members
 *  @memberof   oyRectangle_s
 *  @brief      Custom Rectangle destructor
 *  @internal
 *
 *  This function will free up all memmory allocated by the
 *  input object. First all object members witch have their
 *  own release method are deallocated. Then the deallocateFunc_
 *  of the oy_ object is used to release the rest of the members
 *  that were allocated with oy_->allocateFunc_.
 *
 *  @param[in]  rectangle  the Rectangle object
 *
 *  @version Oyranos: x.x.x
 *  @since   YYYY/MM/DD (Oyranos: x.x.x)
 *  @date    YYYY/MM/DD
 */
void oyRectangle_Release__Members( oyRectangle_s_ * rectangle )
{
  /* Deallocate members here
   * E.g: oyXXX_Release( &rectangle->member );
   */

  if(rectangle->oy_->deallocateFunc_)
  {
    /* Deallocate members of basic type here
     * E.g.: deallocateFunc( rectangle->member );
     */
  }
}

/** @internal
 *  Function    oyRectangle_Init__Members
 *  @memberof   oyRectangle_s
 *  @brief      Custom Rectangle constructor 
 *  @internal
 *
 *  This function will allocate all memmory for the input object.
 *  For the basic member types this is done using the allocateFunc_
 *  of the attatced (oyObject_s)oy_ object.
 *
 *  @param[in]  rectangle  the Rectangle object
 *
 *  @version Oyranos: x.x.x
 *  @since   YYYY/MM/DD (Oyranos: x.x.x)
 *  @date    YYYY/MM/DD
 */
int oyRectangle_Init__Members( oyRectangle_s_ * rectangle )
{
  return 0;
}

/** @internal
 *  Function    oyRectangle_Copy__Members
 *  @memberof   oyRectangle_s
 *  @brief      Custom Rectangle copy constructor
 *  @internal
 *
 *  This function makes a copy of all values from the input
 *  to the output object. The destination object and all of its
 *  members should already be allocated.
 *
 *  @param[in]   src  the oyRectangle_s_ input object
 *  @param[out]  dst  the output oyRectangle_s_ object
 *
 *  @version Oyranos: x.x.x
 *  @since   YYYY/MM/DD (Oyranos: x.x.x)
 *  @date    YYYY/MM/DD
 */
int oyRectangle_Copy__Members( oyRectangle_s_ * dst, oyRectangle_s_ * src)
{
  int error = 0;

  if(!dst || !src)
    return 1;

  /* Copy each value of src to dst here */
  dst->x = src->x;
  dst->y = src->y;
  dst->width = src->width;
  dst->height = src->height;

  return error;
}

/* } Include "Rectangle.private_custom_definitions.c" */



static int oy_rectangle_init_ = 0;
static const char * oyRectangle_StaticMessageFunc_ (
                                       oyPointer           obj,
                                       oyNAME_e            type,
                                       int                 flags )
{
  oyRectangle_s_ * s = (oyRectangle_s_*) obj;
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

  

  
  if(type != oyNAME_NICK || (flags & 0x01))
    sprintf( &text[strlen(text)], "%gx%g+%g+%g", s->width, s->height, s->x, s->y);


  return text;
}
/** @internal
 *  Function oyRectangle_New_
 *  @memberof oyRectangle_s_
 *  @brief   allocate a new oyRectangle_s_  object
 *
 *  @version Oyranos: 
 *  @since   2010/04/26 (Oyranos: 0.1.10)
 *  @date    2010/04/26
 */
oyRectangle_s_ * oyRectangle_New_ ( oyObject_s object )
{
  /* ---- start of common object constructor ----- */
  oyOBJECT_e type = oyOBJECT_RECTANGLE_S;
  int error = 0;
  oyObject_s    s_obj = oyObject_NewFrom( object );
  oyRectangle_s_ * s = 0;

  if(s_obj)
    s = (oyRectangle_s_*)s_obj->allocateFunc_(sizeof(oyRectangle_s_));

  if(!s || !s_obj)
  {
    WARNc_S(_("MEM Error."));
    return NULL;
  }

  error = !memset( s, 0, sizeof(oyRectangle_s_) );
  if(error)
    WARNc_S( "memset failed" );

  memcpy( s, &type, sizeof(oyOBJECT_e) );
  s->copy = (oyStruct_Copy_f) oyRectangle_Copy;
  s->release = (oyStruct_Release_f) oyRectangle_Release;

  s->oy_ = s_obj;

  
  /* ---- start of custom Rectangle constructor ----- */
  error += !oyObject_SetParent( s_obj, oyOBJECT_RECTANGLE_S, (oyPointer)s );
  /* ---- end of custom Rectangle constructor ------- */
  
  
  
  
  /* ---- end of common object constructor ------- */
  if(error)
    WARNc_S( "oyObject_SetParent failed" );


  
  

  
  /* ---- start of custom Rectangle constructor ----- */
  error += oyRectangle_Init__Members( s );
  /* ---- end of custom Rectangle constructor ------- */
  
  
  
  

  if(!oy_rectangle_init_)
  {
    oy_rectangle_init_ = 1;
    oyStruct_RegisterStaticMessageFunc( type,
                                        oyRectangle_StaticMessageFunc_ );
  }

  if(error)
    WARNc1_S("%d", error);

  if(oy_debug)
    oyObject_GetId( s->oy_ );

  return s;
}

/** @internal
 *  Function oyRectangle_Copy__
 *  @memberof oyRectangle_s_
 *  @brief   real copy a Rectangle object
 *
 *  @param[in]     rectangle                 Rectangle struct object
 *  @param         object              the optional object
 *
 *  @version Oyranos: 
 *  @since   2010/04/26 (Oyranos: 0.1.10)
 *  @date    2010/04/26
 */
oyRectangle_s_ * oyRectangle_Copy__ ( oyRectangle_s_ *rectangle, oyObject_s object )
{
  oyRectangle_s_ *s = 0;
  int error = 0;

  if(!rectangle || !object)
    return s;

  s = (oyRectangle_s_*) oyRectangle_New( object );
  error = !s;

  if(!error) {
    
    /* ---- start of custom Rectangle copy constructor ----- */
    error = oyRectangle_Copy__Members( s, rectangle );
    /* ---- end of custom Rectangle copy constructor ------- */
    
    
    
    
    
    
  }

  if(error)
    oyRectangle_Release_( &s );

  return s;
}

/** @internal
 *  Function oyRectangle_Copy_
 *  @memberof oyRectangle_s_
 *  @brief   copy or reference a Rectangle object
 *
 *  @param[in]     rectangle                 Rectangle struct object
 *  @param         object              the optional object
 *
 *  @version Oyranos: 
 *  @since   2010/04/26 (Oyranos: 0.1.10)
 *  @date    2010/04/26
 */
oyRectangle_s_ * oyRectangle_Copy_ ( oyRectangle_s_ *rectangle, oyObject_s object )
{
  oyRectangle_s_ *s = rectangle;

  if(!rectangle)
    return 0;

  if(rectangle && !object)
  {
    s = rectangle;
    
    oyObject_Copy( s->oy_ );
    return s;
  }

  s = oyRectangle_Copy__( rectangle, object );

  return s;
}
 
/** @internal
 *  Function oyRectangle_Release_
 *  @memberof oyRectangle_s_
 *  @brief   release and possibly deallocate a Rectangle object
 *
 *  @param[in,out] rectangle                 Rectangle struct object
 *
 *  @version Oyranos: 
 *  @since   2010/04/26 (Oyranos: 0.1.10)
 *  @date    2010/04/26
 */
int oyRectangle_Release_( oyRectangle_s_ **rectangle )
{
  /* ---- start of common object destructor ----- */
  oyRectangle_s_ *s = 0;

  if(!rectangle || !*rectangle)
    return 0;

  s = *rectangle;

  *rectangle = 0;

  if(oyObject_UnRef(s->oy_))
    return 0;
  /* ---- end of common object destructor ------- */

  
  /* ---- start of custom Rectangle destructor ----- */
  oyRectangle_Release__Members( s );
  /* ---- end of custom Rectangle destructor ------- */
  
  
  
  



  if(s->oy_->deallocateFunc_)
  {
    oyDeAlloc_f deallocateFunc = s->oy_->deallocateFunc_;

    oyObject_Release( &s->oy_ );

    deallocateFunc( s );
  }

  return 0;
}



/* Include "Rectangle.private_methods_definitions.c" { */

/* } Include "Rectangle.private_methods_definitions.c" */

