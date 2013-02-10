/** @file oyranos_file.cpp
 *
 *  Oyranos is an open source Colour Management System 
 *
 *  @par Copyright:
 *            2011-2013 (C) Kai-Uwe Behrmann
 *
 *  @brief    image file handler
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/bsd-license.php>
 *  @since    2011/04/06
 *
 */
/* c++ -Wall -g oyranos_file.cpp `pkg-config --cflags --libs oyranos` -o oyranos-file */

#include <oyConversion_s.h>
#include <oyranos_devices.h>

#include "oyranos_helper.h"
#include "oyranos_helper_macros_cli.h"
#include "oyranos_i18n.h"
#include "oyranos_string.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern "C" {
typedef struct oyProfile_s_ oyProfile_s_;
int          oyProfile_ToFile_       ( oyProfile_s_      * profile,
                                       const char        * file_name );
void* oyAllocFunc(size_t size) {return malloc (size);}
void  oyDeAllocFunc ( oyPointer ptr) { if(ptr) free (ptr); }
}

void displayHelp(char ** argv)
{
  printf("\n");
  printf("oyranos-camera-raw v%d.%d.%d %s\n",
         OYRANOS_VERSION_A,OYRANOS_VERSION_B,OYRANOS_VERSION_C,
         _("is a CameraRaw conversion tool"));
  printf("%s:\n",                 _("Usage"));
  printf("  %s\n",               _("Convert CameraRaw image to Rgb image:"));
  printf("      %s -c -i file.dng -o file.ppm\n", argv[0]);
  printf("\n");
  printf("  %s\n",               _("Dump data:"));
  printf("      %s -f=[icc|openicc] -i file.dng [-o=device.json] [--only-db]\n", argv[0]);
  printf("         --only-db\t%s\n",_("use only DB keys for -f=openicc"));
  printf("\n");
  printf("  %s\n",               _("Show Help:"));
  printf("      %s [-h]\n", argv[0]);
  printf("\n");
  printf("  %s\n",               _("General options:"));
  printf("         -i %s\t%s\n",    _("FILE"),   _("CameraRaw image"));
  printf("         -o %s\t%s\n",    _("FILE"),   _("write to specified file"));
  printf("         -v      \t%s\n", _("verbose"));
  printf("\n");
  printf(_("For more informations read the man page:"));
  printf("\n");
  printf("      man oyranos-camera-raw\n");
}

/** Function oyConversion_FromImageFileName
 *  @brief   generate a Oyranos graph from a image file name
 *
 *  @param[in]     file_name           name of image file
 *  @param[in]     profile_name        name of ICC profile file
 *  @param[in]     flags               set options
 *                                     - 0x01 - request device
 *                                     - 0x02 - no data processing
 *  @param[in]     data_type           the desired data type for output
 *  @param[in]     obj                 Oyranos object (optional)
 *  @return                            generated new graph, owned by caller
 *
 *  @version Oyranos: 0.3.0
 *  @since   2011/04/06 (Oyranos: 0.3.0)
 *  @date    2011/04/06
 */
oyConversion_s * oyConversion_FromImageFileName  (
                                       const char        * file_name,
                                       const char        * profile_name,
                                       uint32_t            flags,
                                       oyDATATYPE_e        data_type,
                                       oyObject_s          obj )
{
  oyFilterNode_s * in, * out;
  int error = 0;
  oyConversion_s * conversion = 0;
  oyOptions_s * options = 0;

  if(!file_name)
    return NULL;

  /* start with an empty conversion object */
  conversion = oyConversion_New( obj );
  /* create a filter node */
  in = oyFilterNode_NewWith( "//" OY_TYPE_STD "/file_read.meta", 0, obj );
  /* set the above filter node as the input */
  oyConversion_Set( conversion, in, 0 );

  /* add a file name argument */
  /* get the options of the input node */
  if(in)
  options = oyFilterNode_GetOptions( in, OY_SELECT_FILTER );
  /* add a new option with the appropriate value */
  error = oyOptions_SetFromText( &options, "//" OY_TYPE_STD "/file_read/filename",
                                 file_name, OY_CREATE_NEW );
  /* set image process options */
  if(flags & 0x01)
    error = oyOptions_SetFromText( &options,
                                   "//" OY_TYPE_STD "/file_read/device",
                                   "1", OY_CREATE_NEW );

  if(profile_name)
  {
    oyProfile_s * prof = oyProfile_FromFile( profile_name, 0, 0 );
    error = oyOptions_MoveInStruct( &options,
                                "//" OY_TYPE_STD "/config/icc_profile.add_meta",
                                    (oyStruct_s**)&prof, OY_CREATE_NEW );
  }
  if(flags & 0x02)
    error = oyOptions_SetFromText( &options,
                                   "//" OY_TYPE_STD "/file_read/render",
                                   "0", OY_CREATE_NEW );
  /* release the options object, this means its not any more refered from here*/
  oyOptions_Release( &options );

  /* add a closing node */
  out = oyFilterNode_NewWith( "//" OY_TYPE_STD "/output", 0, obj );
  error = oyFilterNode_Connect( in, "//" OY_TYPE_STD "/data",
                                out, "//" OY_TYPE_STD "/data", 0 );
  /* set the output node of the conversion */
  if(!error)
    oyConversion_Set( conversion, 0, out );

  return conversion;
}


