/** @file oyranos_string.c
 *
 *  Oyranos is an open source Color Management System 
 *
 *  @par Copyright:
 *            2004-2014 (C) Kai-Uwe Behrmann
 *
 *  @brief    pure string handling functions
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/BSD-3-Clause>
 *  @since    2010/11/16
 */

#include <sys/stat.h>
#include <stddef.h>           /* size_t ptrdiff_t */
#include <stdlib.h>
#include <stdint.h>           /* uint64_t uintptr_t */
#include <stdio.h>
#include <string.h>
#include <iconv.h>

#include "oyranos_config_internal.h"
#include "oyranos.h"
#include "oyranos_debug.h"
#include "oyranos_helper.h"
#include "oyranos_internal.h"
#include "oyranos_io.h"
#include "oyranos_sentinel.h"
#include "oyranos_string.h"

/* --- Helpers  --- */

/* --- static variables   --- */

/* --- structs, typedefs, enums --- */

/* --- internal API definition --- */

/* separate from the external functions */


int          oyStringToDouble        ( const char        * text,
                                       double            * value )
{
  char * p = 0, * t;
  char * save_locale = 0;
  int len = strlen(text);
  int found = 1;
  save_locale = oyStringCopy_( setlocale(LC_NUMERIC, 0 ), oyAllocateFunc_);
  setlocale(LC_NUMERIC, "C");
  /* avoid irritating valgrind output of "Invalid read of size 8"
   * might be a glibc error or a false positive in valgrind */
  t = oyAllocateFunc_( len + 2*sizeof(double) + 1 );
  memset( t, 0, len + 2*sizeof(double) + 1 );

  memcpy( t, text, len );

  if(0 && oy_debug_memory)
  {
    fprintf( stderr, OY_DBG_FORMAT_""OY_PRINT_POINTER" \"%s\" %d "OY_PRINT_POINTER" \"%s\"\n",
            OY_DBG_ARGS_,(ptrdiff_t)text, text, len,
            (ptrdiff_t)t, t  );
    fflush( stderr );
  }

  *value = strtod( t, &p );

  setlocale(LC_NUMERIC, save_locale);

  if(p && p != text && p[0] == '\000')
    found = 0;

  oyFree_m_( t );
  oyFree_m_( save_locale );

  return found;
}


/* string manipulation */

void               oyStringFree_     ( char             ** text,
                                       oyDeAlloc_f         deallocFunc )
{
  if(text)
  {
    if(deallocFunc)
      deallocFunc(*text);
    *text = NULL;
  }
}

char*              oyStringAppendN_  ( const char        * text,
                                       const char        * append,
                                       int                 append_len,
                                       oyAlloc_f           allocateFunc )
{
  char * text_copy = NULL;
  int text_len = 0;

  if(text)
    text_len = oyStrlen_(text);

  if(text_len || append_len)
  {
    oyAllocHelper_m_( text_copy, char,
                      text_len + append_len + 1,
                      allocateFunc, return 0 );

    if(text_len)
      memcpy( text_copy, text, text_len );

    if(append_len)
    {
      if(0 && oy_debug_memory)
      {
        printf( OY_DBG_FORMAT_""OY_PRINT_POINTER" \"%s\" %d %d "OY_PRINT_POINTER" \"%s\"\n",
                OY_DBG_ARGS_,(ptrdiff_t)text_copy, text_copy, text_len,
                append_len, (ptrdiff_t)append,append  );
        fflush( stdout );
      }
      memcpy( &text_copy[text_len], append, append_len );
    }

    text_copy[text_len+append_len] = '\000';
  }

  return text_copy;
}

/** @internal 
 *  @brief append a string and care about allocation
 *
 *  @since Oyranos: version 0.1.8
 *  @date  26 november 2007 (API 0.1.8)
 */
char*              oyStringAppend_   ( const char        * text,
                                       const char        * append,
                                       oyAlloc_f           allocateFunc )
{
  char * text_copy = NULL;
  int append_len = 0;

  if(append)
    append_len = oyStrlen_(append);

  text_copy = oyStringAppendN_(text, append, append_len, allocateFunc);

  return text_copy;
}

