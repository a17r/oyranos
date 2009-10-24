/** @file oyranos_gamma.c
 *
 *  Oyranos is an open source Colour Management System 
 *
 *  @par Copyright:
 *            2005-2009 (C) Kai-Uwe Behrmann
 *
 *  @brief    gamma loader
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2005/02/03
 *
 * It reads the default profile(s) from the Oyranos CMS and recalls this
 * profile(s)
 * as new default profile for a screen, including a possible curves upload to
 * the video card.
 * Currently You need xcalib installed to do the curves upload.
 * 
 */


#include <lcms.h>
#include <icc34.h>

#include "oyranos.h"
#include "oyranos_alpha.h"
#include "oyranos_debug.h"
#include "oyranos_internal.h"
#include "oyranos_config.h"
#include "oyranos_version.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void* oyAllocFunc(size_t size) {return malloc (size);}
void  oyDeAllocFunc ( oyPointer ptr) { if(ptr) free (ptr); }


oyProfile_s *      createProfile     ( float             gamma,
                                       float rx, float ry,
                                       float gx, float gy,
                                       float bx, float by,
                                       float wx, float wy)
{
  cmsCIExyYTRIPLE p;
  LPGAMMATABLE g[3];
  /* 0.31271, 0.32902 D65 */
  cmsCIExyY wtpt_xyY;
  cmsHPROFILE lp = 0;
  size_t size = 0;
  char * data = 0;

  int error = 0;
  oyProfile_s * prof = 0;

  p.Red.x = rx; 
  p.Red.y = ry;
  p.Green.x = gx;
  p.Green.y = gy;
  p.Blue.x = bx;
  p.Blue.y = by;
  wtpt_xyY.x = wx;
  wtpt_xyY.y = wy;
  wtpt_xyY.Y = 1.0;
  g[0] = g[1] = g[2] = cmsBuildGamma(1, gamma);
  lp = cmsCreateRGBProfile( &wtpt_xyY, &p, g);

  _cmsSaveProfileToMem( lp, 0, &size );
  data = oyAllocFunc( size );
  _cmsSaveProfileToMem( lp, data, &size );
  cmsCloseProfile( lp );
  cmsFreeGamma( g[0] );

  prof = oyProfile_FromMem( size, data, 0,0 );


  error = oyProfile_AddTagText( prof, icSigCopyrightTag,
                                      "no copyright; use freely" );

  oyDeAllocFunc( data ); size = 0;
  return prof;
}