int main(int argc, char ** argv)
{
  int error = 0;

  /* the functional switches */
  char * format = 0;
  char * output = 0;
  int only_db = 0;
  char * prof_name = 0,
       * new_profile_name = 0,
       * image_name = 0;
  int verbose = 0;

  int i;
  oyOptions_s * options = 0;
  size_t size = 0;
  char * data = 0;

#ifdef USE_GETTEXT
  setlocale(LC_ALL,"");
#endif
  oyI18NInit_();

  {
    int pos = 1;
    const char *wrong_arg = 0;
    while(pos < argc)
    {
      switch(argv[pos][0])
      {
        case '-':
            for(i = 1; (unsigned int)i < strlen(argv[pos]); ++i)
            switch (argv[pos][i])
            {
              case 'f': OY_PARSE_STRING_ARG(format); break;
              case 'i': OY_PARSE_STRING_ARG(image_name); break;
              case 'o': OY_PARSE_STRING_ARG(output); break;
              case 'p': OY_PARSE_STRING_ARG(prof_name); break;
              case 'v': if(verbose) oy_debug += 1; verbose = 1; break;
              case 'h':
              case '-':
                        if(i == 1)
                        {
                             if(OY_IS_ARG("image"))
                        { OY_PARSE_STRING_ARG2(image_name, "image"); break; }
                        else if(OY_IS_ARG("format"))
                        { OY_PARSE_STRING_ARG2(format, "format"); break; }
                        else if(OY_IS_ARG("output"))
                        { OY_PARSE_STRING_ARG2(output, "output"); break; }
                        else if(OY_IS_ARG("only-db"))
                        { only_db = 1; i=100; break; }
                        else if(OY_IS_ARG("name"))
                        { OY_PARSE_STRING_ARG2(new_profile_name, "name"); break; }
                        else if(OY_IS_ARG("profile"))
                        { OY_PARSE_STRING_ARG2(prof_name, "profile"); break; }
                        else if(OY_IS_ARG("verbose"))
                        { if(verbose) oy_debug += 1; verbose = 1; i=100; break;}
                        }
              default:
                        displayHelp(argv);
                        exit (0);
                        break;
            }
            break;
        default:
                        displayHelp(argv);
                        exit (0);
      }
      if( wrong_arg )
      {
        printf("%s %s\n", _("wrong argument to option:"), wrong_arg);
        exit(1);
      }
      ++pos;
    }
    if(oy_debug) printf( "%s\n", argv[1] );
  }
  if(argc == 1)
  {
                        displayHelp(argv);
                        exit (0);
  }

  oyConversion_s * c = oyConversion_FromImageFileName( image_name, prof_name,
                                  0x01 | 0x04, oyUINT16, 0 );

  oyImage_s * image = oyConversion_GetImage( c, OY_OUTPUT );

  oyConfig_s * device = 0;
  oyOptions_s * image_tags = oyImage_GetTags( image );
  if(image)
    device = (oyConfig_s*)oyOptions_GetType( image_tags, 0,
                                           "device",
                                           oyOBJECT_CONFIG_S );
  oyOption_s * opt = oyConfig_Find( device, "icc_profile.add_meta" );
  oyProfile_s * profile = (oyProfile_s*) oyOption_GetStruct( opt,
                                                         oyOBJECT_PROFILE_S );

  if(format && device)
  {
    char * out_name = 0;
    if(strcmp(format,"icc") == 0)
    {
      if(!profile)
        error = oyDeviceGetProfile( device, 0, &profile );
      if(profile && error < 0)
        /* The profile is a system fallback. skip it */
        oyProfile_Release( &profile );
      if(!profile)
        profile = oyImage_GetProfile( image );

      data = (char*) oyProfile_GetMem( profile, &size, 0, oyAllocFunc);
      if(size && data)
      {
        if(output)
          error = oyWriteMemToFile2_( output,
                                      data, size, 0x01,
                                      &out_name, oyAllocFunc );
        else
          fwrite( data, sizeof(char), size, stdout );

        oyDeAllocFunc( data ); data = 0;

      }
    } else
    if(strcmp(format,"openicc") == 0)
    {
      char * json = 0;

      error = oyOptions_SetFromText( &options, "//" OY_TYPE_STD "/options/source",
                                   "db", OY_CREATE_NEW );  
      error = oyDeviceToJSON( device, options, &json, oyAllocFunc );
      oyOptions_Release( &options );

      /* it is possible that no DB keys are available; use all others */
      if(!json && !only_db)
        error = oyDeviceToJSON( device, NULL, &json, oyAllocFunc );

      if(!json)
      {
        fprintf( stderr, "no DB data available\n" );
        exit(0);
      }

      if(output)
        error = oyWriteMemToFile2_( output,
                                    json, strlen(json), 0x01,
                                    &out_name, oyAllocFunc );
      else
        fprintf( stdout, "%s", json );

      if(json)
        size = strlen(json);
      oyDeAllocFunc( json ); json = 0;
    } else {
      displayHelp(argv);
      exit (1);
    }
  } else
  if(output)
  {
      char * comment = 0;
      STRING_ADD( comment, "source image was " );
      STRING_ADD( comment, image_name );
      oyOptions_SetFromText( &options, "//" OY_TYPE_STD "/file_write/comment",
                             comment, OY_CREATE_NEW );
      error = oyImage_ToFile( image, output, options );

      oyFree_m_( comment );
      oyOptions_Release( &options );
      if(!error && verbose)
        fprintf( stderr, "wrote file to %s\n", output );
  }

  oyProfile_Release( &profile );
  oyOption_Release( &opt );
  oyConfig_Release( &device );
  oyConversion_Release( &c );
  oyOptions_Release( &image_tags );

  oyImage_Release( &image );

  return 0;
}