/** @internal 
 *  @brief   search in a data pointer for text
 *
 *  Extract only the visible text. Stop for any other non print or space sign. 
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/01/06 (Oyranos: 0.1.10)
 *  @date    2009/01/06
 */
int                oyStringFromData_ ( const oyPointer     ptr,
                                       size_t              size,
                                       char             ** text_new,
                                       size_t            * text_new_size,
                                       oyAlloc_f           allocateFunc )
{
  const char * text = (const char*) ptr;
  char * text_tmp = 0;
  int j = 0,
      error = 0;

  if(ptr && size)
  {
    while(j < size)
      if(!isprint( text[j] ) && !isspace( text[j] ))
      {
        text = 0;
        break;
      } else
        ++j;

    if(text)
    {
      text_tmp = allocateFunc( j );
      error = !text_tmp;
      if(!error)
      {
        error = !memcpy( text_tmp, text, j-1 );
        if(error)
          text_tmp[j-1] = 0;
      }
    }
  }

  if(!error && text)
  {
    *text_new = text_tmp;
    *text_new_size = j - 1;
  }

  return error;
}

void               oyStringAddN_     ( char             ** text,
                                       const char        * append,
                                       int                 append_len,
                                       oyAlloc_f           allocateFunc,
                                       oyDeAlloc_f         deallocFunc )
{
  char * text_copy = NULL;

  text_copy = oyStringAppendN_(*text, append, append_len, allocateFunc);

  if(text && *text && deallocFunc)
    deallocFunc(*text);

  *text = text_copy;

  return;
}

/** @internal 
 *  @brief add a string and care about de-/allocation
 *
 *  @since Oyranos: version 0.1.8
 *  @date  26 november 2007 (API 0.1.8)
 */
void               oyStringAdd_      ( char             ** text,
                                       const char        * append,
                                       oyAlloc_f           allocateFunc,
                                       oyDeAlloc_f         deallocFunc )
{
  char * text_copy = NULL;

  text_copy = oyStringAppend_(*text, append, allocateFunc);

  if(!deallocFunc) deallocFunc = oyDeAllocateFunc_;

  if(text && *text && deallocFunc)
    deallocFunc(*text);

  *text = text_copy;

  return;
}


char *             oyStrnchr_        ( char              * text,
                                       char                delimiter,
                                       int                 len )
{
  int i = 0;

  while(i < len)
  {
    if(text[i] == delimiter)
      return &text[i];
    else if(text[i] == 0)
      return 0;
    ++i;
  }

  return 0;
}

char *             oyStrnchrN_       ( char              * text,
                                       char                delimiter,
                                       int                 len,
                                       int               * end )
{
  *end = 0;

  while(*end < len)
  {
    if(text[*end] == delimiter)
      return &text[*end];
    else if(text[*end] == 0)
      return 0;
    ++ (*end);
  }

  return 0;
}

int                oyStringCaseCmp_  ( const char        * a,
                                       const char        * b )
{
#ifdef HAVE_POSIX
  return strcasecmp(a,b);
#else
  char * la = oyStringCopy_(a,0),
       * lb = oyStringCopy_(b,0),
       * t;
  int n = strlen(a), i;
  int result = 0;
  /* the following un caseing is portable,
   * still strcasecmp() might be faster? */
  t = la;
  for(i = 0; i < n; ++i)
    if(isalpha(t[i]))
      t[i] = tolower(t[i]);

  n = strlen(b);
  t = lb;
  for(i = 0; i < n; ++i)
    if(isalpha(t[i]))
      t[i] = tolower(t[i]);
  
  result = strcmp( la, lb );
  oyFree_m_(la);
  oyFree_m_(lb);
  return result;
#endif
}