int main( int argc , char** argv )
{
  char *display_name = getenv("DISPLAY");
  char *monitor_profile = 0;
  int error = 0;

  /* the functional switches */
  int erase = 0;
  int list = 0;
  int setup = 0;
  int database = 0;
  char * format = 0;
  char * output = 0;
  int server = 0;

  char *ptr = NULL;
  int x = 0, y = 0;
  char *oy_display_name = NULL;
  oyProfile_s * prof = 0;
  size_t size = 0;
  const char * filename = 0;
  char * data = 0;
  uint32_t n = 0;

#ifdef USE_GETTEXT
  setlocale(LC_ALL,"");
#endif
  oyI18NInit_();

#ifndef __APPLE__
  if(!display_name)
  {
    WARNc_S( _("DISPLAY variable not set: giving up.") );
    error = 1;
    return error;
  }
#endif

  if(getenv("OYRANOS_DEBUG"))
  {
    int value = atoi(getenv("OYRANOS_DEBUG"));
    if(value > 0)
      oy_debug = value;
  }

  /* cut off the screen information */
  if(display_name &&
     (ptr = strchr(display_name,':')) != 0)
    if( (ptr = strchr(ptr, '.')) != 0 )
      ptr[0] = '\000';

/* allow "-opt val" and "-opt=val" syntax */
#define OY_PARSE_INT_ARG( opt ) \
                        if( pos + 1 < argc && argv[pos][i+1] == 0 ) \
                        { opt = atoi( argv[pos+1] ); \
                          if( opt == 0 && strcmp(argv[pos+1],"0") ) \
                            wrong_arg = "-" #opt; \
                          ++pos; \
                          i = 1000; \
                        } else if(argv[pos][i+1] == '=') \
                        { opt = atoi( &argv[pos][i+2] ); \
                          if( opt == 0 && strcmp(&argv[pos][i+2],"0") ) \
                            wrong_arg = "-" #opt; \
                          i = 1000; \
                        } else wrong_arg = "-" #opt; \
                        if(oy_debug) printf(#opt "=%d\n",opt)
#define OY_PARSE_STRING_ARG( opt ) \
                        if( pos + 1 < argc && argv[pos][i+1] == 0 ) \
                        { opt = argv[pos+1]; \
                          if( opt == 0 && strcmp(argv[pos+1],"0") ) \
                            wrong_arg = "-" #opt; \
                          ++pos; \
                          i = 1000; \
                        } else if(argv[pos][i+1] == '=') \
                        { opt = &argv[pos][i+2]; \
                          if( opt == 0 && strcmp(&argv[pos][i+2],"0") ) \
                            wrong_arg = "-" #opt; \
                          i = 1000; \
                        } else wrong_arg = "-" #opt; \
                        if(oy_debug) printf(#opt "=%s\n",opt)

  if(argc != 1)
  {
    int pos = 1, i;
    char *wrong_arg = 0;
    while(pos < argc)
    {
      switch(argv[pos][0])
      {
        case '-':
            for(i = 1; i < strlen(argv[pos]); ++i)
            switch (argv[pos][i])
            {
              case 'e': erase = 1; monitor_profile = 0; break;
              case 'b': database = 1; monitor_profile = 0; break;
              case 'f': OY_PARSE_STRING_ARG(format); monitor_profile = 0; break;
              case 'l': list = 1; monitor_profile = 0; break;
              case 'o': OY_PARSE_STRING_ARG(output); monitor_profile = 0; break;
              case 'x': server = 1; OY_PARSE_INT_ARG( x ); break;
              case 'y': server = 1; OY_PARSE_INT_ARG( y ); break;
              case 'v': oy_debug += 1; break;
              case 's': setup = 1; break; /* implicite -> setup */
              case 'h':
              case '-':
                        if(i == 1)
                        {
                             if(strcmp(&argv[pos][2],"unset") == 0)
                        { erase = 1; monitor_profile = 0; i=100; break; }
                        else if(strcmp(&argv[pos][2],"setup") == 0)
                        { setup = 1; i=100; break; }
                        else if(strcmp(&argv[pos][2],"format") == 0)
                        { OY_PARSE_STRING_ARG(format); break; }
                        else if(strcmp(&argv[pos][2],"database") == 0)
                        { database = 1; monitor_profile = 0; i=100; break; }
                        else if(strcmp(&argv[pos][2],"list") == 0)
                        { list = 1; monitor_profile = 0; i=100; break; }
                        else if(strcmp(&argv[pos][2],"verbose") == 0)
                        { oy_debug += 1; i=100; break; }
                        }
              default:
                        printf("\n");
                        printf("oyranos-monitor v%d.%d.%d %s\n",
                        OYRANOS_VERSION_A,OYRANOS_VERSION_B,OYRANOS_VERSION_C,
                                _("is a colour profile administration tool for monitors"));
                        printf("%s\n",                 _("Usage"));
                        printf("  %s\n",               _("Set new profile:"));
                        printf("      %s -x pos -y pos %s\n", argv[0],
                                                       _("profile name"));
                        printf("\n");
                        printf("  %s\n",               _("Unset profile:"));
                        printf("      %s -e -x pos -y pos\n", argv[0]);
                        printf("\n");
                        printf("  %s\n",               _("Activate profiles:"));
                        printf("      %s\n",           argv[0]);
                        printf("\n");
                        printf("  %s\n",               _("Query server profile:"));
                        printf("      %s -x pos -y pos\n", argv[0]);
                        printf("\n");
                        printf("  %s\n",               _("Query device data base profile:"));
                        printf("      %s -b -x pos -y pos\n", argv[0]);
                        printf("\n");
                        printf("  %s\n",               _("List devices:"));
                        printf("      %s -l\n",        argv[0]);
                        printf("\n");
                        printf("  %s\n",               _("Dump data:"));
                        printf("      %s -f=[edid|icc|edid_icc] -x pos -y pos -o=edid.bin\n", argv[0]);
                        printf("\n");
                        printf("  %s\n",               _("General options:"));
                        printf("      %s\n",           _("-v verbose"));
                        printf("\n");
                        printf(_("For more informations read the man page:"));
                        printf("\n");
                        printf("      man oyranos-monitor\n");
                        exit (0);
                        break;
            }
            break;
        default:
            monitor_profile = argv[pos];
            /* activate all profiles at once */
            /*error = oyActivateMonitorProfiles (display_name); */

            erase = 0;
      }
      if( wrong_arg )
      {
        printf("%s %s\n", _("wrong argument to option:"), wrong_arg);
        exit(1);
      }
      ++pos;
    }
    if(oy_debug) printf( "%s\n", argv[1] );

    if(!erase && !list && !database && !setup && !server)
      setup = 1;

    oy_display_name = oyGetDisplayNameFromPosition( display_name, x,y,
                                                    oyAllocFunc);

    if(!monitor_profile && !erase && !list && !setup)
    {
      if(database)
      {
        filename = oyGetMonitorProfileNameFromDB( oy_display_name, oyAllocFunc);
        prof = oyProfile_FromFile( filename, 0, 0 );
        data = oyProfile_GetMem( prof, &size, 0, oyAllocFunc );
      } else {
        data = oyGetMonitorProfile(oy_display_name, &size, oyAllocFunc);
        prof = oyProfile_FromMem( size, data, 0, 0 );
        filename = oyProfile_GetFileName( prof, -1 );
      }
      if(size && data) oyDeAllocFunc( data ); data = 0;

      printf("%s:%d profile \"%s\" size: %d\n",
             strrchr(__FILE__,'/')?strrchr(__FILE__,'/')+1:__FILE__,__LINE__,
             filename?filename:OY_PROFILE_NONE, (int)size);

      oyProfile_Release( &prof );
    }

    if(format &&
       (strcmp(format,"edid") == 0 ||
        strcmp(format,"icc") == 0 ||
        strcmp(format,"edid_icc") == 0))
    {
      oyConfigs_s * devices = 0;
      oyConfig_s * c = 0;
      icHeader * header = 0;
      oyOptions_s * options = 0;
      oyOption_s * o = 0;
      char * out_name = 0;

      error = oyOptions_SetFromText( &options,
                                     "//" OY_TYPE_STD "/config/command",
                                     "properties", OY_CREATE_NEW );
      error = oyOptions_SetFromText( &options, "//"OY_TYPE_STD"/config/edid",
                                       "1", OY_CREATE_NEW );
      if(server)
        error = oyOptions_SetFromText( &options,
                                       "//"OY_TYPE_STD"/config/device_name",
                                       oy_display_name, OY_CREATE_NEW );
      else
        error = oyOptions_SetFromText( &options,
                                       "//"OY_TYPE_STD"/config/display_name",
                                       display_name, OY_CREATE_NEW );

      error = oyDevicesGet( 0, "monitor", options, &devices );

      n = oyConfigs_Count( devices );
      if(!error)
      {
        for(i = 0; i < n; ++i)
        {
          c = oyConfigs_Get( devices, i );

          if(strcmp(format,"edid_icc") == 0)
          {
            o = oyConfig_Find( c, "edid1_colours" );

            prof = createProfile(
                    oyOption_GetValueDouble(o,8),
                    oyOption_GetValueDouble(o,0), oyOption_GetValueDouble(o,1),
                    oyOption_GetValueDouble(o,2), oyOption_GetValueDouble(o,3),
                    oyOption_GetValueDouble(o,4), oyOption_GetValueDouble(o,5),
                    oyOption_GetValueDouble(o,6), oyOption_GetValueDouble(o,7));
            oyOption_Release( &o );
            error = oyProfile_AddTagText( prof, icSigProfileDescriptionTag,
                                          (char*) output ? output : format );
            error = oyProfile_AddTagText( prof, icSigDeviceMfgDescTag,
                                  oyConfig_FindString( c, "manufacturer", 0 ) );
            error = oyProfile_AddTagText( prof, icSigDeviceModelDescTag,
                                  oyConfig_FindString( c, "model", 0 ) );
            data = oyProfile_GetMem( prof, &size, 0, oyAllocFunc );
            header = (icHeader*) data;
            o = oyConfig_Find( c, "mnft" );
            sprintf( (char*)&header->manufacturer, "%s",
                     oyConfig_FindString( c, "mnft", 0 ) );
          } else
          if(strcmp(format,"edid") == 0)
          {
            o = oyConfig_Find( c, "edid" );
            data = oyOption_GetData( o, &size, oyAllocFunc );
          } else
          if(strcmp(format,"icc") == 0)
          {
            oyDeviceAskProfile( c, &prof );
            data = oyProfile_GetMem( prof, &size, 0, oyAllocFunc );
          }

          error = oyWriteMemToFile2_( output ? output : format,
                                      data, size, 0x02,
                                      &out_name, oyAllocFunc );
          oyDeAllocFunc( data ); size = 0;
          if(!error)
          { if(oy_debug) printf( "  written to %s\n", out_name ); }
          else
            printf("Could not write to %s\n", out_name?out_name:format);
          if(out_name) oyDeAllocFunc(out_name); out_name = 0;

          oyProfile_Release( &prof );
          oyOption_Release( &o );
          oyConfig_Release( &c );
        }
      }
      oyConfigs_Release( &devices );
    }


    if(list)
    {
      char * text = 0;
      uint32_t n = 0, i;
      oyConfigs_s * devices = 0;
      oyConfig_s * c = 0;
      oyOptions_s * options = 0;
      oyOption_s * o = 0;

      error = oyOptions_SetFromText( &options,
                                     "//" OY_TYPE_STD "/config/command",
                                     "properties", OY_CREATE_NEW );
      error = oyDevicesGet( 0, "monitor", options, &devices );


      n = oyConfigs_Count( devices );
      if(!error)
      {
        for(i = 0; i < n; ++i)
        {
          c = oyConfigs_Get( devices, i );

          if(oy_debug)
          printf("------------------------ %d ---------------------------\n",i);

          error = oyDeviceGetInfo( c, oyNAME_NICK, 0, &text, oyAllocFunc );
          printf("\"%s\" ", text? text:"???");
          error = oyDeviceGetInfo( c, oyNAME_NAME, 0, &text, oyAllocFunc );
          printf("%s\n", text? text:"???");

          if(oy_debug)
          {
            error = oyDeviceGetInfo( c, oyNAME_DESCRIPTION, 0, &text, oyAllocFunc );
            printf("%s\n", text?  text:"???");
          }

          if(text)
            free( text );

          /* verbose adds */
          if(oy_debug)
          {
            o = oyConfig_Find( c, "edid_colours_text.Xorg" );
            text = oyOption_GetValueText( o, oyAllocFunc );
            printf( "%s:%d \"edid_colours_text.Xorg\":\n%s\n",
                 strchr(__FILE__,'/')?strrchr(__FILE__,'/')+1:__FILE__,__LINE__,
                    text?text:"----" );
            if(text) oyDeAllocFunc( text ); text = 0;
            oyOption_Release( &o );

            oyDeviceAskProfile( c, &prof );
            data = oyProfile_GetMem( prof, &size, 0, oyAllocFunc);
            if(size && data)
              oyDeAllocFunc( data );
            filename = oyProfile_GetFileName( prof, -1 );
            printf( "%s:%d server profile \"%s\" size: %d\n",
                 strchr(__FILE__,'/')?strrchr(__FILE__,'/')+1:__FILE__,__LINE__,
                    filename?filename:OY_PROFILE_NONE, (int)size );

            oyDeviceProfileFromDB( c, &text, oyAllocFunc );
            printf( "%s:%d DB profile \"%s\"\n  DB registration key set: %s\n",
                 strchr(__FILE__,'/')?strrchr(__FILE__,'/')+1:__FILE__,__LINE__,
                    text?text:OY_PROFILE_NONE,
                    oyConfig_FindString( c, "key_set_name", 0 ) ?
                      oyConfig_FindString( c, "key_set_name", 0 ) :
                      OY_PROFILE_NONE );

            oyProfile_Release( &prof );
            oyDeAllocFunc( text );
          }

          oyConfig_Release( &c );
        }
      }
      oyConfigs_Release( &devices );
    }

    /* make shure the display name is correct including the screen */
    if(monitor_profile)
      oySetMonitorProfile (oy_display_name, monitor_profile);
    if(monitor_profile || erase)
      oySetMonitorProfile (oy_display_name, 0);
  } else
    setup = 1;

  if(setup || monitor_profile)
    error = oyActivateMonitorProfiles (display_name);

  if(oy_display_name)
    oyDeAllocFunc(oy_display_name);

  return error;
}