int                oyStringSegments_ ( const char        * text,
                                       char                delimiter )
{
  int n = 0;

  if(text[0] == '\000') return 1;
  if(text[0] == delimiter) ++n;
  do { ++n;
  } while( (text = oyStrchr_(text + 1, delimiter)) );

  return n;
}

int                oyStringSegmentsN_( const char        * text,
                                       int                 len,
                                       char                delimiter )
{
  int n = 0;
  int end = 0;

  if(text[0] == '\000') return 1;
  if(text[0] == delimiter) ++n;
  do {
    ++n;
    len -= end;
  } while( (text = oyStrnchrN_((char*)text + 1, delimiter, len, &end)) &&
           len-end-1 > 0 );

  return n;
}

char *             oyStringSegment_  ( char              * text,
                                       char                delimiter,
                                       int                 segment,
                                       int               * end )
{
  uintptr_t end_pos = 0;
  int i = 0;
  char * t = text;

  for(; i < segment; ++i)
  {
    t = oyStrchr_( t, delimiter );
    if(t) ++t;
  }

  end_pos = (uintptr_t) oyStrchr_(t, delimiter);

  if(end_pos == 0)
  {
    *end = oyStrlen_(t);
    return t;
  }

  *end = (int) (end_pos - (uintptr_t) t);

  return t;
}

char *             oyStringSegmentN_ ( char              * text,
                                       int                 len,
                                       char                delimiter,
                                       int                 segment,
                                       int               * end )
{
  uintptr_t end_pos = (uintptr_t)text;
  int i = 0;
  char * t = text;

  for(; i < segment; ++i)
  {
    t = oyStrchr_( t, delimiter );
    ++t;
  }

  if(len)
  {
    end_pos = (uintptr_t) oyStrnchr_(t, delimiter, text + len - t);

    if(end_pos == 0)
    {
      *end = text + len - t;
      return t;
    }
  }

  *end = (int) (end_pos - (uintptr_t) t);

  return t;
}

int    oyStringSegmentX_             ( const char        * text,
                                       char                delimiter,
                                       int               * count,
                                       int              ** pos,
                                       char             ** max_segment )
{
  int n = 0, i, max_segment_len = 0;
  const char * tmp = text;
  static int int_size = sizeof(int);
  const char * start = text;

  if(tmp[0] == delimiter) ++n;
  do { ++n;
  } while( (tmp = oyStrchr_(tmp + 1, delimiter)) );

  if(n > *pos[0])
  {
    oyDeAllocateFunc_( *pos );
    *pos = oyAllocateFunc_( int_size * (n+1) );
    if(!*pos) return 1;
    *pos[0] = n;
  }

  for(i = 0; i < n; ++i)
  {
        ptrdiff_t len = 0;
        char * end = oyStrchr_(start, delimiter);

        if(end > start)
          len = end - start;
        else if (end == start)
          len = 0;
        else
          len = oyStrlen_(start);

        *pos[i] = (int)((ptrdiff_t)(start - text + len));
        start += len + 1;

    if(len > max_segment_len)
      max_segment_len = len;
  }

  if(!max_segment ||
     ((int)*((uint8_t*)&max_segment[0]) < max_segment_len &&
      (int)*((uint8_t*)&max_segment[0]) != 255))
  {
    *((uint8_t*)&max_segment[0]) = OY_MIN(255, max_segment_len);
    *max_segment = oyAllocateFunc_( *((uint8_t*)&max_segment[0]) );
    if(!*max_segment) return 1;
  }

  return 0;
}

char*              oyStringReplace_  ( const char        * text,
                                       const char        * search,
                                       const char        * replacement,
                                       oyAlloc_f           allocateFunc )
{
  char * t = 0;
  const char * start = text,
             * end = text;
  int s_len = strlen(search);

  if(text && search && replacement)
  while((end = strstr(start,search)) != 0)
  {
    oyStringAddN_( &t, start, end-start, oyAllocateFunc_, oyDeAllocateFunc_ );
    oyStringAdd_( &t, replacement, oyAllocateFunc_, oyDeAllocateFunc_ );
    if(strlen(end) > s_len)
      start = end + s_len;
    else
    {
      if(strstr(start,search) != 0)
        oyStringAdd_( &t, replacement, oyAllocateFunc_, oyDeAllocateFunc_ );
      start = end = end + s_len;
      break;
    }
  }

  if(start && strlen(start))
    oyStringAdd_( &t, start, oyAllocateFunc_, oyDeAllocateFunc_ );

  return t;
}

void               oyStringListAddString_ ( char       *** list,
                                       int               * n,
                                       char             ** string,
                                       oyAlloc_f           allocateFunc,
                                       oyDeAlloc_f         deallocateFunc )
{
  int alt_n = *n;
  char ** tmp = oyStringListCat((const char**)*list, alt_n,
                                    (const char**)string, 1,
                                     n, allocateFunc);

  deallocateFunc(*string); *string = 0;
  oyStringListRelease(list, alt_n, deallocateFunc);

  *list = tmp;
}


/** @internal
 *  @brief   find duplicates with strstr()
 *
 *  @version Oyranos: 0.9.5
 *  @since   2013/12/07 (Oyranos: 0.9.5)
 *  @date    2013/12/07
 */
int                oyStringListHas_  ( const char       ** list,
                                       int                 list_n,
                                       const char        * string )
{
  int n = 0;
  int i;

  for(i = 0; i < list_n; ++i)
  {
    if(list[i] && list[i][0])
    {
      if(string && string[0])
      {
        const char * t = oyStrstr_(list[i], string);
        if(t)
          ++n;
      }
    }
  }

  return n;
}

void               oyStringListReplaceBy (
                                       char             ** list,
                                       int                 list_n,
                                       char *           (* replacer)(const char*, oyAlloc_f),
                                       oyAlloc_f           allocateFunc,
                                       oyDeAlloc_f         deAllocateFunc )
{
  int i;
  if(!deAllocateFunc) deAllocateFunc = oyDeAllocateFunc_;
  for(i =  0; i < list_n; ++i)
  {
    char * t = replacer(list[i], allocateFunc);
    if(list[i]) deAllocateFunc(list[i]);
    list[i] = t;
  }
}

/** @internal
 *  @brief reducing filter
 *
 *  In order to collect all possible matches chain this filter.
 *
 *  @version Oyranos: 0.3.0
 *  @since   2007/12/00 (Oyranos: 0.1.8)
 *  @date    2011/01/31
 */
char**             oyStringListFilter_(const char   ** list,
                                       int             list_n,
                                       const char    * dir_string,
                                       const char    * string,
                                       const char    * suffix,
                                       int           * count,
                                       oyAlloc_f       allocateFunc )
{
  char ** nl = 0;
  int nl_n = 0;
  int i;

  for(i = 0; i < list_n; ++i)
  {
    int b = 1;

    if(list[i] && list[i][0])
    {
      if(dir_string && dir_string[0])
      {
        char * t = oyStrstr_(list[i], dir_string);
        if(t)
          b = (oyStrstr_(t+oyStrlen_(dir_string), OY_SLASH)) ? 1:0;
        else
          b = 0;
        if(!b) continue;
      }

      if(b && string && string[0])
      {
        const char * fn;
        fn = oyStrrchr_( list[i], OY_SLASH_C );
        if(!fn)
          fn = list[i];
        else
          fn++;
        b = (oyStrstr_(fn, string))?1:0;
        if(!b) continue;
      }

      if(b && suffix && suffix[0])
      {
        const char * fn = list[i],
                   * f1 = fn + oyStrlen_(fn) - oyStrlen_(suffix);
        b = oyStrcmp_(f1, suffix) == 0;
        if(!b) continue;
      }
    }

    if(b)
    {
      if(!nl)
        oyAllocHelper_m_( nl, char*, list_n + 1, allocateFunc, return 0);

      nl[nl_n] = oyStringCopy_( list[i], allocateFunc );
      if(nl[nl_n])
        ++nl_n;
    }
  }

  if(count)
    *count = nl_n;

  return nl;
}

/* rename some symbols */
#include "oyranos_db.h"
#include "../oyjl/oyjl_core.c"

int     oyStrlen_( const char * str_ )
{
  if(!str_)
    WARNc_S("string missed");
  return strlen(str_); }
void    oyStrcpy_( char * targ_, const char * src_ )
{
  if(!src_ || !targ_)
    WARNc_S("string missed");
  strcpy(targ_,src_); }
char *  oyStrchr_( const char * str_, char c_ )
{
  if(!str_)
    WARNc_S("string missed");
  return strchr(str_,c_); }
char *  oyStrrchr_( const char * str_, char c_ )
{
  if(!str_)
    WARNc_S("string missed");
  return strrchr(str_,c_); }
char *  oyStrstr_( const char * str1_, const char * str2_ )
{
  if(!str1_ || !str2_)
    WARNc_S("string missed");
  return strstr(str1_,str2_); }
int     oyStrcmp_( const char * str1_, const char * str2_ )
{
  if(!str1_ || !str2_)
    WARNc_S("string missed");
  return strcmp(str1_,str2_); }
char    oyToupper_( char c_ ) { return toupper(c_); }


/** @internal returns the binary lenth of a string */
size_t oyStrblen_            (const char * start)
{
  char *end = NULL;

  if( start )
  {
    end = strchr( start, 0 );

    if( end )
      return end - start + 1;
  }

  return 0;
}


/** @func  oyIconv
 *  @brief convert between codesets
 *
 *  @version Oyranos: 0.1.8
 *  @date    2008/05/27
 *  @since   2008/07/23 (Oyranos: 0.1.8)
 */
int                oyIconv           ( const char        * input,
                                       size_t              len_in,
                                       size_t              len_out,
                                       char              * output,
                                       const char        * from_codeset,
                                       const char        * to_codeset )
{
  int error = 0;

  char * out_txt = output;
  char * in_txt = (char*)input;
  const char * loc_env = 
# ifdef USE_GETTEXT
  setlocale( LC_MESSAGES, 0 )
# else
  0
# endif
  , *loc = to_codeset;
  iconv_t cd;
  size_t size, in_left = len_in, out_left = len_out;

  /* application codeset */
  if(!loc && oy_domain_codeset)
    loc = oy_domain_codeset;
  /* environment codeset */
  if(!loc && loc_env)
  {
    char * loc_tmp = strchr(loc_env, '.');
    if(loc_tmp && strlen(loc_tmp) > 2)
      loc = loc_tmp + 1;
  }
  /* fallback codeset */
  if(!loc)
    loc = "UTF-8";

  if(!from_codeset && !oy_domain_codeset)
  {
    error = !memcpy(output, input, sizeof(char) * OY_MIN(len_in,len_out));
    output[len_out] = 0;
    return error;
  }

  cd = iconv_open( loc, from_codeset ? from_codeset : oy_domain_codeset );
  size = iconv( cd, &in_txt, &in_left, &out_txt, &out_left);
  iconv_close( cd );
  *out_txt = 0;

  if(size == (size_t)-1)
    error = -1;
  else
    error = size;

  return error;
}

/** @func  oyIconvGet
 *  @brief convert between codesets
 *
 *  @version Oyranos: 0.1.13
 *  @date    2010/10/24
 *  @since   2010/10723 (Oyranos: 0.1.13)
 */
int          oyIconvGet              ( const char        * text,
                                       void             ** string,
                                       int               * len,
                                       const char        * encoding_from,
                                       const char        * encoding_to,
                                       oyAlloc_f           alloc )
{
  int error = 0;
    *len = strlen(text) * 4 + 4;
    *string = alloc( *len );
    memset( *string, 0, *len );
    error = oyIconv( text, strlen(text), *len-2, *string,
                     encoding_from, encoding_to );
    if(error)
      WARNc2_S("something went wrong. %s:%d\n", text, *len);
    return error;
}


