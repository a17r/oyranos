/** @file oyranos_cmm_lcm2.c
 *
 *  Oyranos is an open source Color Management System 
 *
 *  @par Copyright:
 *            2007-2016 (C) Kai-Uwe Behrmann
 *
 *  @brief    littleCMS CMM module for Oyranos
 *  @internal
 *  @author   Kai-Uwe Behrmann <ku.b@gmx.de>
 *  @par License:
 *            new BSD <http://www.opensource.org/licenses/BSD-3-Clause>
 *  @since    2007/11/12
 */



#include <lcms2.h>
#include <stdarg.h>

#include "oyArray2d_s_.h"
#include "oyCMM_s.h"
#include "oyCMMapi4_s.h"
#include "oyCMMapi4_s_.h"
#include "oyCMMapi6_s_.h"
#include "oyCMMapi7_s.h"
#include "oyCMMapi7_s_.h"
#include "oyCMMapi10_s_.h"
#include "oyCMMui_s_.h"
#include "oyConnectorImaging_s_.h"
#include "oyProfiles_s.h"

#include "oyranos_cmm.h"         /* the API's this CMM implements */
#include "oyranos_config_internal.h"
#include "oyranos_generic.h"         /* oy_connector_imaging_static_object */
#include "oyranos_helper.h"      /* oySprintf_ and other local helpers */
#include "oyranos_i18n.h"
#include "oyranos_io.h"
#include "oyranos_image.h"
#include "oyranos_object_internal.h"
#include "oyranos_string.h"

#ifdef _OPENMP
#define USE_OPENMP 1
#include <omp.h>
#endif

/*
oyCMM_s   lcm2_cmm_module;
oyCMMapi4_s     lcm2_api4_cmm;
oyCMMui_s         lcm2_api4_ui;
oyCMMapi7_s     lcm2_api7_cmm;
oyConnectorImaging_s* lcm2_cmmIccSocket_connectors[2];
oyConnectorImaging_s    lcm2_cmmIccSocket_connector;
oyConnectorImaging_s* lcm2_cmmIccPlug_connectors[2];
oyConnectorImaging_s    lcm2_cmmIccPlug_connector;
oyCMMapi6_s     lcm2_api6_cmm;
oyCMMapi10_s    lcm2_api10_cmm;
oyCMMapi10_s    lcm2_api10_cmm2;
*/

void* oyAllocateFunc_           (size_t        size);
void* oyAllocateWrapFunc_       (size_t        size,
                                 oyAlloc_f     allocate_func);
void  oyDeAllocateFunc_         (void *        data);

#include <math.h>
#include <string.h>                    /* memcpy */


/* --- internal definitions --- */

#define CMM_NICK "lcm2"
#define CMMProfileOpen_M    lcmsOpenProfileFromMemTHR
#define CMMProfileRelease_M lcmsCloseProfile
#define CMMToString_M(text) #text
#define CMMMaxChannels_M 16
#define lcm2PROFILE "lcP2"
#define lcm2TRANSFORM "lcC2"
/** The proofing LUTs grid size may improove the sharpness of out of color 
 *  marking, but at the prise of lost speed and increased memory consumption.
 *  53 is the grid size used internally in lcm2' gamut marking code. */
#define lcm2PROOF_LUT_GRID_RASTER 53
/*#define ENABLE_MPE 1*/

#define CMM_VERSION {0,1,0}

oyMessage_f lcm2_msg = oyMessageFunc;

void lcm2ErrorHandlerFunction         ( cmsContext          ContextID,
                                       cmsUInt32Number     ErrorCode,
                                       const char        * ErrorText );
int            lcm2CMMMessageFuncSet ( oyMessage_f         lcm2_msg_func );
int                lcm2CMMInit       ( );


/** @struct lcm2ProfileWrap_s
 *  @brief lcm2 wrapper for profile data struct
 *
 *  @version Oyranos: 0.1.8
 *  @date    2007/12/10
 *  @since   2007/12/10 (Oyranos: 0.1.8)
 */
typedef struct lcm2ProfileWrap_s_ {
  uint32_t     type;                   /**< shall be lcm2PROFILE */
  size_t       size;
  oyPointer    block;                  /**< Oyranos raw profile pointer. Dont free! */
  oyPointer    lcm2;                   /**< cmsHPROFILE struct */
  icColorSpaceSignature sig;           /**< ICC profile signature */
  oyProfile_s *dbg_profile;            /**< only for debugging */
} lcm2ProfileWrap_s;

/** @struct  lcm2TransformWrap_s
 *  @brief   lcm2 wrapper for transform data struct
 *
 *  @version Oyranos: 0.1.8
 *  @date    2007/12/20
 *  @since   2007/12/20 (Oyranos: 0.1.8)
 */
typedef struct lcm2TransformWrap_s_ {
  int          type;                   /**< shall be lcm2TRANSFORM */
  oyPointer    lcm2;                   /**< cmsHPROFILE struct */
  icColorSpaceSignature sig_in;        /**< ICC profile signature */
  icColorSpaceSignature sig_out;       /**< ICC profile signature */
  oyPixel_t    oy_pixel_layout_in;
  oyPixel_t    oy_pixel_layout_out;
} lcm2TransformWrap_s;


lcm2TransformWrap_s * lcm2TransformWrap_Set_ (
                                       cmsHTRANSFORM       xform,
                                       icColorSpaceSignature color_in,
                                       icColorSpaceSignature color_out,
                                       oyPixel_t           oy_pixel_layout_in,
                                       oyPixel_t           oy_pixel_layout_out,
                                       oyPointer_s       * oy );
int      lcm2CMMTransform_GetWrap_   ( oyPointer_s       * cmm_ptr,
                                       lcm2TransformWrap_s ** s );
int lcm2CMMDeleteTransformWrap       ( oyPointer         * wrap );

lcm2ProfileWrap_s * lcm2CMMProfile_GetWrap_(
                                       oyPointer_s       * cmm_ptr );
int lcm2CMMProfileReleaseWrap        ( oyPointer         * p );

int                lcm2CMMCheckPointer(oyPointer_s       * cmm_ptr,
                                       const char        * resource );
int        oyPixelToLcm2PixelLayout_ ( oyPixel_t           pixel_layout,
                                       icColorSpaceSignature color_space );
char * lcm2Image_GetText             ( oyImage_s         * image,
                                       int                 verbose,
                                       oyAlloc_f           allocateFunc );


char * lcm2FilterNode_GetText        ( oyFilterNode_s    * node,
                                       oyNAME_e            type,
                                       oyAlloc_f           allocateFunc );
extern char lcm2_extra_options[];
char * lcm2FlagsToText               ( int                 flags );
cmsHPROFILE  lcm2GamutCheckAbstract  ( oyProfile_s       * proof,
                                       cmsUInt32Number     flags,
                                       int                 intent,
                                       int                 intent_proof,
                                       uint32_t            icc_profile_flags );
oyPointer  lcm2CMMColorConversion_ToMem_ (
                                       cmsHTRANSFORM     * xform,
                                       oyOptions_s       * opts,
                                       size_t            * size,
                                       oyAlloc_f           allocateFunc );
oyOptions_s* lcm2Filter_CmmIccValidateOptions
                                     ( oyFilterCore_s    * filter,
                                       oyOptions_s       * validate,
                                       int                 statical,
                                       uint32_t          * result );
cmsHPROFILE  lcm2AddProfile          ( oyProfile_s       * p );
lcm2ProfileWrap_s * lcm2AddProofProfile
                                     ( oyProfile_s       * proof,
                                       cmsUInt32Number     flags,
                                       int                 intent,
                                       int                 intent_proof,
                                       uint32_t            icc_profile_flags );
oyPointer lcm2FilterNode_CmmIccContextToMem (
                                       oyFilterNode_s    * node,
                                       size_t            * size,
                                       oyAlloc_f           allocateFunc );
int  lcm2ModuleData_Convert          ( oyPointer_s       * data_in,
                                       oyPointer_s       * data_out,
                                       oyFilterNode_s    * node );
int      lcm2FilterPlug_CmmIccRun    ( oyFilterPlug_s    * requestor_plug,
                                       oyPixelAccess_s   * ticket );
const char * lcm2InfoGetText         ( const char        * select,
                                       oyNAME_e            type,
                                       oyStruct_s        * context );




/* --- implementations --- */
/* explicitely load liblcms functions, to avoid conflicts */
static int lcms_initialised = 0; /* 0 - need init; 1 - successful init; -1 - error on init */
static void * lcms_handle = NULL;

static void (*lcmsSetLogErrorHandler)(cmsLogErrorHandlerFunction Fn) = NULL;
static void (*lcmsSetLogErrorHandlerTHR)(      cmsContext ContextID,
                                               cmsLogErrorHandlerFunction Fn) = NULL;
static icColorSpaceSignature (*lcmsGetColorSpace)(cmsHPROFILE hProfile) = NULL;
static icColorSpaceSignature (*lcmsGetPCS)(cmsHPROFILE hProfile) = NULL;
static icProfileClassSignature (*lcmsGetDeviceClass)(cmsHPROFILE hProfile) = NULL;
static int (*l_cmsLCMScolorSpace)(cmsColorSpaceSignature ProfileSpace) = NULL;
static cmsUInt32Number (*lcmsChannelsOf)(cmsColorSpaceSignature ColorSpace) = NULL;
static cmsHTRANSFORM (*lcmsCreateTransform)(cmsHPROFILE Input,
                                               cmsUInt32Number InputFormat,
                                               cmsHPROFILE Output,
                                               cmsUInt32Number OutputFormat,
                                               cmsUInt32Number Intent,
                                               cmsUInt32Number dwFlags) = NULL;
static cmsHTRANSFORM (*lcmsCreateProofingTransform)(cmsHPROFILE Input,
                                               cmsUInt32Number InputFormat,
                                               cmsHPROFILE Output,
                                               cmsUInt32Number OutputFormat,
                                               cmsHPROFILE Proofing,
                                               int Intent,
                                               int ProofingIntent,
                                               cmsUInt32Number dwFlags) = NULL;
static cmsHTRANSFORM (*lcmsCreateProofingTransformTHR)(cmsContext ContextID,
                                                  cmsHPROFILE Input,
                                                  cmsUInt32Number InputFormat,
                                                  cmsHPROFILE Output,
                                                  cmsUInt32Number OutputFormat,
                                                  cmsHPROFILE Proofing,
                                                  cmsUInt32Number Intent,
                                                  cmsUInt32Number ProofingIntent,
                                                  cmsUInt32Number dwFlags) = NULL;
static cmsHTRANSFORM (*lcmsCreateMultiprofileTransform)(cmsHPROFILE hProfiles[],
                                                                int nProfiles,
                                                                cmsUInt32Number InputFormat,
                                                                cmsUInt32Number OutputFormat,
                                                                int Intent,
                                                                cmsUInt32Number dwFlags) = NULL;
static cmsHTRANSFORM (*lcmsCreateExtendedTransform)(cmsContext ContextID,
                                                   cmsUInt32Number nProfiles, cmsHPROFILE hProfiles[],
                                                   cmsBool  BPC[],
                                                   cmsUInt32Number Intents[],
                                                   cmsFloat64Number AdaptationStates[],
                                                   cmsHPROFILE hGamutProfile,
                                                   cmsUInt32Number nGamutPCSposition,
                                                   cmsUInt32Number InputFormat,
                                                   cmsUInt32Number OutputFormat,
                                                   cmsUInt32Number dwFlags) = NULL;
static void (*lcmsDeleteTransform)(cmsHTRANSFORM hTransform) = NULL;
static void (*lcmsDoTransform)(cmsHTRANSFORM Transform,
                                                 const void * InputBuffer,
                                                 void * OutputBuffer,
                                                 cmsUInt32Number Size) = NULL;
static cmsHPROFILE (*lcmsTransform2DeviceLink)(cmsHTRANSFORM hTransform, cmsFloat64Number Version, cmsUInt32Number dwFlags) = NULL;
static cmsBool (*lcmsSaveProfileToMem)(cmsHPROFILE hProfile, void *MemPtr, 
                                                                cmsUInt32Number* BytesNeeded) = NULL;
static cmsHPROFILE (*lcmsOpenProfileFromMemTHR)(cmsContext ContextID, const void * MemPtr, cmsUInt32Number dwSize) = NULL;
static cmsHPROFILE (*lcmsOpenProfileFromFileTHR)(cmsContext ContextID, const char *ICCProfile, const char *sAccess) = NULL;
static cmsBool (*lcmsCloseProfile)(cmsHPROFILE hProfile) = NULL;
static cmsHPROFILE (*lcmsCreateProfilePlaceholder)(cmsContext ContextID) = NULL;
static cmsHPROFILE (*lcmsCreateLab4ProfileTHR)(cmsContext ContextID, const cmsCIExyY* WhitePoint) = NULL;
static cmsHPROFILE (*lcmsCreateLab4Profile)(const cmsCIExyY* WhitePoint) = NULL;
static void (*lcmsSetProfileVersion)(cmsHPROFILE hProfile, cmsFloat64Number Version) = NULL;
static void (*lcmsSetDeviceClass)(cmsHPROFILE hProfile, icProfileClassSignature sig) = NULL;
static void (*lcmsSetColorSpace)(cmsHPROFILE hProfile, icColorSpaceSignature sig) = NULL;
static void (*lcmsSetPCS)(cmsHPROFILE hProfile, icColorSpaceSignature pcs) = NULL;
static cmsToneCurve* (*lcmsBuildGamma)(cmsContext ContextID, cmsFloat64Number Gamma) = NULL;
static cmsToneCurve*(*lcmsBuildSegmentedToneCurve)(cmsContext ContextID, cmsInt32Number nSegments, const cmsCurveSegment Segments[]) = NULL;
static cmsToneCurve*(*lcmsBuildParametricToneCurve)(cmsContext ContextID, cmsInt32Number Type, const cmsFloat64Number Parameters[]) = NULL;
static void (*lcmsFreeToneCurve)(cmsToneCurve* Curve) = NULL;
static cmsPipeline*      (*lcmsPipelineAlloc)              (cmsContext ContextID, cmsUInt32Number InputChannels, cmsUInt32Number OutputChannels) = NULL;
static int               (*lcmsPipelineInsertStage)        (cmsPipeline* lut, cmsStageLoc loc, cmsStage* mpe) = NULL;
static void              (*lcmsPipelineFree)               (cmsPipeline* lut) = NULL;
static cmsStage*         (*lcmsPipelineGetPtrToFirstStage) (cmsPipeline* lut ) = NULL;
static cmsStageSignature (*lcmsStageType) (cmsStage* stage) = NULL;
static cmsStage*         (*lcmsStageNext) (cmsStage* next ) = NULL;
static int               (*lcmsStageInputChannels)         (cmsStage* stage) = NULL;
static int               (*lcmsStageOutputChannels)        (cmsStage* stage) = NULL;
static cmsStage*(*lcmsStageAllocCLut16bit)(cmsContext ContextID, cmsUInt32Number nGridPoints, cmsUInt32Number inputChan, cmsUInt32Number outputChan, const cmsUInt16Number* Table) = NULL;
static cmsStage*(*lcmsStageAllocCLutFloat)(cmsContext ContextID, cmsUInt32Number nGridPoints, cmsUInt32Number inputChan, cmsUInt32Number outputChan, const cmsFloat32Number* Table) = NULL;
static cmsBool (*lcmsStageSampleCLut16bit)(cmsStage* mpe,    cmsSAMPLER16 Sampler, void* Cargo, cmsUInt32Number dwFlags) = NULL;
static cmsBool (*lcmsStageSampleCLutFloat)(cmsStage* mpe, cmsSAMPLERFLOAT Sampler, void* Cargo, cmsUInt32Number dwFlags) = NULL;
static cmsStage*(*lcmsStageAllocToneCurves)(cmsContext ContextID, cmsUInt32Number nChannels, cmsToneCurve* const Curves[]) = NULL;
static cmsBool (*lcmsWriteTag)(cmsHPROFILE hProfile, cmsTagSignature sig, const void* data) = NULL;
static cmsMLU*(*lcmsMLUalloc)(cmsContext ContextID, cmsUInt32Number nItems) = NULL;
static cmsBool (*lcmsMLUsetASCII)(cmsMLU* mlu,
                                                  const char LanguageCode[3], const char CountryCode[3],
                                                  const char* ASCIIString) = NULL;
static void (*lcmsMLUfree)(cmsMLU* mlu) = NULL;
static cmsHPROFILE (*lcmsCreateRGBProfile)(const cmsCIExyY* WhitePoint,
                                        const cmsCIExyYTRIPLE* Primaries,
                                        cmsToneCurve* TransferFunction[3]) = NULL;
static void (*lcmsLabEncoded2Float)(cmsCIELab* Lab, const cmsUInt16Number wLab[3]) = NULL;
static void (*lcmsFloat2LabEncoded)(cmsUInt16Number wLab[3], const cmsCIELab* Lab) = NULL;
static const cmsCIEXYZ*  (*lcmsD50_XYZ)(void);
static const cmsCIExyY*  (*lcmsD50_xyY)(void);
static cmsFloat64Number (*lcmsDeltaE)(const cmsCIELab* Lab1, const cmsCIELab* Lab2) = NULL;
static void (*lcmsGetAlarmCodes)(cmsUInt16Number NewAlarm[cmsMAXCHANNELS]) = NULL;
static cmsContext (*lcmsCreateContext)(void* Plugin, void* UserData) = NULL;
static cmsContext dummyCreateContext(void* Plugin, void* UserData) {return NULL;}
static void* (*lcmsGetContextUserData)(cmsContext ContextID) = NULL;
static void* dummyGetContextUserData(cmsContext ContextID) {return NULL;}
static cmsContext (*lcmsGetProfileContextID)(cmsHPROFILE hProfile) = NULL;
static cmsContext (*lcmsGetTransformContextID)(cmsHPROFILE hProfile) = NULL;
static int (*lcmsGetEncodedCMMversion)(void) = NULL;
static int dummyGetEncodedCMMversion() {return LCMS_VERSION;}

#define LOAD_FUNC( func, fallback_func ) l##func = dlsym(lcms_handle, #func ); \
               if(!l##func) \
               { \
                 oyMSG_e type = oyMSG_ERROR; \
                 if(#fallback_func != NULL) \
                 { \
                   l##func = fallback_func; \
                   type = oyMSG_WARN; \
                 } else \
                 { \
                   error = 1; \
                 } \
                 report = 1; \
                 lcm2_msg( type,0, OY_DBG_FORMAT_" " \
                                      "dlsym failed: %s", \
                                      OY_DBG_ARGS_, dlerror() ); \
               }

/** Function lcm2CMMInit
 *  @brief   API requirement
 *
 *  @version Oyranos: 0.9.5
 *  @date    2014/02/27
 *  @since   2007/12/11 (Oyranos: 0.1.8)
 */
int                lcm2CMMInit       ( oyStruct_s        * filter )
{
  int error = 0;
  if(!lcms_initialised)
  {
    int report = 0;
    char * fn = oyLibNameCreate_( "lcms2", 2 );
    lcms_handle = dlopen(fn, RTLD_LAZY);

    if(!lcms_handle)
    {
      lcm2_msg( oyMSG_ERROR,0, OY_DBG_FORMAT_" "
               "init \"%s\" failed: %s",
                OY_DBG_ARGS_, fn, dlerror() );
      error = 1;
      lcms_initialised = -1;
    } else
    {
      LOAD_FUNC( cmsSetLogErrorHandler, NULL );
      LOAD_FUNC( cmsSetLogErrorHandlerTHR, NULL );
      LOAD_FUNC( cmsGetColorSpace, NULL );
      LOAD_FUNC( cmsGetPCS, NULL );
      LOAD_FUNC( cmsGetDeviceClass, NULL );
      LOAD_FUNC( _cmsLCMScolorSpace, NULL );
      LOAD_FUNC( cmsChannelsOf, NULL );
      LOAD_FUNC( cmsCreateTransform, NULL );
      LOAD_FUNC( cmsCreateProofingTransform, NULL );
      LOAD_FUNC( cmsCreateProofingTransformTHR, NULL );
      LOAD_FUNC( cmsCreateMultiprofileTransform, NULL );
      LOAD_FUNC( cmsCreateExtendedTransform, NULL );
      LOAD_FUNC( cmsDeleteTransform, NULL );
      LOAD_FUNC( cmsDoTransform, NULL );
      LOAD_FUNC( cmsTransform2DeviceLink, NULL );
      LOAD_FUNC( cmsSaveProfileToMem, NULL );
      LOAD_FUNC( cmsOpenProfileFromMemTHR, NULL );
      LOAD_FUNC( cmsOpenProfileFromFileTHR, NULL );
      LOAD_FUNC( cmsCloseProfile, NULL );
      LOAD_FUNC( cmsCreateProfilePlaceholder, NULL );
      LOAD_FUNC( cmsSetProfileVersion, NULL );
      LOAD_FUNC( cmsCreateLab4ProfileTHR, NULL );
      LOAD_FUNC( cmsCreateLab4Profile, NULL );
      LOAD_FUNC( cmsCreateRGBProfile, NULL );
      LOAD_FUNC( cmsSetDeviceClass, NULL );
      LOAD_FUNC( cmsSetColorSpace, NULL );
      LOAD_FUNC( cmsSetPCS, NULL );
      LOAD_FUNC( cmsBuildGamma, NULL );
      LOAD_FUNC( cmsBuildSegmentedToneCurve, NULL );
      LOAD_FUNC( cmsBuildParametricToneCurve, NULL );
      LOAD_FUNC( cmsFreeToneCurve, NULL );
      LOAD_FUNC( cmsPipelineAlloc, NULL );
      LOAD_FUNC( cmsPipelineFree, NULL );
      LOAD_FUNC( cmsPipelineInsertStage, NULL );
      LOAD_FUNC( cmsPipelineGetPtrToFirstStage, NULL );
      LOAD_FUNC( cmsStageType, NULL );
      LOAD_FUNC( cmsStageNext, NULL );
      LOAD_FUNC( cmsStageInputChannels, NULL );
      LOAD_FUNC( cmsStageOutputChannels, NULL );
      LOAD_FUNC( cmsStageAllocCLut16bit, NULL );
      LOAD_FUNC( cmsStageAllocCLutFloat, NULL );
      LOAD_FUNC( cmsStageSampleCLut16bit, NULL );
      LOAD_FUNC( cmsStageSampleCLutFloat, NULL );
      LOAD_FUNC( cmsStageAllocToneCurves, NULL );
      LOAD_FUNC( cmsWriteTag, NULL );
      LOAD_FUNC( cmsMLUalloc, NULL );
      LOAD_FUNC( cmsMLUsetASCII, NULL );
      LOAD_FUNC( cmsMLUfree, NULL );
      LOAD_FUNC( cmsLabEncoded2Float, NULL );
      LOAD_FUNC( cmsFloat2LabEncoded, NULL );
      LOAD_FUNC( cmsD50_XYZ, NULL );
      LOAD_FUNC( cmsD50_xyY, NULL );
      LOAD_FUNC( cmsDeltaE, NULL );
      LOAD_FUNC( cmsGetAlarmCodes, NULL );
#if LCMS_VERSION >= 2060
      LOAD_FUNC( cmsCreateContext, dummyCreateContext ); /* available since lcms 2.6 */
      LOAD_FUNC( cmsGetContextUserData, dummyGetContextUserData ); /* available since lcms 2.6 */
#else
      lcmsCreateContext = dummyCreateContext;
      lcmsGetContextUserData = dummyGetContextUserData;
#endif
      LOAD_FUNC( cmsGetProfileContextID, NULL );
      LOAD_FUNC( cmsGetTransformContextID, NULL );
      LOAD_FUNC( cmsGetEncodedCMMversion, dummyGetEncodedCMMversion );
      if(lcmsSetLogErrorHandler)
        lcmsSetLogErrorHandler( lcm2ErrorHandlerFunction );
      else
          lcm2_msg( oyMSG_WARN, (oyStruct_s*)NULL,
                    OY_DBG_FORMAT_"can not set error handler %d %d",
                    OY_DBG_ARGS_, lcmsGetEncodedCMMversion, LCMS_VERSION );
      if(lcmsGetEncodedCMMversion() != LCMS_VERSION)
          lcm2_msg( oyMSG_WARN, (oyStruct_s*)NULL,
                    OY_DBG_FORMAT_" compile and run time version differ %d %d",
                    OY_DBG_ARGS_, lcmsGetEncodedCMMversion, LCMS_VERSION );
          
      if(error)
        lcms_initialised = -1;
      else
        lcms_initialised = 1;

      if(report)
        lcm2_msg( oyMSG_WARN,0, OY_DBG_FORMAT_" "
               "init \"%s\" issue(s): v%d",
                OY_DBG_ARGS_, fn, lcmsGetEncodedCMMversion() );
    }
    oyFree_m_( fn );

  } else if(lcms_initialised == -1)
    error = 1;
  return error;
}



/** Function lcm2CMMProfile_GetWrap_
 *  @brief   convert to lcm2 profile wrapper struct
 *
 *  @version Oyranos: 0.1.8
 *  @date    2007/12/10
 *  @since   2007/12/10 (Oyranos: 0.1.8)
 */
lcm2ProfileWrap_s * lcm2CMMProfile_GetWrap_( oyPointer_s* cmm_ptr )
{
  lcm2ProfileWrap_s * s = 0;

  char * type_ = lcm2PROFILE;
  int type = *((int32_t*)type_);

  if(cmm_ptr && !lcm2CMMCheckPointer( cmm_ptr, lcm2PROFILE ) &&
     oyPointer_GetPointer(cmm_ptr))
    s = (lcm2ProfileWrap_s*) oyPointer_GetPointer(cmm_ptr);

  if(s && s->type != type)
    s = 0;

  if(oy_debug >= 2)
  {
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)cmm_ptr,
              OY_DBG_FORMAT_" profile size: %d %s cmm_ptr: %d",
              OY_DBG_ARGS_, s->size, s->dbg_profile?oyNoEmptyString_m_(oyProfile_GetFileName( s->dbg_profile,-1 )):"????",
              oyStruct_GetId((oyStruct_s*)cmm_ptr) );
  }

  return s;
}

/** Function lcm2CMMTransform_GetWrap_
 *  @brief   convert to lcm2 transform wrapper struct
 *
 *  @version Oyranos: 0.1.8
 *  @since   2007/12/20 (Oyranos: 0.1.8)
 *  @date    2009/05/28
 */
int      lcm2CMMTransform_GetWrap_   ( oyPointer_s       * cmm_ptr,
                                       lcm2TransformWrap_s ** s )
{
  char * type_ = lcm2TRANSFORM;
  int type = *((int32_t*)type_);

  if(cmm_ptr && !lcm2CMMCheckPointer( cmm_ptr, lcm2TRANSFORM ) &&
     oyPointer_GetPointer(cmm_ptr))
    *s = (lcm2TransformWrap_s*) oyPointer_GetPointer(cmm_ptr);

  if(*s && ((*s)->type != type || !(*s)->lcm2))
  {
    *s = 0;
    return 1;
  }

  return 0;
}

/** Function lcm2CMMProfileReleaseWrap
 *  @brief   release a lcm2 profile wrapper struct
 *
 *  @version Oyranos: 0.1.8
 *  @date    2007/12/20
 *  @since   2007/12/20 (Oyranos: 0.1.8)
 */
int lcm2CMMProfileReleaseWrap(oyPointer *p)
{
  int error = !p;
  lcm2ProfileWrap_s * s = 0;
  
  char * type_ = lcm2PROFILE;
  int type = *((int32_t*)type_);
  char s_type[4];

  if(!error && *p)
    s = (lcm2ProfileWrap_s*) *p;

  if(!error)
    error = !s;

  if(!error)
    memcpy(s_type, &s->type, 4);

  if(!error && s->type != type)
    error = 1;

  if(!error)
  {
#if LCMS_VERSION >= 2060
    oyProfile_s * p = lcmsGetContextUserData( lcmsGetProfileContextID( s->lcm2 ) );
    oyProfile_Release ( &p );
#endif
    CMMProfileRelease_M (s->lcm2);
    oyProfile_Release( &s->dbg_profile );

    s->lcm2 = 0;
    s->type = 0;
    s->size = 0;
    s->block = 0;
    free(s);
  }

  if(!error)
    *p = 0;

  return error;
}


/** Function lcm2CMMDataOpen
 *  @brief   oyCMMProfileOpen_t implementation
 *
 *  @version Oyranos: 0.1.10
 *  @since   2007/11/12 (Oyranos: 0.1.8)
 *  @date    2007/12/27
 */
int          lcm2CMMData_Open        ( oyStruct_s        * data,
                                       oyPointer_s       * oy )
{
  oyPointer_s * s = 0;
  int error = 0;

  if(!error)
  {
    char * type_ = lcm2PROFILE;
    int type = *((int32_t*)type_);
    size_t size = 0;
    oyPointer block = 0;
    oyProfile_s * p = NULL;
    lcm2ProfileWrap_s * s = calloc(sizeof(lcm2ProfileWrap_s), 1);

    if(data->type_ == oyOBJECT_PROFILE_S)
    {
      p = (oyProfile_s*)data;
      block = oyProfile_GetMem( p, &size, 0, oyAllocateFunc_ );
    }

    s->type = type;
    s->size = size;
    s->block = block;
    if(oy_debug >= 2)
    {
      s->dbg_profile = oyProfile_Copy( p, 0 );
      lcm2_msg( oyMSG_DBG, data,
                OY_DBG_FORMAT_" going to open %s", OY_DBG_ARGS_,
                p?oyProfile_GetFileName( p,-1 ):"????" );
    }

#if LCMS_VERSION < 2060
    s->lcm2 = CMMProfileOpen_M( data, block, size );
#else
    {
      oyProfile_s * p2 = oyProfile_Copy( p, NULL );
      cmsContext tc = lcmsCreateContext( NULL, p2 ); /* threading context */
      lcmsSetLogErrorHandlerTHR( tc, lcm2ErrorHandlerFunction );
      s->lcm2 = CMMProfileOpen_M( tc, block, size );
    }
#endif
    if(!s->lcm2)
      lcm2_msg( oyMSG_WARN, (oyStruct_s*)data,
             OY_DBG_FORMAT_" %s() failed", OY_DBG_ARGS_, "CMMProfileOpen_M" );
    error = oyPointer_Set( oy, 0,
                          lcm2PROFILE, s, CMMToString_M(CMMProfileOpen_M),
                          lcm2CMMProfileReleaseWrap );
    if(error)
      lcm2_msg( oyMSG_WARN, (oyStruct_s*)data,
             OY_DBG_FORMAT_" oyPointer_Set() failed", OY_DBG_ARGS_ );
  }

  if(!error)
    s = oy;

  if(!error)
    error = !s;

  return error;
}

/** Function lcm2CMMCheckPointer
 *  @brief   
 *
 *  @version Oyranos: 0.1.8
 *  @date    2007/11/12
 *  @since   2007/11/12 (Oyranos: 0.1.8)
 */
int                lcm2CMMCheckPointer(oyPointer_s       * cmm_ptr,
                                       const char        * resource )
{
  int error = !cmm_ptr;

  if(cmm_ptr &&
     oyPointer_GetPointer(cmm_ptr) && oyPointer_GetResourceName(cmm_ptr))
  {
    int * res_id = (int*)oyPointer_GetResourceName(cmm_ptr);

    if(!oyCMMlibMatchesCMM(oyPointer_GetLibName(cmm_ptr), CMM_NICK) ||
       *res_id != *((int*)(resource)) )
      error = 1;
  } else {
    error = 1;
  }

  return error;
}



/** Function oyPixelToLcm2PixelLayout_
 *  @brief
 *
 *  @version Oyranos: 0.1.8
 *  @date    2007/11/00
 *  @since   2007/11/00 (Oyranos: 0.1.8)
 */
int        oyPixelToLcm2PixelLayout_ ( oyPixel_t           pixel_layout,
                                       icColorSpaceSignature color_space )
{
  int cmm_pixel = 0;
  int chan_n = oyToChannels_m (pixel_layout);
  int c_off = oyToColorOffset_m (pixel_layout);
  oyDATATYPE_e data_type = oyToDataType_m (pixel_layout);
  int planar = oyToPlanar_m (pixel_layout);
  int flavour = oyToFlavor_m (pixel_layout);
  unsigned int cchans = lcmsChannelsOf( (cmsColorSpaceSignature)color_space );
  unsigned int lcm2_color_space = l_cmsLCMScolorSpace( color_space );
  int extra = chan_n - cchans;

  if(chan_n > CMMMaxChannels_M)
    lcm2_msg( oyMSG_WARN,0, OY_DBG_FORMAT_" "
             "can not handle more than %d channels; found: %d",
             OY_DBG_ARGS_, CMMMaxChannels_M, chan_n);

  cmm_pixel |= CHANNELS_SH(cchans);
  if(extra)
    cmm_pixel |= EXTRA_SH(extra);
  if(c_off == 1)
    cmm_pixel |= SWAPFIRST_SH(1);
  if(data_type == oyUINT8)
    cmm_pixel |= BYTES_SH(1);
  else if(data_type == oyUINT16 || data_type == oyHALF)
    cmm_pixel |= BYTES_SH(2);
  else if(data_type == oyFLOAT)
    cmm_pixel |= BYTES_SH(4);
  else if(data_type == oyDOUBLE)
    cmm_pixel |= BYTES_SH(0);
  if(data_type == oyDOUBLE || data_type == oyFLOAT || data_type == oyHALF)
    cmm_pixel |= FLOAT_SH(1);
  if(oyToSwapColorChannels_m (pixel_layout))
    cmm_pixel |= DOSWAP_SH(1);
  if(oyToByteswap_m(pixel_layout))
    cmm_pixel |= ENDIAN16_SH(1);
  if(planar)
    cmm_pixel |= PLANAR_SH(1);
  if(flavour)
    cmm_pixel |= FLAVOR_SH(1);

  /* lcms2 uses V4 style value ranges */
  cmm_pixel |= COLORSPACE_SH( lcm2_color_space );


  return cmm_pixel;
}

/** Function lcm2CMMDeleteTransformWrap
 *  @brief
 *
 *  @version Oyranos: 0.1.8
 *  @since   2007/12/00 (Oyranos: 0.1.8)
 *  @date    2007/12/00
 */
int lcm2CMMDeleteTransformWrap(oyPointer * wrap)
{
  
  if(wrap && *wrap)
  {
    lcm2TransformWrap_s * s = (lcm2TransformWrap_s*) *wrap;

    lcmsDeleteTransform (s->lcm2);
    s->lcm2 = 0;

    free(s);

    *wrap = 0;

    return 0;
  }

  return 1;
}

/** Function lcm2TransformWrap_Set_
 *  @brief   fill a lcm2TransformWrap_s struct
 *
 *  @version Oyranos: 0.1.8
 *  @since   2007/12/21 (Oyranos: 0.1.8)
 *  @date    2007/12/21
 */
lcm2TransformWrap_s * lcm2TransformWrap_Set_ (
                                       cmsHTRANSFORM       xform,
                                       icColorSpaceSignature color_in,
                                       icColorSpaceSignature color_out,
                                       oyPixel_t           oy_pixel_layout_in,
                                       oyPixel_t           oy_pixel_layout_out,
                                       oyPointer_s       * oy )
{
  int error = !xform;
  lcm2TransformWrap_s * s = 0;
  
  if(!error)
  {
    char * type_ = lcm2TRANSFORM;
    int type = *((int32_t*)type_);
    lcm2TransformWrap_s * ltw = calloc(sizeof(lcm2TransformWrap_s), 1);

    ltw->type = type;

    ltw->lcm2 = xform; xform = 0;

    ltw->sig_in  = color_in;
    ltw->sig_out = color_out;
    ltw->oy_pixel_layout_in  = oy_pixel_layout_in;
    ltw->oy_pixel_layout_out = oy_pixel_layout_out;
    s = ltw;
    if(oy_debug >= 2)
      lcm2_msg( oyMSG_DBG, NULL, OY_DBG_FORMAT_
             " xform: "OY_PRINT_POINTER
             " ltw: "OY_PRINT_POINTER,
             OY_DBG_ARGS_, ltw->lcm2, ltw );
  }

  if(!error)
    oyPointer_Set( oy, 0, 0, s,
                  "lcm2CMMDeleteTransformWrap", lcm2CMMDeleteTransformWrap );

  return s;
}

int            lcm2IntentFromOptions ( oyOptions_s       * opts,
                                       int                 proof )
{
  int intent = 0,
      intent_proof = 0;
  const char * o_txt = 0;

#ifndef oyStrlen_
#define oyStrlen_ strlen
#endif
      o_txt = oyOptions_FindString  ( opts, "rendering_intent", 0);
      if(o_txt && oyStrlen_(o_txt))
        intent = atoi( o_txt );

      o_txt = oyOptions_FindString  ( opts, "rendering_intent_proof", 0);
      if(o_txt && oyStrlen_(o_txt))
        intent_proof = atoi( o_txt );

      intent_proof = intent_proof == 0 ? INTENT_RELATIVE_COLORIMETRIC :
                                         INTENT_ABSOLUTE_COLORIMETRIC;

  if(oy_debug > 2)
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)opts, OY_DBG_FORMAT_"\n"
             "  proof: %d  intent: %d  intent_proof: %d\n",
             OY_DBG_ARGS_,
                proof,     intent,     intent_proof );

  if(proof)
    return intent_proof;
  else
    return intent;
}

uint32_t       lcm2FlagsFromOptions  ( oyOptions_s       * opts )
{
  int bpc = 0,
      gamut_warning = 0,
      precalculation = 0,
      precalculation_curves = 1,
      no_white_on_white_fixup = 1,
      flags = 0;
  const char * o_txt = 0;
  static int precalculation_curves_warn = 0;

      o_txt = oyOptions_FindString  ( opts, "rendering_bpc", 0 );
      if(o_txt && oyStrlen_(o_txt))
        bpc = atoi( o_txt );

      o_txt = oyOptions_FindString  ( opts, "rendering_gamut_warning", 0 );
      if(o_txt && oyStrlen_(o_txt))
        gamut_warning = atoi( o_txt );

      o_txt = oyOptions_FindString  ( opts, "precalculation", 0 );
      if(o_txt && oyStrlen_(o_txt))
        precalculation = atoi( o_txt );

      o_txt = oyOptions_FindString  ( opts, "precalculation_curves", 0 );
      if(o_txt && oyStrlen_(o_txt))
        precalculation_curves = atoi( o_txt );

      o_txt = oyOptions_FindString  ( opts, "no_white_on_white_fixup", 0 );
      if(o_txt && oyStrlen_(o_txt))
        no_white_on_white_fixup = atoi( o_txt );

      /* this should be moved to the CMM and not be handled here in Oyranos */
      flags = bpc ?           flags | cmsFLAGS_BLACKPOINTCOMPENSATION :
                              flags & (~cmsFLAGS_BLACKPOINTCOMPENSATION);
      flags = gamut_warning ? flags | cmsFLAGS_GAMUTCHECK :
                              flags & (~cmsFLAGS_GAMUTCHECK);
      flags = no_white_on_white_fixup ?
                              flags | cmsFLAGS_NOWHITEONWHITEFIXUP :
                              flags & (~cmsFLAGS_NOWHITEONWHITEFIXUP);
      switch(precalculation)
      {
      case 0: flags |= 0; break;
      case 1: flags |= cmsFLAGS_NOOPTIMIZE; break;
      case 2: flags |= cmsFLAGS_HIGHRESPRECALC; break;
      case 3: flags |= cmsFLAGS_LOWRESPRECALC; break;
      }

      if(lcmsGetEncodedCMMversion() >= 2070)
      {
        switch(precalculation_curves)
        {
        case 0: flags |= 0; break;
        case 1: flags |= cmsFLAGS_CLUT_POST_LINEARIZATION | cmsFLAGS_CLUT_PRE_LINEARIZATION; break;
        }
      } else
        if(precalculation_curves_warn++ == 0)
          lcm2_msg( oyMSG_WARN, (oyStruct_s*)opts, OY_DBG_FORMAT_
              "Skipping cmsFLAGS_CLUT_POST_LINEARIZATION! Can not handle flag for DL creation. v%d\n",
              OY_DBG_ARGS_, lcmsGetEncodedCMMversion() );

  if(oy_debug > 2)
    lcm2_msg( oyMSG_DBG, (oyStruct_s*)opts, OY_DBG_FORMAT_"\n"
              "%s\n", OY_DBG_ARGS_, lcm2FlagsToText(flags) );

  return flags;
}

uint16_t in[4] = {32000,32000,32000,0}, out[4] = {65535,65535,65535,65535};

/** Function lcm2CMMConversionContextCreate_
 *  @brief   create a CMM transform
 *
 *  @version Oyranos: 0.3.3
 *  @since   2008/12/28 (Oyranos: 0.1.10)
 *  @date    2011/11/18
 */
cmsHTRANSFORM  lcm2CMMConversionContextCreate_ (
                                       oyFilterNode_s    * node,
                                       cmsHPROFILE       * lps,
                                       int                 profiles_n,
                                       oyProfiles_s      * simulation,
                                       int                 proof_n,
                                       int                 proof,
                                       oyPixel_t           oy_pixel_layout_in,
                                       oyPixel_t           oy_pixel_layout_out,
                                       oyOptions_s       * opts,
                                       lcm2TransformWrap_s ** ltw,
                                       oyPointer_s       * oy )
{
  oyPixel_t lcm2_pixel_layout_in = 0;
  oyPixel_t lcm2_pixel_layout_out = 0;
  int error = !lps;
  cmsHTRANSFORM xform = 0;
  cmsHPROFILE * merge = 0;
  icColorSpaceSignature color_in = 0;
  icColorSpaceSignature color_out = 0;
  icProfileClassSignature profile_class_in = 0;
  int intent = lcm2IntentFromOptions( opts,0 ),
      intent_proof = lcm2IntentFromOptions( opts,1 ),
      cmyk_cmyk_black_preservation = 0,
      flags = lcm2FlagsFromOptions( opts ),
      gamut_warning = flags & cmsFLAGS_GAMUTCHECK;
  const char * o_txt = 0;
  double adaption_state = 0.0;
  int multi_profiles_n = profiles_n;

  if(!lps || !profiles_n || !oy_pixel_layout_in || !oy_pixel_layout_out)
    return 0;

      flags = proof ?         flags | cmsFLAGS_SOFTPROOFING :
                              flags & (~cmsFLAGS_SOFTPROOFING);

  if(!error && lps[0] && lps[profiles_n-1])
  {
    color_in = lcmsGetColorSpace( lps[0] );
    if(profiles_n > 1)
      color_out = lcmsGetColorSpace( lps[profiles_n-1] );
    else
      color_out = lcmsGetPCS( lps[profiles_n-1] );
    profile_class_in = lcmsGetDeviceClass( lps[0] );
  }

  lcm2_pixel_layout_in  = oyPixelToLcm2PixelLayout_(oy_pixel_layout_in,
                                                   color_in);
  lcm2_pixel_layout_out = oyPixelToLcm2PixelLayout_(oy_pixel_layout_out,
                                                   color_out);

      o_txt = oyOptions_FindString  ( opts, "cmyk_cmyk_black_preservation", 0 );
      if(o_txt && oyStrlen_(o_txt))
        cmyk_cmyk_black_preservation = atoi( o_txt );

      intent = cmyk_cmyk_black_preservation ? intent + 10 : intent;
      if(cmyk_cmyk_black_preservation == 2)
        intent += 13;

      o_txt = oyOptions_FindString  ( opts, "adaption_state", 0 );
      if(o_txt && oyStrlen_(o_txt))
        adaption_state = atof( o_txt );

  if(!error)
  {
    cmsUInt32Number * intents=0;
    cmsBool * bpc=0;
    cmsFloat64Number * adaption_states=0;
         if(profiles_n == 1 || profile_class_in == icSigLinkClass)
    {
        /* we have to erase the color space */
#if 1
        int csp = T_COLORSPACE(lcm2_pixel_layout_in);
        lcm2_pixel_layout_in &= (~COLORSPACE_SH( csp ));
        csp = T_COLORSPACE(lcm2_pixel_layout_out);
        lcm2_pixel_layout_out &= (~COLORSPACE_SH( csp ));
#endif
        xform = lcmsCreateTransform( lps[0], lcm2_pixel_layout_in,
                                     0, lcm2_pixel_layout_out,
                                     (intent > 3)?0:intent,
                                     flags | cmsFLAGS_KEEP_SEQUENCE );
    }
    else if(profiles_n == 2 && (!proof_n || (!proof && !gamut_warning)))
    {
      oyAllocHelper_m_( intents, cmsUInt32Number, 2,0, goto end);
      oyAllocHelper_m_( bpc, cmsBool, 2,0, goto end);
      oyAllocHelper_m_( adaption_states, cmsFloat64Number, 2,0, goto end);
      intents[0] = intent; intents[1] = intent;
      bpc[0] = flags & cmsFLAGS_BLACKPOINTCOMPENSATION;
      bpc[1] = flags & cmsFLAGS_BLACKPOINTCOMPENSATION;
      adaption_states[0] = adaption_state; adaption_states[1] = adaption_state;
      xform = lcmsCreateExtendedTransform( 0, profiles_n, lps, bpc,
                                          intents, adaption_states, NULL, 0,
                                          lcm2_pixel_layout_in,
                                          lcm2_pixel_layout_out, flags | cmsFLAGS_KEEP_SEQUENCE );
    }
    else
    {
      int i;

      if(proof_n && (proof || gamut_warning))
      {
        int len = sizeof(cmsHPROFILE) * (profiles_n + proof_n);

        oyAllocHelper_m_( merge, cmsHPROFILE, profiles_n + proof_n,0, goto end);
        memset( merge, 0, len );
        memcpy( merge, lps, sizeof(cmsHPROFILE) * (profiles_n - 1) );

        for(i = 0; i < proof_n; ++i)
        {
          lcm2ProfileWrap_s * wrap = lcm2AddProofProfile( 
                                             oyProfiles_Get(simulation,i),flags,
                                             intent, intent_proof, 0);
          merge[profiles_n-1 + i] = wrap->lcm2;
        }

        merge[profiles_n + proof_n -1] = lps[profiles_n - 1];

        /* merge effect and simulation profiles */
        multi_profiles_n += proof_n;
        lps = merge;
      }

      if(flags & cmsFLAGS_GAMUTCHECK)
        flags |= cmsFLAGS_GRIDPOINTS(lcm2PROOF_LUT_GRID_RASTER);

    if(oy_debug > 2)
    {
    uint32_t f = lcm2_pixel_layout_in;
    printf ("%s:%d %s() float:%d optimised:%d colorspace:%d extra:%d channels:%d lcms_bytes %d \n", __FILE__,__LINE__,__func__, T_FLOAT(f), T_OPTIMIZED(f), T_COLORSPACE(f), T_EXTRA(f), T_CHANNELS(f), T_BYTES(f) );
    f = lcm2_pixel_layout_out;
    printf ("%s:%d %s() float:%d optimised:%d colorspace:%d extra:%d channels:%d lcms_bytes %d \n", __FILE__,__LINE__,__func__, T_FLOAT(f), T_OPTIMIZED(f), T_COLORSPACE(f), T_EXTRA(f), T_CHANNELS(f), T_BYTES(f) );
      printf("multi_profiles_n: %d intent: %d adaption: %g flags: %d \"%s\" l1 %d, l2 %d\n",
              multi_profiles_n, intent, adaption_state, flags,
              lcm2FlagsToText(flags),
              lcm2_pixel_layout_in, lcm2_pixel_layout_out);
    }

#define SET_ARR(arr,val,n) for(i = 0; i < n; ++i) arr[i] = val;
      oyAllocHelper_m_( intents, cmsUInt32Number, multi_profiles_n,0, goto end);
      oyAllocHelper_m_( bpc, cmsBool, multi_profiles_n,0, goto end);
      oyAllocHelper_m_( adaption_states, cmsFloat64Number, multi_profiles_n,0, goto end);
      SET_ARR(intents,intent,multi_profiles_n);
      SET_ARR(bpc,flags & cmsFLAGS_BLACKPOINTCOMPENSATION,multi_profiles_n);
      SET_ARR(adaption_states,adaption_state,multi_profiles_n);
      xform = lcmsCreateExtendedTransform( 0, multi_profiles_n, lps, bpc,
                                          intents, adaption_states, NULL, 0,
                                          lcm2_pixel_layout_in,
                                          lcm2_pixel_layout_out, flags | cmsFLAGS_KEEP_SEQUENCE );
      if(oy_debug >= 2)
      {
        int i;
        lcm2_msg( oyMSG_DBG, (oyStruct_s*)opts,
                OY_DBG_FORMAT_"lcmsCreateExtendedTransform(multi_profiles_n %d)"
                " xform: "OY_PRINT_POINTER,
                OY_DBG_ARGS_, multi_profiles_n, xform, ltw );
#if LCMS_VERSION >= 2060
        for(i = 0; i < multi_profiles_n; ++i)
        {
          oyProfile_s * p = lcmsGetContextUserData( lcmsGetProfileContextID( lps[i] ) );
          const char * fn = oyProfile_GetFileName( p, -1 );
          size_t size = 0;
          char * block = oyProfile_GetMem( p, &size, 0, oyAllocateFunc_ );
          oyFree_m_(block);
          fprintf( stdout, " -> \"%s\"[%lu]", fn?fn:"----", (long unsigned int)size );
        }
        fprintf(stdout, "\n");
#endif
      }

#ifdef ENABLE_MPE
      unsigned char in[3] = {128,128,128};
      unsigned short o[3];
      lcmsDoTransform( xform, in, o, 1 );
      printf("%d %d %d\n", o[0],o[1],o[2]);
#endif /* ENABLE_MPE */

      oyFree_m_( intents );
      oyFree_m_( bpc );
      oyFree_m_( adaption_states );
    }
    if(intents) free(intents);
  }

  if(!xform || oy_debug > 2)
  {
    int level = oyMSG_DBG;
    uint32_t f = lcm2_pixel_layout_in, i;

    if(!xform)
    {
      level = oyMSG_WARN;
      error = 1;
    }

    lcm2_msg( level, (oyStruct_s*)opts, OY_DBG_FORMAT_
              " float:%d optimised:%d colorspace:%d extra:%d channels:%d lcms_bytes %d",
              OY_DBG_ARGS_,
              T_FLOAT(f), T_OPTIMIZED(f), T_COLORSPACE(f), T_EXTRA(f), T_CHANNELS(f), T_BYTES(f) );
    f = lcm2_pixel_layout_out;
    lcm2_msg( level, (oyStruct_s*)opts, OY_DBG_FORMAT_
              "float:%d optimised:%d colorspace:%d extra:%d channels:%d lcms_bytes %d",
              OY_DBG_ARGS_,
              T_FLOAT(f), T_OPTIMIZED(f), T_COLORSPACE(f), T_EXTRA(f), T_CHANNELS(f), T_BYTES(f) );
    lcm2_msg( level, (oyStruct_s*)opts, OY_DBG_FORMAT_
              "multi_profiles_n: %d intent: %d adaption: %g \"%s\"",
              OY_DBG_ARGS_,
              multi_profiles_n, intent, adaption_state,
              lcm2FlagsToText(flags));
    for(i=0; i < profiles_n; ++i)
      lcm2_msg( level,(oyStruct_s*)node, OY_DBG_FORMAT_"\n"
             "  ColorSpace:%s->PCS:%s DeviceClass:%s",
             OY_DBG_ARGS_,
             lps[0]?oyICCColorSpaceGetName(lcmsGetColorSpace( lps[0])):"----",
             lps[i]?oyICCColorSpaceGetName(lcmsGetPCS( lps[i] )):"----",
             lps[i]?oyICCDeviceClassDescription(lcmsGetDeviceClass(lps[i])):"----" );
  }

  if(!error && ltw && oy)
    *ltw= lcm2TransformWrap_Set_( xform, color_in, color_out,
                                  oy_pixel_layout_in, oy_pixel_layout_out, oy );

  end:
  return xform;
}

/** Function lcm2CMMColorConversion_ToMem_
 *
 *  convert a lcm2 color conversion context to a device link
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/28 (Oyranos: 0.1.10)
 *  @date    2008/12/28
 */
oyPointer  lcm2CMMColorConversion_ToMem_ (
                                       cmsHTRANSFORM     * xform,
                                       oyOptions_s       * opts,
                                       size_t            * size,
                                       oyAlloc_f           allocateFunc )
{
  int error = !xform;
  oyPointer data = 0;
  cmsUInt32Number size_ = 0;
  int flags = lcm2FlagsFromOptions( opts );

  if(!error)
  {
    cmsHPROFILE dl= lcmsTransform2DeviceLink( xform, 4.3,
                                              flags | cmsFLAGS_KEEP_SEQUENCE );

    *size = 0;

#if 0
    {
        int nargs = 1, i;
        size_t size = sizeof(int) + nargs * sizeof(cmsPSEQDESC);
        LPcmsSEQ pseq = (LPcmsSEQ) oyAllocateFunc_(size);
        
        ZeroMemory(pseq, size);
        pseq ->n = nargs;

        for (i=0; i < nargs; i++) {

            strcpy(pseq ->seq[i].Manufacturer, CMM_NICK);
            strcpy(pseq ->seq[i].Model, "CMM ");
        }

        cmsAddTag(dl, icSigProfileSequenceDescTag, pseq);
        free(pseq);
    }
#endif

    if(dl)
      lcmsSaveProfileToMem( dl, 0, &size_ );
    else
      lcm2_msg( oyMSG_WARN, (oyStruct_s*)opts,
                OY_DBG_FORMAT_"no DL from transform. flags: %s",
                OY_DBG_ARGS_, lcm2FlagsToText(flags) );
    if(size_)
    {
      data = allocateFunc( size_ );
      lcmsSaveProfileToMem( dl, data, &size_ );
    } else
      lcm2_msg( oyMSG_WARN, (oyStruct_s*)opts,
                OY_DBG_FORMAT_"can not convert CMM profile to memory",
                OY_DBG_ARGS_ );
    *size = size_;
  }

  return data;
}

oyOptions_s* lcm2Filter_CmmIccValidateOptions
                                     ( oyFilterCore_s    * filter,
                                       oyOptions_s       * validate,
                                       int                 statical,
                                       uint32_t          * result )
{
  uint32_t error = !filter;

  if(!error)
    error = oyFilterRegistrationMatch(oyFilterCore_GetRegistration(filter),
                                      "//"OY_TYPE_STD"/icc_color",
                                      oyOBJECT_CMM_API4_S);

  *result = error;

  return 0;
}

oyWIDGET_EVENT_e   lcm2WidgetEvent   ( oyOptions_s       * options,
                                       oyWIDGET_EVENT_e    type,
                                       oyStruct_s        * event )
{return 0;}


oyDATATYPE_e lcm2_cmmIcc_data_types[7] = {oyUINT8, oyUINT16, oyHALF, oyFLOAT, oyDOUBLE, 0};

oyConnectorImaging_s_ lcm2_cmmIccSocket_connector = {
  oyOBJECT_CONNECTOR_IMAGING_S,0,0,
                               (oyObject_s)&oy_connector_imaging_static_object,
  oyCMMgetImageConnectorSocketText, /* getText */
  oy_image_connector_texts, /* texts */
  "//" OY_TYPE_STD "/manipulator.data", /* connector_type */
  oyFilterSocket_MatchImagingPlug, /* filterSocket_MatchPlug */
  0, /* is_plug == oyFilterPlug_s */
  lcm2_cmmIcc_data_types, /* data_types */
  3, /* data_types_n; elements in data_types array */
  1, /* max_color_offset */
  1, /* min_channels_count; */
  16, /* max_channels_count; */
  1, /* min_color_count; */
  16, /* max_color_count; */
  1, /* can_planar; can read separated channels */
  1, /* can_interwoven; can read continuous channels */
  1, /* can_swap; can swap color channels (BGR)*/
  1, /* can_swap_bytes; non host byte order */
  1, /* can_revert; revert 1 -> 0 and 0 -> 1 */
  1, /* can_premultiplied_alpha; */
  1, /* can_nonpremultiplied_alpha; */
  0, /* can_subpixel; understand subpixel order */
  0, /* oyCHANNELTYPE_e    * channel_types; */
  0, /* channel_types_n */
  1, /* id; relative to oyFilterCore_s, e.g. 1 */
  0  /* is_mandatory; mandatory flag */
};
oyConnectorImaging_s_* lcm2_cmmIccSocket_connectors[2]={&lcm2_cmmIccSocket_connector,0};

oyConnectorImaging_s_ lcm2_cmmIccPlug_connector = {
  oyOBJECT_CONNECTOR_IMAGING_S,0,0,
                               (oyObject_s)&oy_connector_imaging_static_object,
  oyCMMgetImageConnectorPlugText, /* getText */
  oy_image_connector_texts, /* texts */
  "//" OY_TYPE_STD "/manipulator.data", /* connector_type */
  oyFilterSocket_MatchImagingPlug, /* filterSocket_MatchPlug */
  1, /* is_plug == oyFilterPlug_s */
  lcm2_cmmIcc_data_types, /* data_types */
  3, /* data_types_n; elements in data_types array */
  1, /* max_color_offset */
  1, /* min_channels_count; */
  16, /* max_channels_count; */
  1, /* min_color_count; */
  16, /* max_color_count; */
  1, /* can_planar; can read separated channels */
  1, /* can_interwoven; can read continuous channels */
  1, /* can_swap; can swap color channels (BGR)*/
  1, /* can_swap_bytes; non host byte order */
  1, /* can_revert; revert 1 -> 0 and 0 -> 1 */
  1, /* can_premultiplied_alpha; */
  1, /* can_nonpremultiplied_alpha; */
  0, /* can_subpixel; understand subpixel order */
  0, /* oyCHANNELTYPE_e    * channel_types; */
  0, /* channel_types_n */
  1, /* id; relative to oyFilterCore_s, e.g. 1 */
  0  /* is_mandatory; mandatory flag */
};
oyConnectorImaging_s_* lcm2_cmmIccPlug_connectors[2]={&lcm2_cmmIccPlug_connector,0};

/** Function lcm2AddProofProfile
 *  @brief   add a abstract proofing profile to the lcm2 profile stack 
 *
 *  Look in the Oyranos cache for a CMM internal representation or generate a
 *  new abstract profile containing the proofing profiles changes. This can be
 *  a proofing color space simulation or out of gamut marking.
 *
 *  @version Oyranos: 0.9.6
 *  @date    2016/05/02
 *  @since   2009/11/05 (Oyranos: 0.1.10)
 */
lcm2ProfileWrap_s*lcm2AddProofProfile( oyProfile_s       * proof,
                                       cmsUInt32Number     flags,
                                       int                 intent,
                                       int                 intent_proof,
                                       uint32_t            icc_profile_flags )
{
  int error = 0;
  cmsHPROFILE * hp = 0;
  oyPointer_s * cmm_ptr = 0;
  lcm2ProfileWrap_s * s = 0;
  char * hash_text = 0,
       num[12];

  if(!proof || proof->type_ != oyOBJECT_PROFILE_S)
  {
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)proof, OY_DBG_FORMAT_
              "no profile provided %s", OY_DBG_ARGS_,
              (proof != NULL) ? oyStruct_GetText( (oyStruct_s*) proof->type_, oyNAME_NAME, 0 ) : "" );
    return 0;
  }

  /* build hash text */
  STRING_ADD( hash_text, "abstract proofing profile " );
  STRING_ADD( hash_text, oyObject_GetName( proof->oy_, oyNAME_NICK ) );
  STRING_ADD( hash_text, " intent:" );
  sprintf( num, "%d", intent );
  STRING_ADD( hash_text, num );
  STRING_ADD( hash_text, " intent_proof:" );
  sprintf( num, "%d", intent_proof );
  STRING_ADD( hash_text, num );
  STRING_ADD( hash_text, " flags|gmtCheck|softPrf:" );
  sprintf( num, "%d|%d|%d", (int)flags, (flags & cmsFLAGS_GAMUTCHECK)?1:0,
                                        (flags & cmsFLAGS_SOFTPROOFING)?1:0 );
  STRING_ADD( hash_text, num );

  /* cache look up */
  cmm_ptr = oyPointer_LookUpFromText( hash_text, lcm2PROFILE );

  oyPointer_Set( cmm_ptr, CMM_NICK, 0,0,0,0 );

  /* for empty profile create a new abstract one */
  if(!oyPointer_GetPointer(cmm_ptr))
  {
    oyPointer_s * oy = cmm_ptr;

    char * type_ = lcm2PROFILE;
    uint32_t type = *((uint32_t*)type_);
    cmsUInt32Number size = 0;
    oyPointer block = 0;
    lcm2ProfileWrap_s * s = calloc(sizeof(lcm2ProfileWrap_s), 1);

    if(oy_debug > 3)
      fprintf( stderr, OY_DBG_FORMAT_" created: \"%s\"",
               OY_DBG_ARGS_, hash_text );
    else
    lcm2_msg( oyMSG_DBG, (oyStruct_s*)proof,
             OY_DBG_FORMAT_" created abstract proofing profile: \"%s\"",
             OY_DBG_ARGS_, hash_text );
 
    /* create */
    hp = lcm2GamutCheckAbstract( proof, flags, intent, intent_proof, icc_profile_flags );
    if(hp)
    {
      /* save to memory */
      if(!lcmsSaveProfileToMem( hp, 0, &size ))
        lcm2_msg( oyMSG_WARN, (oyStruct_s*)proof,
             OY_DBG_FORMAT_"lcmsSaveProfileToMem failed for \"%s\" %s",
             OY_DBG_ARGS_, hash_text, oyProfile_GetFileName( proof, -1 ) );
      block = oyAllocateFunc_( size );
      lcmsSaveProfileToMem( hp, block, &size );
      lcmsCloseProfile( hp ); hp = 0;
    }

    s->type = type;
    s->size = size;
    s->block = block;

    /* reopen */
#if LCMS_VERSION < 2060
    s->lcm2 = CMMProfileOpen_M( proof, block, size );
#else
    {
      oyProfile_s * proof2 = oyProfile_Copy( proof, NULL );
      cmsContext tc = lcmsCreateContext( NULL, proof2 ); /* threading context */
      lcmsSetLogErrorHandlerTHR( tc, lcm2ErrorHandlerFunction );
      s->lcm2 = CMMProfileOpen_M( tc, block, size );
    }
#endif
    error = oyPointer_Set( oy, 0,
                          lcm2PROFILE, s, CMMToString_M(CMMProfileOpen_M),
                          lcm2CMMProfileReleaseWrap );
  }

  if(!error)
  {
    s = lcm2CMMProfile_GetWrap_( cmm_ptr );
    error = !s;
  }

  oyPointer_Release( &cmm_ptr );
  if(hash_text)
    oyFree_m_(hash_text);

  if(!error)
    return s;
  else
    return 0;
}


/** Function lcm2AddProfile
 *  @brief   add a profile from Oyranos to the lcm2 profile stack 
 *
 *  Look in the Oyranos cache for a CMM internal representation
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/28 (Oyranos: 0.1.10)
 *  @date    2008/12/28
 */
cmsHPROFILE  lcm2AddProfile          ( oyProfile_s       * p )
{
  int error = 0;
  cmsHPROFILE * hp = 0;
  oyPointer_s * cmm_ptr = 0;
  lcm2ProfileWrap_s * s = 0;

  if(!p || p->type_ != oyOBJECT_PROFILE_S)
  {
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)p, OY_DBG_FORMAT_" "
             "no profile provided", OY_DBG_ARGS_ );
    return 0;
  }

  cmm_ptr = oyPointer_LookUpFromObject( (oyStruct_s*)p, lcm2PROFILE );
  if(oy_debug >= 2)
  {
    lcm2_msg( oyMSG_DBG, (oyStruct_s*)p,
              OY_DBG_FORMAT_" going to open %s cmm_ptr: %d", OY_DBG_ARGS_,
              p?oyProfile_GetFileName( p,-1 ):"????", oyStruct_GetId((oyStruct_s*)cmm_ptr) );
  }

  if(!cmm_ptr)
  {
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)p,
             OY_DBG_FORMAT_" oyPointer_LookUpFromObject() failed", OY_DBG_ARGS_ );
    return 0;
  }

  oyPointer_Set( cmm_ptr, CMM_NICK, 0,0,0,0 );

  if(!oyPointer_GetPointer(cmm_ptr))
    error = lcm2CMMData_Open( (oyStruct_s*)p, cmm_ptr );

  if(error)
  {
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)p,
             OY_DBG_FORMAT_" lcm2CMMData_Open() failed", OY_DBG_ARGS_ );
  } else
  {
    s = lcm2CMMProfile_GetWrap_( cmm_ptr );
    error = !s;
    if(error)
      lcm2_msg( oyMSG_WARN, (oyStruct_s*)p,
             OY_DBG_FORMAT_" lcm2CMMProfile_GetWrap_() failed", OY_DBG_ARGS_ );
  }

  if(!error)
    hp = s->lcm2;

  oyPointer_Release( &cmm_ptr );

  if(!error)
    return hp;
  else
    return 0;
}


int gamutCheckSampler16(const cmsUInt16Number In[],
                              cmsUInt16Number Out[],
                        void                * Cargo)
{
  cmsCIELab Lab1, Lab2;
  oyPointer * ptr = (oyPointer*)Cargo;

  lcmsLabEncoded2Float(&Lab1, In);
  lcmsDoTransform( ptr[0], In, Out, 1 );
  lcmsLabEncoded2Float(&Lab2, Out);
  /*double d = cmsDeltaE( &Lab1, &Lab2 );
  if(abs(d) > 10 && ptr[1] != NULL)
  {
    Lab2.L = 50.0;
    Lab2.a = Lab2.b = 0.0;
  }*/
  lcmsFloat2LabEncoded(Out, &Lab2); 

  return TRUE;
}

int  gamutCheckSamplerFloat          ( const cmsFloat32Number In[],
                                             cmsFloat32Number Out[],
                                       void                 * Cargo )
{
  cmsCIELab Lab1, Lab2;
  double d;
  cmsFloat32Number i[3], o[3];
  oyPointer * ptr = (oyPointer*)Cargo;

  i[0] = Lab1.L = In[0] * 100.0;
  i[1] = Lab1.a = In[1] * 257.0 - 128.0;
  i[2] = Lab1.b = In[2] * 257.0 - 128.0;

  lcmsDoTransform( ptr[0], i, o, 1 );

  Lab2.L = o[0]; Lab2.a = o[1]; Lab2.b = o[2];

  d = lcmsDeltaE( &Lab1, &Lab2 );
  if((fabs(d) > 10) && ptr[1] != NULL)
  {
    Lab2.L = 50.0;
    Lab2.a = Lab2.b = 0.0;
  }

  Out[0] = Lab2.L/100.0; 
  Out[1] = (Lab2.a + 128.0) / 257.0;
  Out[2] = (Lab2.b + 128.0) / 257.0;

  return TRUE;
}

const char *       oyICCMpeDescription(cmsStageSignature sig, int type )
{
#if LCMS_VERSION >= 2060
  switch ((unsigned int)sig)
  {
    // Multi process elements types
    case cmsSigCurveSetElemType: return type ? "cvst" : _("Curve Set");
    case cmsSigMatrixElemType:   return type ? "matf" : _("Matrix");
    case cmsSigCLutElemType:     return type ? "clut" : _("Look Up Table");

    case cmsSigBAcsElemType:     return type ? "bACS" : _("BAcs");
    case cmsSigEAcsElemType:     return type ? "eACS" : _("EAcs");

    // Custom from here, not in the ICC Spec
    case cmsSigXYZ2LabElemType:  return type ? "l2x " : _("XYZ2Lab");
    case cmsSigLab2XYZElemType:  return type ? "x2l " : _("Lab2XYZ");
    case cmsSigNamedColorElemType: return type ? "ncl " : _("Named Color");
    case cmsSigLabV2toV4:          return type ? "2 4 " : _("V2toV4");
    case cmsSigLabV4toV2:          return type ? "4 2 " : _("V4toV2");
  
    // Identities
    case cmsSigIdentityElemType:   return type ? "idn " : _("Identity");

    // Float to floatPCS
    case cmsSigLab2FloatPCS:       return type ? "d2l '" : _("Lab2FloatPCS");
    case cmsSigFloatPCS2Lab:       return type ? "l2d '" : _("FloatPCS2Lab");
    case cmsSigXYZ2FloatPCS:       return type ? "d2x '" : _("XYZ2FloatPCS");
#if LCMS_VERSION >= 2070
    case cmsSigFloatPCS2XYZ:       return type ? "x2d '" : _("FloatPCS2XYZ");
    case cmsSigClipNegativesElemType: return type ? "clp '" : _("Clip Negatives");
#endif /* >= 2070 */
    case 0: return _("----");
    default:
#endif /* >= 2060 */
             { static union { char c[8]; cmsStageSignature sig; } stage_sig = { .c[4] = 0 };
               stage_sig.sig = (cmsStageSignature)oyValueUInt32( sig );
               return stage_sig.c;
             }
#if LCMS_VERSION >= 2060
  }
#endif
}

// A single stage
struct _cmsStage_struct {

    cmsContext          ContextID;

    cmsStageSignature   Type;           // Identifies the stage
    cmsStageSignature   Implements;     // Identifies the *function* of the stage (for optimizations)
};

void printPipeline( cmsPipeline * lut )
{
  cmsStage * first = lcmsPipelineGetPtrToFirstStage(lut),
           * next = first;
  int i = 0;
  do {
    fprintf(stderr, "stage[%d] %s:%s-%s %d -> %d\n", i, oyICCMpeDescription(lcmsStageType(next),oyNAME_NICK),
            oyICCMpeDescription(next->Implements,oyNAME_NAME),
            oyICCMpeDescription(lcmsStageType(next),oyNAME_NAME), lcmsStageInputChannels(next), lcmsStageOutputChannels(next) );
    ++i;
  } while ((next = lcmsStageNext( next )) != NULL);
   
}

/** Function lcm2GamutCheckAbstract
 *  @brief   convert a proofing profile into a abstract one
 *
 *  Abstract profiles can easily be merged into a multi profile transform.
 *
 *  @param         proof               the proofing profile; owned by the
 *                                     function
 *  @param         flags               the gamut check and softproof flags
 *  @param         intent              rendering intent
 *  @param         intent_proof        proof rendering intent
 *
 *  @version Oyranos: 0.1.11
 *  @since   2009/11/04 (Oyranos: 0.1.10)
 *  @date    2010/08/14
 */
cmsHPROFILE  lcm2GamutCheckAbstract  ( oyProfile_s       * proof,
                                       cmsUInt32Number     flags,
                                       int                 intent,
                                       int                 intent_proof,
                                       uint32_t            icc_profile_flags )
{
  cmsUInt16Number OldAlarm[cmsMAXCHANNELS];
#if LCMS_VERSION >= 2060
      cmsContext tc = lcmsCreateContext( NULL, NULL ); /* threading context */
      lcmsSetLogErrorHandlerTHR( tc, lcm2ErrorHandlerFunction );
#else
      void * tc = NULL;
#endif
      cmsUInt32Number size = 0;
      char * data = 0;
      cmsHPROFILE gmt = 0,
                  hLab = 0,
                  hproof = 0;
      cmsHTRANSFORM tr = 0, tr16 = 0;
      cmsStage * gmt_lut = 0,
               * gmt_lut16 = 0;
      cmsPipeline * gmt_pl = lcmsPipelineAlloc( 0,3,3 ),
                  * gmt_pl16 = lcmsPipelineAlloc( 0,3,3 );
      cmsToneCurve * t[3] = {0,0,0},
                   * g[3] = {0,0,0};

      cmsHTRANSFORM
#ifdef ENABLE_MPE
                   ptr[2] = {0,0},
#endif /* ENABLE_MPE */
                 ptr16[2] = {0,0};
      int r = 0, i, done = 0, done_16 = 0;
      cmsMLU * mlu[2] = {0,0};
#ifdef ENABLE_MPE
      cmsFloat64Number params[4] = {0.0, 0.0, 0.0, 0.0};
#endif /* ENABLE_MPE */

      lcm2_msg( oyMSG_DBG, (oyStruct_s*)proof, OY_DBG_FORMAT_
                "softproofing %d gamutcheck %d intent %d intent_proof %d", OY_DBG_ARGS_,
                flags & cmsFLAGS_SOFTPROOFING,
                flags & cmsFLAGS_GAMUTCHECK,
                intent, intent_proof );

      if(!(flags & cmsFLAGS_GAMUTCHECK || flags & cmsFLAGS_SOFTPROOFING))
        return gmt;

      hLab  = lcmsCreateLab4ProfileTHR(tc, lcmsD50_xyY());
#if LCMS_VERSION < 2060
      hproof = lcm2AddProfile( proof );
#else
      {
        const char * fn = oyProfile_GetFileName( proof, -1 );
        hproof = lcmsOpenProfileFromFileTHR( tc, fn, "r" );
      }
#endif

      if(!hLab || !hproof)
      { lcm2_msg( oyMSG_ERROR, (oyStruct_s*)proof, OY_DBG_FORMAT_
                 "hLab or hproof failed", OY_DBG_ARGS_);
                 goto clean; }

#if LCMS_VERSION >= 2060 && defined(USE_OPENMP)
#pragma omp parallel for private(i)
#endif
  for(i = 0; i < 2; ++i)
  {
    if(i)
    {
      /* disable float mpe as the tag appears buggy in Oyranos or lcms */
#ifdef ENABLE_MPE
#if LCMS_VERSION >= 2060
      cmsContext tc = lcmsCreateContext( NULL, NULL ); /* threading context */
      lcmsSetLogErrorHandlerTHR( tc, lcm2ErrorHandlerFunction );
#else
      void * tc = NULL;
#endif
      int error = 0;
      tr = lcmsCreateProofingTransformTHR (  tc,
                                               hLab, TYPE_Lab_FLT,
                                               hLab, TYPE_Lab_FLT,
                                               hproof,
                                               intent,
            /* TODO The INTENT_ABSOLUTE_COLORIMETRIC should lead to 
               paper simulation, but does take white point into account.
               Do we want this?
             */
                                               intent_proof,
                                               flags | cmsFLAGS_KEEP_SEQUENCE);
      if(!tr) { lcm2_msg( oyMSG_ERROR, (oyStruct_s*)proof, OY_DBG_FORMAT_
                          "cmsCreateProofingTransform() failed", OY_DBG_ARGS_);
                error = 1; }
      ptr[0] = tr;
      ptr[1] = flags & cmsFLAGS_GAMUTCHECK ? (oyPointer)1 : 0;
      if(!error)
      {
        gmt_lut = lcmsStageAllocCLutFloat( tc,lcm2PROOF_LUT_GRID_RASTER, 3,3, 0);
        r = lcmsStageSampleCLutFloat( gmt_lut, gamutCheckSamplerFloat, ptr, 0 );
        if(!r) { lcm2_msg( oyMSG_ERROR, (oyStruct_s*)proof, OY_DBG_FORMAT_
                          "cmsStageSampleCLutFloat() failed", OY_DBG_ARGS_);
                 error = 1; }
      }
      done = 1;
#endif /* ENABLE_MPE */

    } else
    {
#if LCMS_VERSION >= 2060
      cmsContext tc = lcmsCreateContext( NULL, NULL ); /* threading context */
      lcmsSetLogErrorHandlerTHR( tc, lcm2ErrorHandlerFunction );
#else
      void * tc = NULL;
#endif
      int error = 0;
      tr16 = lcmsCreateProofingTransformTHR( tc,
                                               hLab, TYPE_Lab_16,
                                               hLab, TYPE_Lab_16,
                                               hproof,
                                               intent,
            /* TODO The INTENT_ABSOLUTE_COLORIMETRIC should lead to 
               paper simulation, but does take white point into account.
               Do we want this?
             */
                                               intent_proof,
                                               flags | cmsFLAGS_KEEP_SEQUENCE);
      if(!tr16) { lcm2_msg( oyMSG_ERROR, (oyStruct_s*)proof, OY_DBG_FORMAT_
                          "cmsCreateProofingTransform() failed", OY_DBG_ARGS_);
                  error = 1; }
      ptr16[0] = tr16;
      ptr16[1] = flags & cmsFLAGS_GAMUTCHECK ? (oyPointer)1 : 0;

      if(!error)
      {
        gmt_lut16 = lcmsStageAllocCLut16bit( tc, lcm2PROOF_LUT_GRID_RASTER,3,3,0);
        r = lcmsStageSampleCLut16bit( gmt_lut16, gamutCheckSampler16, ptr16, 0);
        if(!r)   { lcm2_msg( oyMSG_ERROR, (oyStruct_s*)proof, OY_DBG_FORMAT_
                          "cmsStageSampleCLut16bit() failed", OY_DBG_ARGS_);
                   error = 1; }
      }
      done_16 = 1;
    }
  }

      if(
#ifdef ENABLE_MPE
         /*!gmt_lut || !done || */
#endif /* ENABLE_MPE */
         !gmt_lut16 || !done_16)
      {
        lcm2_msg( oyMSG_WARN, (oyStruct_s*)proof, OY_DBG_FORMAT_ " "
                 "failed to build: %s %s %s %s",
                 OY_DBG_ARGS_, gmt_lut?"lut":"", gmt_lut16?"lut16":"",
                 done?"done":"not ready", done_16?"done":"not ready" );
        goto clean;
      }

      gmt = lcmsCreateProfilePlaceholder( tc ); if(!gmt) goto clean;
      if(icc_profile_flags & OY_ICC_VERSION_2)
        lcmsSetProfileVersion( gmt, 2.4 );
      else
        lcmsSetProfileVersion( gmt, 4.2 );
      lcmsSetDeviceClass( gmt, icSigAbstractClass );
      lcmsSetColorSpace( gmt, icSigLabData );
      lcmsSetPCS( gmt, icSigLabData );
#define E if(!r) { lcm2_msg( oyMSG_ERROR, (oyStruct_s*)proof, \
                   OY_DBG_FORMAT_ "could not write tag", OY_DBG_ARGS_); \
                   if(gmt) lcmsCloseProfile( gmt ); gmt = 0; \
                   goto clean; }
      mlu[0] = lcmsMLUalloc(tc,1);
      mlu[1] = lcmsMLUalloc(tc,1);
      r = lcmsMLUsetASCII(mlu[0], "EN", "us", "proofing"); E
      r = lcmsWriteTag( gmt, (cmsTagSignature)icSigProfileDescriptionTag, mlu[0] ); E
      r = lcmsMLUsetASCII(mlu[1], "EN", "us", "no copyright; use freely"); E
      r = lcmsWriteTag( gmt, (cmsTagSignature)icSigCopyrightTag, mlu[1]); E
      r = lcmsWriteTag( gmt, (cmsTagSignature)icSigMediaWhitePointTag, lcmsD50_XYZ() ); E

#if ENABLE_MPE
      /* set parametric unbound curve, type 6: (aX + b) ^ y + c */
#if 0
      /* Initialize segmented curve
         Segment 0: from minus infinite */
      cmsCurveSegment seg[2];
      size =  sizeof(seg);
      memset( seg, 0, size );
      seg[0].x0 = -1.0;
      seg[0].x1 = 1.0;
      seg[0].Type = 6;

      seg[0].Params[0] = 1;
      seg[0].Params[1] = 1;
      seg[0].Params[2] = 0;
      seg[0].Params[3] = 0;
      seg[0].Params[4] = 0;

      t[0] = t[1] = t[2] = lcmsBuildSegmentedToneCurve(tc, 2, seg);
#else
      params[0] = 1.0; /* y - gamma */
      params[1] = 1.0; /* a */
      params[2] = 0.0; /* b */
      params[3] = 0.0; /* c */

      t[0] = t[1] = t[2] = lcmsBuildParametricToneCurve(tc, 6, params);
#endif
      if(!t[0])
      {
        lcm2_msg( oyMSG_WARN, (oyStruct_s*)proof, OY_DBG_FORMAT_ " "
                 "failure in cmsBuildSegmentedToneCurve,  used version:%d",
                 OY_DBG_ARGS_, lcmsGetEncodedCMMversion() );
        goto clean;
      }
      /* float */
      /* cmsPipeline owns the cmsStage memory */
      lcmsPipelineInsertStage( gmt_pl, cmsAT_BEGIN,
                              lcmsStageAllocToneCurves( tc, 3, t ) );
      lcmsPipelineInsertStage( gmt_pl, cmsAT_END, gmt_lut );
      lcmsPipelineInsertStage( gmt_pl, cmsAT_END,
                              lcmsStageAllocToneCurves( tc, 3, t ) );
      r = lcmsWriteTag( gmt, cmsSigDToB0Tag, gmt_pl ); E
      if(oy_debug) printPipeline(gmt_pl);
#endif /* ENABLE_MPE */

      /* 16-bit int */
      g[0] = g[1] = g[2] = lcmsBuildGamma(tc, 1.0);
      lcmsPipelineInsertStage( gmt_pl16, cmsAT_BEGIN,
                              lcmsStageAllocToneCurves( tc, 3, g ) );
      lcmsPipelineInsertStage( gmt_pl16, cmsAT_END, gmt_lut16 );
      lcmsPipelineInsertStage( gmt_pl16, cmsAT_END,
                              lcmsStageAllocToneCurves( tc, 3, g ) );
      r = lcmsWriteTag( gmt, cmsSigAToB0Tag, gmt_pl16 ); E
      if(oy_debug) printPipeline(gmt_pl16);
#undef E

  if(oy_debug && getenv("OY_DEBUG_WRITE"))
  {
      char * t = 0; oyStringAddPrintf( &t, 0,0,
      "%04d-%s-abstract-proof[%d].ppm", ++oy_debug_write_id,CMM_NICK,oyStruct_GetId((oyStruct_s*)proof));
      lcmsSaveProfileToMem( gmt, 0, &size );
      data = oyAllocateFunc_( size );
      lcmsSaveProfileToMem( gmt, data, &size );
      oyWriteMemToFile_( t, data, size );
      if(data) oyDeAllocateFunc_( data ); data = 0;
      oyFree_m_(t);
  }

  lcmsGetAlarmCodes(OldAlarm);

  clean:

      if(hLab) lcmsCloseProfile( hLab ); hLab = 0;
      if(tr) lcmsDeleteTransform( tr ); tr = 0;
      if(tr16) lcmsDeleteTransform( tr16 ); tr16 = 0;
      if(t[0])  lcmsFreeToneCurve( t[0] );
      if(g[0])  lcmsFreeToneCurve( g[0] );
      if(gmt_pl) lcmsPipelineFree( gmt_pl );
      if(gmt_pl16) lcmsPipelineFree( gmt_pl16 );
      if(mlu[0]) lcmsMLUfree( mlu[0] ); mlu[0] = 0;
      if(mlu[1]) lcmsMLUfree( mlu[1] ); mlu[1] = 0;

  oyProfile_Release( &proof );

  return gmt;
}

/**
 *  This function implements oyMOptions_Handle_f.
 *
 *  @version Oyranos: 0.3.0
 *  @since   2011/02/21 (Oyranos: 0.3.0)
 *  @date    2011/02/21
 */
int          lcm2MOptions_Handle2    ( oyOptions_s       * options,
                                       const char        * command,
                                       oyOptions_s      ** result )
{
  int error = 0;
  oyProfile_s * prof = 0,
              * p = 0;

  if(oyFilterRegistrationMatch(command,"can_handle", 0))
  {
    if(oyFilterRegistrationMatch(command,"create_profile", 0))
    {
      p = (oyProfile_s*) oyOptions_GetType( options,-1, "proofing_profile",
                                            oyOBJECT_PROFILE_S );
      if(!p)
      {
        error = -1;
      }

      oyProfile_Release( &p );

      return error;
    }
    else
      return -1;
  }
  else if(oyFilterRegistrationMatch(command,"create_profile", 0))
  {
    int32_t icc_profile_flags = 0;
    oyOptions_FindInt( options, "icc_profile_flags", 0, &icc_profile_flags ); 

    p = (oyProfile_s*) oyOptions_GetType( options,-1, "proofing_profile",
                                          oyOBJECT_PROFILE_S );
    if(p)
    {
      int intent = lcm2IntentFromOptions( options,0 ),
      intent_proof = lcm2IntentFromOptions( options,1 ),
      flags = lcm2FlagsFromOptions( options );
      oyOption_s * o;

      lcm2ProfileWrap_s * wrap = lcm2AddProofProfile( p, flags | cmsFLAGS_SOFTPROOFING,
                                            intent, intent_proof, icc_profile_flags );
      oyProfile_Release( &p );

      prof = oyProfile_FromMem( wrap->size, wrap->block, 0, 0 );

      o = oyOption_FromRegistration( OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD OY_SLASH "icc_profile.create_profile.proofing_effect._" CMM_NICK,
                        0 );
      error = oyOption_MoveInStruct( o, (oyStruct_s**) &prof );
      if(!*result)
        *result = oyOptions_New(0);
      oyOptions_MoveIn( *result, &o, -1 );
    } else
        lcm2_msg( oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ " "
                 "no option \"proofing_effect\" of type oyProfile_s found",
                 OY_DBG_ARGS_ );
  }

  return 0;
}

oyProfiles_s * lcm2ProfilesFromOptions( oyFilterNode_s * node, oyFilterPlug_s * plug,
                                        oyOptions_s * node_options,
                                        const char * key, int profiles_switch, int verbose )
{
  oyProfiles_s * profiles = NULL;
  oyOption_s * o = NULL;
  
  if(profiles_switch || oy_debug || verbose)
    o = oyOptions_Find( node_options, key, oyNAME_PATTERN );
  if(o)
  {
    profiles = (oyProfiles_s*) oyOption_GetStruct( o, oyOBJECT_PROFILES_S );
    if((oy_debug || verbose))
    {
      lcm2_msg( oyMSG_WARN, (oyStruct_s*)node, OY_DBG_FORMAT_
               " found \"%s\" %d  switch %d",
               OY_DBG_ARGS_, key, oyProfiles_Count( profiles ), profiles_switch );
    } else
    if( !profiles )
    {
      oyFilterSocket_Callback( plug, oyCONNECTOR_EVENT_INCOMPATIBLE_OPTION );
      lcm2_msg( oyMSG_WARN, (oyStruct_s*)node, OY_DBG_FORMAT_
               " incompatible \"%s\"", OY_DBG_ARGS_, key );
      
    }
    oyOption_Release( &o );
  }

  if(!profiles_switch)
    oyProfiles_Release( &profiles );

  return profiles;
}

/** Function lcm2FilterNode_CmmIccContextToMem
 *  @brief   implement oyCMMFilterNode_CreateContext_f()
 *
 *  @version Oyranos: 0.1.8
 *  @since   2008/11/01 (Oyranos: 0.1.8)
 *  @date    2008/11/01
 */
oyPointer lcm2FilterNode_CmmIccContextToMem (
                                       oyFilterNode_s    * node,
                                       size_t            * size,
                                       oyAlloc_f           allocateFunc )
{
  oyPointer block = 0;
  int error = 0;
  int n,i,len;
  size_t size_ = 0;
  oyFilterPlug_s * plug = oyFilterNode_GetPlug( node, 0 );
  oyFilterSocket_s * socket = oyFilterNode_GetSocket( node, 0 ),
                   * remote_socket = oyFilterPlug_GetSocket( plug );
  oyImage_s * image_input = 0,
            * image_output = 0;
  cmsHPROFILE * lps = 0;
  cmsHTRANSFORM xform = 0;
  oyOptions_s * node_tags = oyFilterNode_GetTags( node ),
              * node_options = oyFilterNode_GetOptions( node, 0 );
  oyProfile_s * p = 0,
              * prof = 0,
              * image_input_profile,
              * image_output_profile;
  oyProfiles_s * profiles = 0,
               * profs = 0;
  oyProfileTag_s * psid = 0,
                 * info = 0,
                 * cprt = 0;
  int profiles_n = 0,
      profiles_simulation_n = 0,
      proof = 0,
      effect_switch = 0;
  int verbose = oyOptions_FindString( node_tags, "verbose", "true" ) ? 1 : 0;

  image_input = (oyImage_s*)oyFilterSocket_GetData( remote_socket );
  image_output = (oyImage_s*)oyFilterSocket_GetData( socket );
  image_input_profile = oyImage_GetProfile( image_input );
  image_output_profile = oyImage_GetProfile( image_output );

  if(!image_input)
    return 0;

  if(image_input->type_ != oyOBJECT_IMAGE_S)
  {
    oyFilterSocket_Callback( plug, oyCONNECTOR_EVENT_INCOMPATIBLE_DATA );
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)node,
             OY_DBG_FORMAT_" missed input image %d", OY_DBG_ARGS_,
             image_input->type_ );
  }
  if(!image_output || image_output->type_ != oyOBJECT_IMAGE_S)
  {
    oyFilterSocket_Callback( plug, oyCONNECTOR_EVENT_INCOMPATIBLE_DATA );
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)node,
             OY_DBG_FORMAT_" missed output image %d", OY_DBG_ARGS_, image_output?image_output->type_:0 );
  }

  /*oyDATATYPE_e data_type = 0;
  data_type = oyToDataType_m( oyImage_GetPixelLayout( image_input, oyLAYOUT ) );

  if(data_type == oyHALF)
  {
    oyFilterSocket_Callback( plug, oyCONNECTOR_EVENT_INCOMPATIBLE_DATA );
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)node,
             OY_DBG_FORMAT_" can not handle oyHALF", OY_DBG_ARGS_ );
  }*/

  len = sizeof(cmsHPROFILE) * (15 + 2 + 1);
  lps = oyAllocateFunc_( len );
  memset( lps, 0, len );

  /* input profile */
  lps[ profiles_n++ ] = lcm2AddProfile( image_input_profile );
  if(!image_input_profile)
  {
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)node, OY_DBG_FORMAT_" "
             "missed image_input->profile_", OY_DBG_ARGS_ );
    return 0;
  }
  p = oyProfile_Copy( image_input_profile, 0 );
  profs = oyProfiles_New( 0 );
  error = oyProfiles_MoveIn( profs, &p, -1 );

  /* effect profiles */
  effect_switch = oyOptions_FindString  ( node_options, "effect_switch", "1" ) ? 1 : 0;
  profiles = lcm2ProfilesFromOptions( node, plug, node_options,
                                      "profiles_effect", effect_switch, verbose );
  n = oyProfiles_Count( profiles );
  if(n)
    for(i = 0; i < n; ++i)
    {
        p = oyProfiles_Get( profiles, i );

        /* Look in the Oyranos cache for a CMM internal representation */
        lps[ profiles_n++ ] = lcm2AddProfile( p );
        error = oyProfiles_MoveIn( profs, &p, -1 );
    }
  oyProfiles_Release( &profiles );


  /* simulation profile */
  proof = oyOptions_FindString  ( node_options, "proof_soft", "1" ) ? 1 : 0;
  proof += oyOptions_FindString  ( node_options, "proof_hard", "1" ) ? 1 : 0;

  if(oy_debug  > 2 && proof)
      lcm2_msg( oyMSG_DBG, (oyStruct_s*)node, OY_DBG_FORMAT_
               " proof requested",OY_DBG_ARGS_);

  profiles = lcm2ProfilesFromOptions( node, plug, node_options,
                                      "profiles_simulation", proof, verbose );
  n = oyProfiles_Count( profiles );
  if(n)
    for(i = 0; i < n; ++i)
    {
      p = oyProfiles_Get( profiles, i );

      if(oy_debug)
        lcm2_msg( oyMSG_DBG,(oyStruct_s*)node, OY_DBG_FORMAT_
                  " found profile: %s",
                  OY_DBG_ARGS_, p?oyProfile_GetFileName( p,-1 ):"????");

      error = oyProfiles_MoveIn( profs, &p, -1 );
      ++profiles_simulation_n;

      oyProfile_Release( &p );
    }
  else
    if(verbose || oy_debug > 2)
      lcm2_msg( oyMSG_DBG,(oyStruct_s*)node, OY_DBG_FORMAT_
                " no simulation profile found", OY_DBG_ARGS_);


  /* output profile */
  if(!image_output_profile)
  {
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)node, OY_DBG_FORMAT_" "
             "missed image_output->profile_", OY_DBG_ARGS_ );
    return 0;
  }
  lps[ profiles_n++ ] = lcm2AddProfile( image_output_profile );
  p = oyProfile_Copy( image_output_profile, 0 );
  error = oyProfiles_MoveIn( profs, &p, -1 );

  *size = 0;

  /* create the context */
  xform = lcm2CMMConversionContextCreate_( node, lps, profiles_n,
                                           profiles, profiles_simulation_n, proof,
                                oyImage_GetPixelLayout( image_input, oyLAYOUT ),
                                oyImage_GetPixelLayout( image_output, oyLAYOUT ),
                                           node_options, 0, 0 );
  if(oy_debug > 3)
    lcm2_msg( oyMSG_DBG, (oyStruct_s*)node, OY_DBG_FORMAT_"\n%s",
              OY_DBG_ARGS_,
              oyFilterNode_GetText( node, oyNAME_NAME ) );
  error = !xform;

  if(!error)
  {
    if(oy_debug)
      block = lcm2CMMColorConversion_ToMem_( xform, node_options,
                                              size, oyAllocateFunc_ );
    else
      block = lcm2CMMColorConversion_ToMem_( xform, node_options,
                                              size, allocateFunc );
    error = !block || !*size;
    lcmsDeleteTransform( xform ); xform = 0;
  } else
  {
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)node, OY_DBG_FORMAT_"\n"
              "loading failed profiles_n:%d profiles_simulation_n:%d profiles:%d",
              OY_DBG_ARGS_,
              profiles_n, profiles_simulation_n, oyProfiles_Count(profiles) );
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)node, OY_DBG_FORMAT_"\n"
              "  input profile: \"%s\" %s %s->%s %s\n  %s",
              OY_DBG_ARGS_,
              oyProfile_GetText( image_input_profile, oyNAME_DESCRIPTION ),
              oyProfile_GetText( image_input_profile, oyNAME_NAME ),
              oyICCColorSpaceGetName( oyProfile_GetSignature( 
                            image_input_profile, oySIGNATURE_COLOR_SPACE ) ),
              oyICCColorSpaceGetName( oyProfile_GetSignature( 
                            image_input_profile, oySIGNATURE_PCS ) ),
              oyICCDeviceClassDescription( oyProfile_GetSignature( 
                            image_input_profile, oySIGNATURE_CLASS ) ),
              oyPixelPrint(oyImage_GetPixelLayout( image_input, oyLAYOUT ), malloc));
    lcm2_msg( oyMSG_WARN, (oyStruct_s*)node, OY_DBG_FORMAT_"\n"
              "  output profile: \"%s\" %s %s->%s %s\n  %s",
              OY_DBG_ARGS_,
              oyProfile_GetText( image_input_profile, oyNAME_DESCRIPTION ),
              oyProfile_GetText( image_output_profile, oyNAME_NAME ),
              oyICCColorSpaceGetName( oyProfile_GetSignature( 
                            image_input_profile, oySIGNATURE_COLOR_SPACE ) ),
              oyICCColorSpaceGetName( oyProfile_GetSignature( 
                            image_input_profile, oySIGNATURE_PCS ) ),
              oyICCDeviceClassDescription( oyProfile_GetSignature( 
                            image_input_profile, oySIGNATURE_CLASS ) ),
              oyPixelPrint(oyImage_GetPixelLayout( image_output, oyLAYOUT ), malloc));
  }

  /* additional tags for debugging */
  if(!error && (oy_debug || verbose))
  {
    if(!error && size)
    {
      size_ = *size;

      if(!size_)
        return NULL;

      prof = oyProfile_FromMem( size_, block, 0, 0 );

      /* icSigProfileSequenceIdentifierType */
      {
        oyStructList_s * list = oyStructList_New(0);
        int i, n = oyProfiles_Count( profs );
        for( i = 0; i < n ; ++i )
        {
          oyProfile_s * p = oyProfiles_Get( profs, i );
          oyStructList_MoveIn( list, (oyStruct_s**) &p, -1, 0 );
        }
        psid = oyProfileTag_Create( list, icSigProfileSequenceIdentifierTag,
                                 icSigProfileSequenceIdentifierType, 0, 0 );
        if(psid)
          error = oyProfile_TagMoveIn ( prof, &psid, -1 );

        oyStructList_Release( &list );
      }

      /* Info tag */
      if(!error)
      {
        oyStructList_s * list = 0;
        char h[5] = {"Info"};
        uint32_t * hi = (uint32_t*)&h;
        char * cc_name = lcm2FilterNode_GetText( node, oyNAME_NICK,
                                                 oyAllocateFunc_ );
        oyName_s * name = oyName_new(0);
        const char * lib_name = oyFilterNode_GetModuleName( node );

        name = oyName_set_ ( name, cc_name, oyNAME_NAME,
                             oyAllocateFunc_, oyDeAllocateFunc_ );
        name = oyName_set_ ( name, lib_name, oyNAME_NICK,
                             oyAllocateFunc_, oyDeAllocateFunc_ );
        oyDeAllocateFunc_( cc_name );
        list = oyStructList_New(0);
        error = oyStructList_MoveIn( list,  (oyStruct_s**) &name, -1, 0 );

        if(!error)
        {
          info = oyProfileTag_Create( list, (icTagSignature)oyValueUInt32(*hi),
                                      icSigTextType, 0, 0);
          error = !info;
        }

        oyStructList_Release( &list );

        if(info)
          error = oyProfile_TagMoveIn ( prof, &info, -1 );
      }

      if(!error)
        cprt = oyProfile_GetTagById( prof, icSigCopyrightTag );

      /* icSigCopyrightTag */
      if(!error && !cprt)
      {
        oyStructList_s * list = 0;
        const char * c_text = "no copyright; use freely";
        oyName_s * name = oyName_new(0);

        name = oyName_set_ ( name, c_text, oyNAME_NAME,
                             oyAllocateFunc_, oyDeAllocateFunc_ );
        list = oyStructList_New(0);
        error = oyStructList_MoveIn( list, (oyStruct_s**) &name, -1, 0 );

        if(!error)
        {
          cprt = oyProfileTag_Create( list, icSigCopyrightTag,
                                      icSigTextType, 0, 0);
          error = !cprt;
        }

        oyStructList_Release( &list );

        if(cprt)
          error = oyProfile_TagMoveIn ( prof, &cprt, -1 );
      }

      if(block)
        oyDeAllocateFunc_( block ); block = 0; size_ = 0;

      block = oyProfile_GetMem( prof, &size_, 0, allocateFunc );

      *size = size_;
      oyProfile_Release( &prof );
    }
  }

  oyFilterPlug_Release( &plug );
  oyFilterSocket_Release( &socket );
  oyFilterSocket_Release( & remote_socket );
  oyOptions_Release( &node_tags );
  oyImage_Release( &image_input );
  oyImage_Release( &image_output );
  oyProfile_Release( &image_input_profile );
  oyProfile_Release( &image_output_profile );
  oyOptions_Release( &node_options );
  oyProfiles_Release( &profs );
  oyProfiles_Release( &profiles );

  return block;
}

char * lcm2Image_GetText             ( oyImage_s         * image,
                                       int                 verbose,
                                       oyAlloc_f           allocateFunc )
{
  oyPixel_t pixel_layout = oyImage_GetPixelLayout(image,oyLAYOUT); 
  int n     = oyToChannels_m( pixel_layout );
  oyProfile_s * profile = oyImage_GetProfile( image );
  int cchan_n = oyProfile_GetChannelsCount( profile );
  int coff_x = oyToColorOffset_m( pixel_layout );
  oyDATATYPE_e t = oyToDataType_m( pixel_layout );
  int swap  = oyToSwapColorChannels_m( pixel_layout );
  /*int revert= oyT_FLAVOR_M( pixel_layout );*/
  int so = oyDataTypeGetSize( t );
  char * text = oyAllocateFunc_(512);
  char * hash_text = 0;
  oyImage_s * s = image;

  /* describe the image */
  oySprintf_( text,   "  <oyImage_s>\n");
  hashTextAdd_m( text );
  oySprintf_( text, "    %s\n", oyProfile_GetText(profile, oyNAME_NAME));
  hashTextAdd_m( text );
  oySprintf_( text,   "    <channels all=\"%d\" color=\"%d\" />\n", n,cchan_n);
  hashTextAdd_m( text );
  oySprintf_( text,
                      "    <offsets first_color_sample=\"%d\" next_pixel=\"%d\" />\n"
              /*"  next line = %d\n"*/,
              coff_x, oyImage_GetPixelLayout( s,oyPOFF_X )/*, mask[oyPOFF_Y]*/ );
  hashTextAdd_m( text );

  if(swap || oyToByteswap_m( pixel_layout ))
  {
    hashTextAdd_m(    "    <swap" );
    if(swap)
      hashTextAdd_m(  " colorswap=\"yes\"" );
    if( oyToByteswap_m( pixel_layout ) )
      hashTextAdd_m(  " byteswap=\"yes\"" );
    hashTextAdd_m(    " />\n" );
  }

  if( oyToFlavor_m( pixel_layout ) )
  {
    oySprintf_( text, "    <flawor value=\"yes\" />\n" );
    hashTextAdd_m( text );
  }
  oySprintf_( text,   "    <sample_type value=\"%s[%dByte]\" />\n",
                    oyDataTypeToText(t), so );
  hashTextAdd_m( text );
  oySprintf_( text,   "  </oyImage_s>");
  hashTextAdd_m( text );

  if(allocateFunc == oyAllocateFunc_)
    oyDeAllocateFunc_(text);
  else
  {
    oyDeAllocateFunc_(text);
    text = hash_text;
    hash_text = oyStringCopy_( text, allocateFunc );
    oyDeAllocateFunc_( text );
  }
  text = 0;

  return hash_text;
}

/** Function lcm2FilterNode_GetText
 *  @brief   implement oyCMMFilterNode_GetText_f()
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/27 (Oyranos: 0.1.10)
 *  @date    2009/06/02
 */
char * lcm2FilterNode_GetText        ( oyFilterNode_s    * node,
                                       oyNAME_e            type,
                                       oyAlloc_f           allocateFunc )
{
#ifdef NO_OPT
  return oyStringCopy_( oyFilterNode_GetText( node, type ), allocateFunc );
#else
  const char * tmp = 0,
             * model = 0;
  char * hash_text = 0,
       * temp = 0;
  oyFilterNode_s * s = node;

  oyImage_s * in_image = 0,
            * out_image = 0;
  int verbose;
  oyOptions_s * node_opts = oyFilterNode_GetOptions( node, 0 );
  oyOptions_s * node_tags = oyFilterNode_GetTags( node ),
              * opts_tmp, * opts_tmp2, * options;
  oyFilterCore_s * node_core = oyFilterNode_GetCore( node );
  oyFilterPlug_s * plug = oyFilterNode_GetPlug( node, 0 );
  oyFilterSocket_s * socket = oyFilterNode_GetSocket( node, 0 ),
                   * remote_socket = oyFilterPlug_GetSocket( plug );
  oyProfiles_s * profiles;
  oyProfile_s * p;
  int effect_switch, proof, i,n;

  /* pick all sockets (output) data */
  out_image = (oyImage_s*)oyFilterSocket_GetData( remote_socket );
  /* pick all plug (input) data */
  in_image = (oyImage_s*)oyFilterSocket_GetData( socket );

  if(!node)
    return 0;

  verbose = oyOptions_FindString( node_tags, "verbose", "true" ) ? 1 : 0;

  /* 1. create hash text */
  hashTextAdd_m( "<oyFilterNode_s>\n  " );

  /* the filter text */
  hashTextAdd_m( oyFilterCore_GetText( node_core, oyNAME_NAME ) );

  /* make a description */
  {
    /* input data */
    hashTextAdd_m(   " <data_in>\n" );
    if(in_image)
    {
      temp = lcm2Image_GetText( in_image, verbose, oyAllocateFunc_ );
      hashTextAdd_m( temp );
      oyDeAllocateFunc_(temp); temp = 0;
    }
    hashTextAdd_m( "\n </data_in>\n" );

    /* pick inbuild defaults */
    opts_tmp2 = oyOptions_FromText( lcm2_extra_options, 0, NULL );
    opts_tmp = oyOptions_ForFilter( "//" OY_TYPE_STD "/icc_color",
                                oyOPTIONSOURCE_FILTER | OY_SELECT_COMMON , 0 );
    options = oyOptions_FromBoolean( opts_tmp, opts_tmp2, oyBOOLEAN_UNION,NULL);
    oyOptions_Release( &opts_tmp );
    oyOptions_Release( &opts_tmp2 );
    opts_tmp = options;
    /* add existing custom options */
    options = oyOptions_FromBoolean( opts_tmp, node_opts, oyBOOLEAN_UNION,NULL);
    oyOptions_Release( &opts_tmp );

    /* options -> xforms */
    hashTextAdd_m(   " <oyOptions_s>\n" );
    model = oyOptions_GetText( options, oyNAME_NAME );
    hashTextAdd_m( model );
    hashTextAdd_m( "\n </oyOptions_s>\n" );
    oyOptions_Release( &options );

    /* abstract profiles */
    proof = oyOptions_FindString  ( node_opts, "proof_soft", "1" ) ? 1 : 0;
    proof += oyOptions_FindString  ( node_opts, "proof_hard", "1" ) ? 1 : 0;
    effect_switch = oyOptions_FindString  ( node_opts, "effect_switch", "1" ) ? 1 : 0;
    if(proof || effect_switch)
    hashTextAdd_m(   " <oyProfiles_s>" );
    profiles = lcm2ProfilesFromOptions( node, plug, node_opts, "profiles_effect", effect_switch, verbose );
    n = oyProfiles_Count( profiles );
    if(n)
    {
      for(i = 0; i < n; ++i)
      {
        p = oyProfiles_Get( profiles, i );
        model = oyProfile_GetText( p, oyNAME_NAME );
    hashTextAdd_m( "\n  " );
    hashTextAdd_m( model );
        oyProfile_Release( &p );
      }
    }
    oyProfiles_Release( &profiles );
    if(proof || effect_switch)
    hashTextAdd_m( "\n </oyProfiles_s>\n" );

    /* output data */
    hashTextAdd_m(   " <data_out>\n" );
    if(out_image)
    {
      temp = lcm2Image_GetText( out_image, verbose, oyAllocateFunc_ );
      hashTextAdd_m( temp );
      oyDeAllocateFunc_(temp); temp = 0;
    }
    hashTextAdd_m( "\n </data_out>\n" );
  }
  hashTextAdd_m( tmp );

  hashTextAdd_m(   "</oyFilterNode_s>\n" );

  oyOptions_Release( &node_opts );
  oyOptions_Release( &node_tags );
  oyFilterCore_Release( &node_core );
  oyFilterPlug_Release( &plug );
  oyFilterSocket_Release( &socket );
  oyFilterSocket_Release( &remote_socket );
  oyImage_Release( &in_image );
  oyImage_Release( &out_image );

  return oyStringCopy_( hash_text, allocateFunc );
#endif
}

/** Function lcm2FlagsToText
 *  @brief   debugging helper
 *
 *  @version Oyranos: 0.1.13
 *  @since   2010/11/28 (Oyranos: 0.1.13)
 *  @date    2010/11/28
 */
char * lcm2FlagsToText               ( int                 flags )
{
  char * t = 0;
  char num[24];

  sprintf(num, "%d", flags);
  STRING_ADD( t, "flags[" );
  STRING_ADD( t, num );
  STRING_ADD( t, "]: " );
#define STRING_ADD_FLAG( flag_name ) \
  if(flags & flag_name) \
    STRING_ADD( t, " " #flag_name );
  STRING_ADD_FLAG( cmsFLAGS_NOCACHE );
  STRING_ADD_FLAG( cmsFLAGS_NOOPTIMIZE );
  STRING_ADD_FLAG( cmsFLAGS_NULLTRANSFORM );
  STRING_ADD_FLAG( cmsFLAGS_GAMUTCHECK );
  STRING_ADD_FLAG( cmsFLAGS_SOFTPROOFING );
  STRING_ADD_FLAG( cmsFLAGS_BLACKPOINTCOMPENSATION );
  STRING_ADD_FLAG( cmsFLAGS_NOWHITEONWHITEFIXUP );
  STRING_ADD_FLAG( cmsFLAGS_HIGHRESPRECALC );
  STRING_ADD_FLAG( cmsFLAGS_LOWRESPRECALC );
  STRING_ADD_FLAG( cmsFLAGS_8BITS_DEVICELINK );
  STRING_ADD_FLAG( cmsFLAGS_GUESSDEVICECLASS );
  STRING_ADD_FLAG( cmsFLAGS_KEEP_SEQUENCE );
  STRING_ADD_FLAG( cmsFLAGS_FORCE_CLUT );
  STRING_ADD_FLAG( cmsFLAGS_CLUT_POST_LINEARIZATION );
  STRING_ADD_FLAG( cmsFLAGS_CLUT_PRE_LINEARIZATION );
  return t;
}

/** Function lcm2ModuleData_Convert
 *  @brief   convert between data formats
 *  @ingroup cmm_handling
 *
 *  The function might be used to provide a module specific context.
 *  Implements oyModuleData_Convert_f
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/28 (Oyranos: 0.1.10)
 *  @date    2008/12/28
 */
int  lcm2ModuleData_Convert          ( oyPointer_s       * data_in,
                                       oyPointer_s       * data_out,
                                       oyFilterNode_s    * node )
{
  int error = !data_in || !data_out;
  oyPointer_s * cmm_ptr_in = data_in,
             * cmm_ptr_out = data_out;
  lcm2TransformWrap_s * ltw  = 0;
  cmsHTRANSFORM xform = 0;
  cmsHPROFILE lps[2] = {0,0};
  oyFilterPlug_s * plug = oyFilterNode_GetPlug( node, 0 );
  oyFilterSocket_s * socket = oyFilterNode_GetSocket( node, 0 ),
                   * remote_socket = oyFilterPlug_GetSocket( plug );
  oyOptions_s * node_options = oyFilterNode_GetOptions( node, 0 );
  oyImage_s * image_input = (oyImage_s*)oyFilterSocket_GetData( remote_socket ),
            * image_output = (oyImage_s*)oyFilterSocket_GetData( socket );

  if(!error)
  {
    cmm_ptr_in = (oyPointer_s*) data_in;
    cmm_ptr_out = (oyPointer_s*) data_out;
  }

  if(!error &&
     ( (strcmp( oyPointer_GetResourceName(cmm_ptr_in), oyCOLOR_ICC_DEVICE_LINK ) != 0) ||
       (strcmp( oyPointer_GetResourceName(cmm_ptr_out), lcm2TRANSFORM ) != 0) ) )
    error = 1;

  if(!error)
  {
#if LCMS_VERSION < 2060
    lps[0] = CMMProfileOpen_M( node, oyPointer_GetPointer(cmm_ptr_in),
                               oyPointer_GetSize( cmm_ptr_in) );
#else
    {
      oyFilterNode_s * node2 = oyFilterNode_Copy( node, NULL );
      cmsContext tc = lcmsCreateContext( NULL, node2 ); /* threading context */
      lcmsSetLogErrorHandlerTHR( tc, lcm2ErrorHandlerFunction );
      lps[0] = CMMProfileOpen_M( tc, oyPointer_GetPointer(cmm_ptr_in),
                                 oyPointer_GetSize( cmm_ptr_in) );
    }
#endif
    xform = lcm2CMMConversionContextCreate_( node, lps, 1, 0,0,0,
                                oyImage_GetPixelLayout( image_input, oyLAYOUT ),
                                oyImage_GetPixelLayout( image_output,oyLAYOUT ),
                                           node_options,
                                           &ltw, cmm_ptr_out );

    if(oy_debug > 4)
    {
      oyProfile_s *p = oyProfile_FromMem( oyPointer_GetSize( cmm_ptr_in),
                                          oyPointer_GetPointer(cmm_ptr_in),0,0);
      uint32_t id[8]={0,0,0,0,0,0,0,0};
      char * hash_text = oyStringCopy_( lcm2TRANSFORM":", oyAllocateFunc_ );

      char * t = lcm2FilterNode_GetText( node, oyNAME_NICK, oyAllocateFunc_ );
      STRING_ADD( hash_text, t );
      oyFree_m_(t);

      oyMiscBlobGetHash_((void*)hash_text, oyStrlen_(hash_text), 0,
                         (unsigned char*)id);
      oyStringAddPrintf_( &t, oyAllocateFunc_, oyDeAllocateFunc_,
                          "node: %d hash: %08x%08x%08x%08x",
                          oyStruct_GetId((oyStruct_s*)node),
                          id[0],id[1],id[2],id[3] );

      oyProfile_GetMD5( p, OY_COMPUTE, id );
      oyStringAddPrintf_( &t, oyAllocateFunc_, oyDeAllocateFunc_,
                          " oyDL: %08x%08x%08x%08x", id[0],id[1],id[2],id[3] );
      
      if(oy_debug >= 1)
      lcm2_msg( oyMSG_DBG,(oyStruct_s*) node, OY_DBG_FORMAT_"oyDL: %08x%08x%08x%08x %s %s",
                OY_DBG_ARGS_, id[0],id[1],id[2],id[3], t, hash_text );

      oyPointer_SetId( cmm_ptr_out, t );

      oyProfile_Release( &p );
      oyFree_m_(t);
    }

    if(!xform)
    {
      uint32_t f = oyImage_GetPixelLayout( image_input, oyLAYOUT );
      lcm2_msg( oyMSG_WARN,(oyStruct_s*) node, OY_DBG_FORMAT_
      "float:%d optimised:%d colorspace:%d extra:%d channels:%d lcms_bytes%d",
      OY_DBG_ARGS_,
      T_FLOAT(f), T_OPTIMIZED(f), T_COLORSPACE(f), T_EXTRA(f), T_CHANNELS(f), T_BYTES(f) );
      error = 1;
    }
    {
#if LCMS_VERSION >= 2060
      oyFilterNode_s * node = lcmsGetContextUserData( lcmsGetProfileContextID( lps[0] ) );
      oyFilterNode_Release( &node );
#endif
      CMMProfileRelease_M (lps[0] );
    }
  }
  oyFilterPlug_Release( &plug );
  oyFilterSocket_Release( &socket );
  oyFilterSocket_Release( & remote_socket );
  oyImage_Release( &image_input );
  oyImage_Release( &image_output );
  oyOptions_Release( &node_options );

  return error;
}

char * oyCMMCacheListPrint_();

/** Function lcm2FilterPlug_CmmIccRun
 *  @brief   implement oyCMMFilterPlug_GetNext_f()
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/07/18 (Oyranos: 0.1.8)
 *  @date    2011/06/17
 */
int      lcm2FilterPlug_CmmIccRun    ( oyFilterPlug_s    * requestor_plug,
                                       oyPixelAccess_s   * ticket )
{
  int j, k, n;
  int error = 0;
  oyDATATYPE_e data_type_in = 0,
               data_type_out = 0;
  int channels_out, channels_in;
  int bps_in;
  oyPixel_t pixel_layout_in,
            layout_out;

  oyFilterSocket_s * socket = oyFilterPlug_GetSocket( requestor_plug );
  oyFilterPlug_s * plug = 0;
  oyFilterNode_s * input_node,
                 * node = oyFilterSocket_GetNode( socket );
  oyImage_s * image_input = 0, * image_output = 0;
  oyArray2d_s * array_in = 0, * array_out = 0;
  lcm2TransformWrap_s * ltw  = 0;
  oyPixelAccess_s * new_ticket = ticket;

  plug = oyFilterNode_GetPlug( node, 0 );
  input_node = oyFilterNode_GetPlugNode( node, 0 );

  image_input = oyFilterPlug_ResolveImage( plug, socket, ticket );
  pixel_layout_in = oyImage_GetPixelLayout( image_input, oyLAYOUT );
  channels_in = oyToChannels_m( pixel_layout_in );

  image_output = oyPixelAccess_GetOutputImage( ticket );
  layout_out = oyImage_GetPixelLayout( image_output, oyLAYOUT );
  channels_out = oyToChannels_m( layout_out );

  if(oyImage_GetPixelLayout( image_input, oyLAYOUT ) != 
     oyImage_GetPixelLayout( image_output, oyLAYOUT ))
  {
    /* create a new ticket to avoid pixel layout conflicts */
    /* keep old ticket array dimensions */
    oyArray2d_s * a,
                * old_a = oyPixelAccess_GetArray( new_ticket );
    new_ticket = oyPixelAccess_Copy( ticket, ticket->oy_ );
    oyPixelAccess_SetOutputImage( new_ticket, image_input );

    /* remove old array as it's layout does not fit */
    oyPixelAccess_SetArray( new_ticket, 0, 0 );
    /* should be empty */
    a = oyPixelAccess_GetArray( new_ticket );
    if(!a)
    {
      /* Use original pixel size for being save and do not fiddle with ROI's */
      int w = oyArray2d_GetDataGeo1( old_a, 2 ) / channels_out;
      int h = oyArray2d_GetDataGeo1( old_a, 3 );
      a = oyArray2d_Create( NULL, w * channels_in,h, oyToDataType_m( pixel_layout_in ), ticket->oy_ );
      if(oy_debug)
      {
        lcm2_msg( oy_debug?oyMSG_WARN:oyMSG_DBG, (oyStruct_s*)ticket, OY_DBG_FORMAT_"layout_out(%d) != layout_in(%d) created %s",
                OY_DBG_ARGS_, layout_out, pixel_layout_in,
                oyArray2d_Show( a, channels_in ));
      }
    }
    oyArray2d_Release( &old_a );
    oyPixelAccess_SetArray( new_ticket, a, 0 );
    oyArray2d_Release( &a );

    oyPixelAccess_SynchroniseROI( new_ticket, ticket );

    if(oy_debug)
      lcm2_msg( oy_debug?oyMSG_WARN:oyMSG_DBG, (oyStruct_s*)ticket, OY_DBG_FORMAT_"new_ticket %s",
                OY_DBG_ARGS_,
                oyPixelAccess_Show( new_ticket ));
  }

  /* We let the input filter do its processing first. */
  error = oyFilterNode_Run( input_node, plug, new_ticket );
  if(error != 0)
  {
    lcm2_msg( oyMSG_ERROR, (oyStruct_s*)input_node, OY_DBG_FORMAT_"%s %d err:%d",
                OY_DBG_ARGS_, _("running new ticket failed"),
                oyStruct_GetId( (oyStruct_s*)new_ticket ), error );
    return error;
  }

  array_in = oyPixelAccess_GetArray( new_ticket );
  array_out = oyPixelAccess_GetArray( ticket );
  if(oy_debug > 2)
    lcm2_msg( oyMSG_DBG, (oyStruct_s*)new_ticket, OY_DBG_FORMAT_"%s %cnew_ticket->array:%s %s[%d]",
              OY_DBG_ARGS_,_("Read from"), oyPixelAccess_ArrayIsFocussed( new_ticket )?' ':'~',
              oyArray2d_Show( array_in, channels_in ),
              _("Image"), oyStruct_GetId( (oyStruct_s*)image_input ) );
  if(oy_debug > 2)
    lcm2_msg( oyMSG_DBG, (oyStruct_s*)ticket, OY_DBG_FORMAT_"%s %cticket->array:%s %s[%d]",
              OY_DBG_ARGS_,_("Write to"), oyPixelAccess_ArrayIsFocussed( ticket )?' ':'~',
              oyArray2d_Show( array_out, channels_out ),
              _("Image"), oyStruct_GetId( (oyStruct_s*)image_output ) );

  data_type_in = oyToDataType_m( oyImage_GetPixelLayout( image_input, oyLAYOUT ) );
  bps_in = oyDataTypeGetSize( data_type_in );

  /*if(data_type_in == oyHALF)
  {
    oyFilterSocket_Callback( requestor_plug, oyCONNECTOR_EVENT_INCOMPATIBLE_DATA );
    lcm2_msg(oyMSG_WARN,(oyStruct_s*)ticket, OY_DBG_FORMAT_" can not handle oyHALF",OY_DBG_ARGS_);
    error = 1;
  }*/

  if(!image_output)
  {
    lcm2_msg( oyMSG_WARN,(oyStruct_s*)ticket, OY_DBG_FORMAT_ " no ticket->output_image",
             OY_DBG_ARGS_);
    error = 1;
  }

  if(!error)
  {
    oyPointer_s * backend_data = oyFilterNode_GetContext( node );
    data_type_out = oyToDataType_m( oyImage_GetPixelLayout( image_output, oyLAYOUT ) );

    /* get transform */
    error = lcm2CMMTransform_GetWrap_( backend_data, &ltw );
    if(oy_debug >= 2 && ltw)
      lcm2_msg( oyMSG_DBG, NULL, OY_DBG_FORMAT_
             " xform: "OY_PRINT_POINTER
             " ltw: "OY_PRINT_POINTER
             " backend_data: %d",
             OY_DBG_ARGS_, ltw->lcm2, ltw, oyStruct_GetId((oyStruct_s*)backend_data) );

    if(oy_debug > 4)
    /* verify context */
    {
      int msg_type = oyMSG_DBG;
      uint32_t id[8]={0,0,0,0,0,0,0,0};
      char * hash_text = oyStringCopy_( lcm2TRANSFORM":", oyAllocateFunc_ );

      char * t = 0;
      t = lcm2FilterNode_GetText( node, oyNAME_NICK, oyAllocateFunc_ );
      STRING_ADD( hash_text, t );
      oyFree_m_(t);

      oyMiscBlobGetHash_((void*)hash_text, oyStrlen_(hash_text), 0,
                         (unsigned char*)id);
      oyStringAddPrintf_( &t, oyAllocateFunc_, oyDeAllocateFunc_,
                          "hash: %08x%08x%08x%08x",
                          id[0],id[1],id[2],id[3] );

      /* check if we obtained the context from our
       * lcm2_api4_cmm::lcm2FilterNode_CmmIccContextToMem */
      if(oyPointer_GetFuncName( backend_data ) &&
         strstr(oyPointer_GetLibName( backend_data ),CMM_NICK) &&
         /* check if context and actual options match */
         oyPointer_GetId( backend_data ) &&
         !strstr(oyPointer_GetId( backend_data ),t))
      {
        /* send error message */
        error = 1;
        msg_type = oyMSG_ERROR;

        lcm2_msg( msg_type, (oyStruct_s*)ticket, OY_DBG_FORMAT_
                  "requested and actual contexts differ by hash",OY_DBG_ARGS_ );
      }

      if(error || oy_debug > 4)
        lcm2_msg( msg_type, (oyStruct_s*)ticket, OY_DBG_FORMAT_
                  "node: %d \"%s\" (context %s)\nwant: %s\n%s", OY_DBG_ARGS_,
                  oyStruct_GetId((oyStruct_s*)node), t,
                  oyNoEmptyString_m_(oyPointer_GetId( backend_data )),
                  oy_debug > 0 && error > 0 ? hash_text : "----",
                  oy_debug > 0 && error > 0 ? oyCMMCacheListPrint_() : "" );
      if(oy_debug > 4 && error < 1)
        lcm2_msg( msg_type, (oyStruct_s*)ticket, OY_DBG_FORMAT_
                  "%s", OY_DBG_ARGS_, hash_text );

      oyFree_m_(hash_text);
      oyFree_m_(t);
    }
    oyPointer_Release( &backend_data );
  }

  DBGs_PROG4_S( ticket, "channels in/out: %d[%d]->%d[%d]",
                channels_in, oyStruct_GetId((oyStruct_s*)image_input),
                channels_out, oyStruct_GetId((oyStruct_s*)image_output) );

  if(ltw && !array_out)
  {
    lcm2_msg( oyMSG_ERROR,(oyStruct_s*)ticket, OY_DBG_FORMAT_ " no ticket->array",
             OY_DBG_ARGS_);
    error = 1;
  }

  /* now do some position blind manipulations */
  if(ltw && error <= 0)
  {
    uint8_t * array_in_tmp = 0,
            * array_out_tmp = 0;
    float * array_in_tmp_flt = 0,
          * array_out_tmp_flt = 0;
    double * array_in_tmp_dbl = 0,
           * array_out_tmp_dbl = 0;
    uint8_t ** array_in_data = oyArray2d_GetData( array_in ),
            ** array_out_data = oyArray2d_GetData( array_out );
    int threads_n = 
#if defined(_OPENMP) && defined(USE_OPENMP)
                    omp_get_max_threads();
#else
                    1;
#endif
    int w_in =  (int)(oyArray2d_GetWidth(array_in)+0.5),
        w_out = (int)(oyArray2d_GetWidth(array_out)+0.5);
    int stride_in = w_in * bps_in;

    n = OY_MIN(w_in/channels_in, w_out/channels_out);

    if(oy_debug)
      lcm2_msg( oyMSG_DBG,(oyStruct_s*)ticket, OY_DBG_FORMAT_
             " %s[%d]=\"%s\" threads_n: %d %s "OY_PRINT_POINTER
             " -> %s "OY_PRINT_POINTER" convert pixel: %d",
             OY_DBG_ARGS_,
             _("Node"),oyStruct_GetId((oyStruct_s*)node),oyStruct_GetInfo((oyStruct_s*)node,0,0),
             threads_n,
             oyArray2d_Show(array_in,channels_in),array_in_data,
             oyArray2d_Show(array_out,channels_out),array_out_data,n );

    if(!(data_type_in == oyUINT8 ||
         data_type_in == oyUINT16 ||
         data_type_in == oyHALF ||
         data_type_in == oyFLOAT ||
         data_type_in == oyDOUBLE))
    {
      oyFilterSocket_Callback( requestor_plug, oyCONNECTOR_EVENT_INCOMPATIBLE_DATA );
      error = 1;
    }
    
    if(ltw->sig_in  == icSigXYZData &&
       (data_type_in == oyFLOAT ||
        data_type_in == oyDOUBLE))
    {
      array_in_tmp = oyAllocateFunc_( stride_in * threads_n );
      if(data_type_in == oyFLOAT)
        array_in_tmp_flt = (float*) array_in_tmp;
      else if(data_type_in == oyDOUBLE)
        array_in_tmp_dbl = (double*) array_in_tmp;
    }
    if(ltw->sig_out  == icSigXYZData &&
       (data_type_out == oyFLOAT ||
        data_type_out == oyDOUBLE))
    {
      array_out_tmp = array_out_data[0];
    }
    

    /*  - - - - - conversion - - - - - */
    /*lcm2_msg(oyMSG_WARN,(oyStruct_s*)ticket, "%s: %d Start lines: %d",
            __FILE__,__LINE__, array_out->height);*/
    if(!error)
    {
      const double xyz_factor = 1.0 + 32767.0/32768.0;
      const int use_xyz_scale = 1;
      int index = 0;
      int array_in_height = oyArray2d_GetHeight(array_in),
          array_out_height = oyArray2d_GetHeight(array_out),
          lines = OY_MIN(array_in_height, array_out_height);
      if(lines > threads_n * 10)
      {
#if defined(USE_OPENMP)
#pragma omp parallel for private(index,j,array_in_tmp_flt,array_in_tmp_dbl,array_out_tmp_flt,array_out_tmp_dbl)
#endif
        for( k = 0; k < lines; ++k)
        {
          if(array_in_tmp && use_xyz_scale)
          {
#if defined(_OPENMP) && defined(USE_OPENMP)
            index = omp_get_thread_num();
#endif
            memcpy( &array_in_tmp[stride_in*index], array_in_data[k],
                    w_in * bps_in );
            if(data_type_in == oyFLOAT)
            {
              array_in_tmp_flt = (float*) &array_in_tmp[stride_in*index];
              for(j = 0; j < w_in; ++j)
              {
                array_in_tmp_flt[j] /= xyz_factor;
              }
            } else
            if(data_type_in == oyDOUBLE)
            {
              array_in_tmp_dbl = (double*) &array_in_tmp[stride_in*index];
              for(j = 0; j < w_in; ++j)
              {
                array_in_tmp_dbl[j] /= xyz_factor;
              }
            }
            lcmsDoTransform( ltw->lcm2, &array_in_tmp[stride_in*index],
                                       array_out_data[k], n );
          } else
            lcmsDoTransform( ltw->lcm2, array_in_data[k],
                                       array_out_data[k], n );
          if(array_out_tmp && use_xyz_scale)
          {
            if(data_type_out == oyFLOAT)
            {
              array_out_tmp_flt = (float*) array_out_data[k];
              for(j = 0; j < w_out; ++j)
                array_out_tmp_flt[j] *= xyz_factor;
            } else
            if(data_type_out == oyDOUBLE)
            {
              array_out_tmp_dbl = (double*) array_out_data[k];
              for(j = 0; j < w_out; ++j)
                array_out_tmp_dbl[j] *= xyz_factor;
            }
          }
        }
      } else
        for( k = 0; k < lines; ++k)
        {
          if(array_in_tmp && use_xyz_scale)
          {
            memcpy( array_in_tmp, array_in_data[k], w_in * bps_in );
            if(data_type_in == oyFLOAT)
            for(j = 0; j < w_in; ++j)
            {
              array_in_tmp_flt[j] /= xyz_factor;
            }
            if(data_type_in == oyDOUBLE)
            for(j = 0; j < w_in; ++j)
            {
              array_in_tmp_dbl[j] /= xyz_factor;
            }
            lcmsDoTransform( ltw->lcm2, array_in_tmp,
                                       array_out_data[k], n );
          } else
            lcmsDoTransform( ltw->lcm2, array_in_data[k],
                                       array_out_data[k], n );
          if(array_out_tmp && use_xyz_scale)
          {
            if(data_type_out == oyFLOAT)
            {
              array_out_tmp_flt = (float*) array_out_data[k];
              for(j = 0; j < w_out; ++j)
                array_out_tmp_flt[j] *= xyz_factor;
            } else
            if(data_type_out == oyDOUBLE)
            {
              array_out_tmp_dbl = (double*) array_out_data[k];
              for(j = 0; j < w_out; ++j)
                array_out_tmp_dbl[j] *= xyz_factor;
            }
          }
        }
    /*lcm2_msg(oyMSG_WARN,(oyStruct_s*)ticket, "%s: %d End width: %d",
            __FILE__,__LINE__, n);*/
    }

    if(array_in_tmp)
      oyDeAllocateFunc_( array_in_tmp );

    if(getenv("OY_DEBUG_WRITE"))
    {
      char * t = 0; oyStringAddPrintf( &t, 0,0,
      "%04d-%s-array_in[%d].ppm", ++oy_debug_write_id,CMM_NICK,oyStruct_GetId((oyStruct_s*)array_in));
      oyArray2d_ToPPM_( (oyArray2d_s_*)array_in, t );
      lcm2_msg( oyMSG_DBG, (oyStruct_s*)ticket,
                 OY_DBG_FORMAT_ "wrote debug image to: %s",
                 OY_DBG_ARGS_, t );
      t[0] = '\000'; oyStringAddPrintf( &t, 0,0,
      "%04d-%s-array_out[%d].ppm", oy_debug_write_id,CMM_NICK,oyStruct_GetId((oyStruct_s*)array_out));
      oyArray2d_ToPPM_( (oyArray2d_s_*)array_out, t );
      lcm2_msg( oyMSG_DBG, (oyStruct_s*)ticket,
                 OY_DBG_FORMAT_ "wrote debug image to: %s",
                 OY_DBG_ARGS_, t );
      t[0] = '\000'; oyStringAddPrintf( &t, 0,0,
      "%04d-%s-node[%d]-array_out[%d]%dc.ppm", oy_debug_write_id,CMM_NICK,oyStruct_GetId((oyStruct_s*)node),oyStruct_GetId((oyStruct_s*)array_out),channels_out);
      {
        oyProfile_s * p = oyImage_GetProfile( image_output );
        oyImage_s * img = oyImage_Create( oyArray2d_GetWidth(array_out)/channels_out, oyArray2d_GetHeight(array_out),NULL,
                                          oyImage_GetPixelLayout( image_output, oyLAYOUT ), p, NULL );
        oyImage_ReadArray( img, NULL, array_out, NULL );
        oyImage_WritePPM( img, t, t );
        oyProfile_Release( &p );
        oyImage_Release( &img );
      }
      lcm2_msg( oyMSG_DBG, (oyStruct_s*)ticket,
                 OY_DBG_FORMAT_ "wrote debug image to: %s",
                 OY_DBG_ARGS_, t );
      oyFree_m_(t);
    }
  } else
  {
    oyFilterGraph_s * ticket_graph = oyPixelAccess_GetGraph( ticket );
    oyOptions_s * ticket_graph_opts = 
                                       oyFilterGraph_GetOptions( ticket_graph );
    if(error)
      oyFilterSocket_Callback( requestor_plug,
                               oyCONNECTOR_EVENT_INCOMPATIBLE_CONTEXT );
    else
      oyFilterSocket_Callback( requestor_plug,
                               oyCONNECTOR_EVENT_OK );

    error = oyOptions_SetFromText( &ticket_graph_opts,
                     "//" OY_TYPE_STD "/profile/dirty", "true", OY_CREATE_NEW );
    oyFilterGraph_Release( &ticket_graph );
    oyOptions_Release( &ticket_graph_opts );
    error = 1;
  }

  if(oyImage_GetPixelLayout( image_input, oyLAYOUT ) != 
     oyImage_GetPixelLayout( image_output, oyLAYOUT ))
    oyPixelAccess_Release( &new_ticket );

  oyFilterPlug_Release( &plug );
  oyFilterSocket_Release( &socket );
  oyFilterNode_Release( &input_node );
  oyFilterNode_Release( &node );
  oyImage_Release( &image_input );
  oyImage_Release( &image_output );
  oyArray2d_Release( &array_in );
  oyArray2d_Release( &array_out );

  return error;
}




/*
oyPointer          oyCMMallocateFunc   ( size_t            size )
{
  oyPointer p = 0;
  if(size)
    p = malloc(size);
  return p;
}

void               oyCMMdeallocateFunc ( oyPointer         mem )
{
  if(mem)
    free(mem);
}*/

/** Function lcm2ErrorHandlerFunction
 *  @brief
 *
 *  @version Oyranos: 0.1.8
 *  @date    2007/11/00
 *  @since   2007/11/00 (Oyranos: 0.1.8)
 */
void lcm2ErrorHandlerFunction        ( cmsContext          ContextID,
                                       cmsUInt32Number     ErrorCode,
                                       const char        * ErrorText )
{
  int code = 0;
#if LCMS_VERSION < 2060
  oyStruct_s * s = ContextID;
#else
  oyStruct_s * s = ContextID ? lcmsGetContextUserData( ContextID ) : NULL;
#endif
  code = oyMSG_ERROR;
  lcm2_msg( code, s, CMM_NICK ": %s", ErrorText );
}

/** Function lcm2CMMMessageFuncSet
 *  @brief
 *
 *  @version Oyranos: 0.1.8
 *  @date    2007/11/00
 *  @since   2007/11/00 (Oyranos: 0.1.8)
 */
int            lcm2CMMMessageFuncSet ( oyMessage_f         message_func )
{
  lcm2_msg = message_func;
  return 0;
}

char lcm2_extra_options[] = {
 "\n\
  <" OY_TOP_SHARED ">\n\
   <" OY_DOMAIN_INTERNAL ">\n\
    <" OY_TYPE_STD ">\n\
     <" "icc_color" ">\n\
      <cmyk_cmyk_black_preservation.advanced>0</cmyk_cmyk_black_preservation.advanced>\n\
      <precalculation.advanced>0</precalculation.advanced>\n\
      <precalculation_curves.advanced>1</precalculation_curves.advanced>\n\
      <adaption_state.advanced>1.0</adaption_state.advanced>\n\
      <no_white_on_white_fixup.advanced>1</no_white_on_white_fixup.advanced>\n\
     </" "icc_color" ">\n\
    </" OY_TYPE_STD ">\n\
   </" OY_DOMAIN_INTERNAL ">\n\
  </" OY_TOP_SHARED ">\n"
};

#define A(long_text) STRING_ADD( tmp, long_text)

/** Function lcm2GetOptionsUI
 *  @brief   return XFORMS for matching options
 *
 *  @version Oyranos: 0.9.5
 *  @date    2014/01/08
 *  @since   2009/07/29 (Oyranos: 0.1.10)
 */
int lcm2GetOptionsUI                 ( oyCMMapiFilter_s   * module,
                                       oyOptions_s        * options,
                                       char              ** ui_text,
                                       oyAlloc_f            allocateFunc )
{
  char * tmp = 0;

  tmp = (char *)oyOptions_FindString( options,
                                      "cmyk_cmyk_black_preservation", 0 );
  if(tmp == 0)
    return 0;

  tmp = oyStringCopy_( "\
  <xf:group type=\"frame\">\
    <xf:label>little CMS 2 ", oyAllocateFunc_ );

  A(       _("Extended Options"));
  A(                         ":</xf:label>\n");
  A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_INTERNAL "/" OY_TYPE_STD "/" "icc_color/cmyk_cmyk_black_preservation\">\n\
      <xf:label>" );
  A(          _("Black Preservation"));
  A(                              "</xf:label>\n\
      <xf:hint>" );
  A(          _("Decide how to preserve the black channel for Cmyk to Cmyk transforms"));
  A(                              "</xf:hint>\n\
      <xf:help>" );
  A(          _("Cmyk to Cmyk transforms can provide various strategies to preserve the black only channel. None means, black might change to Cmy and thus text prints not very well. LittleCMS 2 has added two different modes to deal with that: Black-ink-only preservation and black-plane preservation. The first is simple and effective: do all the colorimetric transforms but keep only K (preserving L*) where the source image is only black. The second mode is fair more complex and tries to preserve the WHOLE K plane."));
  A(                              "</xf:help>\n\
      <xf:choices>\n\
       <xf:item>\n\
        <xf:value>0</xf:value>\n\
        <xf:label>");
  A(            _("[none]"));
  A(                 "</xf:label>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:value>1</xf:value>\n\
        <xf:label>LCMS_PRESERVE_PURE_K</xf:label>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:value>2</xf:value>\n\
        <xf:label>LCMS_PRESERVE_K_PLANE</xf:label>\n\
       </xf:item>\n\
      </xf:choices>\n\
     </xf:select1>\n");
  A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_INTERNAL "/" OY_TYPE_STD "/" "icc_color/precalculation\">\n\
      <xf:label>" );
  A(          _("Optimization"));
  A(                              "</xf:label>\n\
      <xf:hint>" );
  A(          _("Color Transforms can be differently stored internally"));
  A(                              "</xf:hint>\n\
      <xf:help>" );
  A(          _("Little CMS tries to optimize profile chains whatever possible. There are some built-in optimization schemes, and you can add new schemas by using a plug-in. This generally improves the performance of the transform, but may introduce a small delay of 1-2 seconds when creating the transform. If you are going to transform just few colors, you don't need this precalculations. Then, the flag cmsFLAGS_NOOPTIMIZE in cmsCreateTransform() can be used to inhibit the optimization process. See the API reference for a more detailed discussion of the flags."));
  A(                              "</xf:help>\n\
      <xf:choices>\n\
       <xf:item>\n\
        <xf:value>0</xf:value>\n\
        <xf:label>normal</xf:label>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:value>1</xf:value>\n\
        <xf:label>LCMS2_NOOPTIMIZE</xf:label>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:value>2</xf:value>\n\
        <xf:label>LCMS2_HIGHRESPRECALC</xf:label>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:value>3</xf:value>\n\
        <xf:label>LCMS2_LOWRESPRECALC</xf:label>\n\
       </xf:item>\n\
      </xf:choices>\n\
     </xf:select1>\n");
  A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_INTERNAL "/" OY_TYPE_STD "/" "icc_color/precalculation_curves\">\n\
      <xf:label>" );
  A(          _("Curves for Optimization"));
  A(                              "</xf:label>\n\
      <xf:hint>" );
  A(          _("Color Transform CLUT's can additionally use curves for special cases"));
  A(                              "</xf:hint>\n\
      <xf:help>" );
  A(          _("Little CMS can use curves before and after CLUT's for special cases like gamma encoded values to and from linear gamma values. Performance will suffer."));
  A(                              "</xf:help>\n\
      <xf:choices>\n\
       <xf:item>\n\
        <xf:value>0</xf:value>\n\
        <xf:label>");
  A(            _("[none]"));
  A(                 "</xf:label>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:value>1</xf:value>\n\
        <xf:label>LCMS2_POST+PRE_CURVES</xf:label>\n\
       </xf:item>\n\
      </xf:choices>\n\
     </xf:select1>\n");
  A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_INTERNAL "/" OY_TYPE_STD "/" "icc_color/adaption_state\">\n\
      <xf:label>" );
  A(          _("Adaptation State"));
  A(                              "</xf:label>\n\
      <xf:hint>" );
  A(          _("Adaptation state for absolute colorimetric intent"));
  A(                              "</xf:hint>\n\
      <xf:help>" );
  A(          _("The adaption state should be between 0 and 1.0 and will apply to the absolute colorimetric intent."));
  A(                              "</xf:help>\n\
      <xf:choices>\n\
       <xf:item>\n\
        <xf:value>0.0</xf:value>\n\
        <xf:label>0.0</xf:label>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:value>1.0</xf:value>\n\
        <xf:label>1.0</xf:label>\n\
       </xf:item>\n\
      </xf:choices>\n\
     </xf:select1>\n");
  A("\
     <xf:select1 ref=\"/" OY_TOP_SHARED "/" OY_DOMAIN_INTERNAL "/" OY_TYPE_STD "/" "icc_color/no_white_on_white_fixup\">\n\
      <xf:label>" );
  A(          _("No White on White Fix"));
  A(                              "</xf:label>\n\
      <xf:hint>" );
  A(          _("Skip White Point on White point alignment"));
  A(                              "</xf:hint>\n\
      <xf:help>" );
  A(          _("Avoid force of White on White mapping. Default for absolute rendering intent."));
  A(                              "</xf:help>\n\
      <xf:choices>\n\
       <xf:item>\n\
        <xf:value>0</xf:value>\n\
        <xf:label>");
  A(            _("No"));
  A(                 "</xf:label>\n\
       </xf:item>\n\
       <xf:item>\n\
        <xf:value>1</xf:value>\n\
        <xf:label>");
  A(            _("Yes"));
  A(                  "</xf:label>\n\
       </xf:item>\n\
      </xf:choices>\n\
     </xf:select1>\n\
   </xf:group>\n"
   );

  if(allocateFunc && tmp)
  {
    char * t = oyStringCopy_( tmp, allocateFunc );
    oyFree_m_( tmp );
    tmp = t; t = 0;
  } else
    return 1;

  *ui_text = tmp;

  return 0;
}

/** Function lcm2CreateICCMatrixProfile
 *  @brief   create a profile from primaries, white point and one gamma value
 *
 *  Used for ICC from EDID, Camera RAW etc. Marti calls these matrix/shaper.
 *
 *  @version Oyranos: 0.9.6
 *  @date    2014/04/07
 *  @since   2009/10/24 (Oyranos: 0.1.10)
 */
oyProfile_s *      lcm2CreateICCMatrixProfile (
                                       float             gamma,
                                       float rx, float ry,
                                       float gx, float gy,
                                       float bx, float by,
                                       float wx, float wy,
                                       int icc_profile_flags )
{
  cmsCIExyYTRIPLE p;
  cmsToneCurve * g[3] = {0,0,0};
  /* 0.31271, 0.32902 D65 */
  cmsCIExyY wtpt_xyY;
  cmsHPROFILE lp = 0;
  cmsUInt32Number size = 0;
  char * data = 0;

  int error = 0;
  oyProfile_s * prof = 0;

  p.Red.x = rx; 
  p.Red.y = ry;
  p.Red.Y = 1.0;
  p.Green.x = gx;
  p.Green.y = gy;
  p.Green.Y = 1.0;
  p.Blue.x = bx;
  p.Blue.y = by;
  p.Blue.Y = 1.0;
  wtpt_xyY.x = wx;
  wtpt_xyY.y = wy;
  wtpt_xyY.Y = 1.0;
  g[0] = g[1] = g[2] = lcmsBuildGamma(0, (double)gamma);
  lcm2_msg( oyMSG_DBG,0, OY_DBG_FORMAT_
             " red: %g %g %g green: %g %g %g blue: %g %g %g white: %g %g gamma: %g",
             OY_DBG_ARGS_, rx,ry,p.Red.Y, gx,gy,p.Green.Y,bx,by,p.Blue.Y,wx,wy,gamma );
  lp = lcmsCreateRGBProfile( &wtpt_xyY, &p, g);

  if(icc_profile_flags & OY_ICC_VERSION_2)
    lcmsSetProfileVersion(lp, 2.4);

  lcmsSaveProfileToMem( lp, 0, &size );
  if(!size)
    lcm2_msg( oyMSG_WARN,0, OY_DBG_FORMAT_
             "lcmsSaveProfileToMem failed for: red: %g %g %g green: %g %g %g blue: %g %g %g white: %g %g gamma: %g",
             OY_DBG_ARGS_, rx,ry,p.Red.Y, gx,gy,p.Green.Y,bx,by,p.Blue.Y,wx,wy,gamma );
  data = oyAllocateFunc_( size );
  lcmsSaveProfileToMem( lp, data, &size );
  lcmsCloseProfile( lp );
  lcmsFreeToneCurve( g[0] );

  prof = oyProfile_FromMem( size, data, 0,0 );


  error = oyProfile_AddTagText( prof, icSigCopyrightTag,
                                      "no copyright; use freely" );
  if(error) WARNc2_S("%s %d", _("found issues"),error);

  oyDeAllocateFunc_( data ); size = 0;
  return prof;
}

/**
 *  This function implements oyMOptions_Handle_f.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/12/11 (Oyranos: 0.1.10)
 *  @date    2009/12/11
 */
int          lcm2MOptions_Handle     ( oyOptions_s       * options,
                                       const char        * command,
                                       oyOptions_s      ** result )
{
  oyOption_s * o = 0;
  oyProfile_s * prof = 0;
  int error = 0;

  if(oyFilterRegistrationMatch(command,"can_handle", 0))
  {
    if(oyFilterRegistrationMatch(command,"create_profile", 0))
    {
      double val = 0.0;
      o = oyOptions_Find( options,
        "color_matrix.redx_redy_greenx_greeny_bluex_bluey_whitex_whitey_gamma",
                          oyNAME_PATTERN );
      error = oyOptions_FindDouble( options,
        "color_matrix.redx_redy_greenx_greeny_bluex_bluey_whitex_whitey_gamma",
                            8, &val );
      if(!o)
      {
        lcm2_msg( oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_ " "
                 "no option \"color_matrix.redx_redy_greenx_greeny_bluex_bluey_whitex_whitey_gamma\" found",
                 OY_DBG_ARGS_ );
        error = 1;
      } else if( error != 0 )
      {
        lcm2_msg( oyMSG_WARN, (oyStruct_s*)options, OY_DBG_FORMAT_" "
                 "option \"color_matrix.redx_redy_greenx_greeny_bluex_bluey_whitex_whitey_gamma\" %s",
                 OY_DBG_ARGS_,
                 (error < 0) ? "contains less than 9 required values" :
                               "access returned with error" );
      }

      oyOption_Release( &o );

      return error;
    }
    else
      return 1;
  }
  else if(oyFilterRegistrationMatch(command,"create_profile", 0))
  {
    o = oyOptions_Find( options,
         "color_matrix.redx_redy_greenx_greeny_bluex_bluey_whitex_whitey_gamma",
                        oyNAME_PATTERN );
    if(o)
    {
      int32_t icc_profile_flags = 0;
      oyOptions_FindInt( options, "icc_profile_flags", 0, &icc_profile_flags ); 

      prof = lcm2CreateICCMatrixProfile (
                    oyOption_GetValueDouble(o,8),
                    oyOption_GetValueDouble(o,0), oyOption_GetValueDouble(o,1),
                    oyOption_GetValueDouble(o,2), oyOption_GetValueDouble(o,3),
                    oyOption_GetValueDouble(o,4), oyOption_GetValueDouble(o,5),
                    oyOption_GetValueDouble(o,6), oyOption_GetValueDouble(o,7),
                    icc_profile_flags );
      oyOption_Release( &o );

      o = oyOption_FromRegistration( OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD OY_SLASH "icc_profile.create_profile.color_matrix._" CMM_NICK,
                        0 );
      error = oyOption_MoveInStruct( o, (oyStruct_s**) &prof );
      if(!*result)
        *result = oyOptions_New(0);
      oyOptions_MoveIn( *result, &o, -1 );
    }
  }

  return 0;
}

/**
 *  This function implements oyCMMinfoGetText_f.
 *
 *  @version Oyranos: 0.3.0
 *  @since   2011/02/21 (Oyranos: 0.3.0)
 *  @date    2011/02/21
 */
const char * lcm2InfoGetTextProfileC2( const char        * select,
                                       oyNAME_e            type,
                                       oyStruct_s        * context )
{
         if(strcmp(select, "can_handle")==0)
  {
         if(type == oyNAME_NICK)
      return "check";
    else if(type == oyNAME_NAME)
      return _("check");
    else
      return _("Check if this module can handle a certain command.");
  } else if(strcmp(select, "create_profile")==0)
  {
         if(type == oyNAME_NICK)
      return "proofing_effect";
    else if(type == oyNAME_NAME)
      return _("Create a ICC abstract proofing profile.");
    else
      return _("The littleCMS \"create_profile.proofing_effect\" command lets you create ICC abstract profiles from a given ICC profile for proofing. The filter expects a oyOption_s object with name \"proofing_profile\" containing a oyProfile_s as value. The options \"rendering_intent\", \"rendering_intent_proof\", \"rendering_bpc\", \"rendering_gamut_warning\", \"precalculation\", \"precalculation_curves\", \"cmyk_cmyk_black_preservation\", \"adaption_state\"  and \"no_white_on_white_fixup\" are honoured. The result will appear in \"icc_profile\" with the additional attributes \"create_profile.proofing_effect\" as a oyProfile_s object.");
  } else if(strcmp(select, "help")==0)
  {
         if(type == oyNAME_NICK)
      return "help";
    else if(type == oyNAME_NAME)
      return _("Create a ICC proofing profile.");
    else
      return _("The littleCMS \"create_profile.proofing_effect\" command lets you create ICC abstract profiles from some given ICC profile. See the \"proofing_effect\" info item.");
  }
  return 0;
}
const char *lcm2_texts_profile_create[4] = {"can_handle","create_profile","help",0};

/** @instance lcm2_api10_cmm2
 *  @brief    littleCMS oyCMMapi10_s implementation
 *
 *  a filter for proofing effect profile creation
 *
 *  @version Oyranos: 0.3.0
 *  @since   2011/02/21 (Oyranos: 0.3.0)
 *  @date    2011/02/21
 */
oyCMMapi10_s_    lcm2_api10_cmm2 = {

  oyOBJECT_CMM_API10_S,
  0,0,0,
  0,

  lcm2CMMInit,
  lcm2CMMMessageFuncSet,

  OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD OY_SLASH
  "create_profile.proofing_effect.icc._" CMM_NICK "._CPU",

  CMM_VERSION,
  CMM_API_VERSION,                  /**< int32_t module_api[3] */
  0,   /* id_; keep empty */
  0,   /* api5_; keep empty */
  0,   /* runtime_context */
 
  lcm2InfoGetTextProfileC2,            /**< getText */
  (char**)lcm2_texts_profile_create,   /**<texts; list of arguments to getText*/
 
  lcm2MOptions_Handle2                 /**< oyMOptions_Handle_f oyMOptions_Handle */
};

/**
 *  This function implements oyCMMinfoGetText_f.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/12/11 (Oyranos: 0.1.10)
 *  @date    2009/12/11
 */
const char * lcm2InfoGetTextProfileC ( const char        * select,
                                       oyNAME_e            type,
                                       oyStruct_s        * context )
{
         if(strcmp(select, "can_handle")==0)
  {
         if(type == oyNAME_NICK)
      return "check";
    else if(type == oyNAME_NAME)
      return _("check");
    else
      return _("Check if this module can handle a certain command.");
  } else if(strcmp(select, "create_profile")==0)
  {
         if(type == oyNAME_NICK)
      return "create_profile";
    else if(type == oyNAME_NAME)
      return _("Create a ICC matrix profile.");
    else
      return _("The littleCMS \"create_profile.color_matrix\" command lets you create ICC profiles from some given colorimetric coordinates. The filter expects a oyOption_s object with name \"color_matrix.redx_redy_greenx_greeny_bluex_bluey_whitex_whitey_gamma\" containing 9 floats in the order of CIE*x for red, CIE*y for red, CIE*x for green, CIE*y for green, CIE*x for blue, CIE*y for blue, CIE*x for white, CIE*y for white and a gamma value.");
  } else if(strcmp(select, "help")==0)
  {
         if(type == oyNAME_NICK)
      return "help";
    else if(type == oyNAME_NAME)
      return _("Create a ICC matrix profile.");
    else
      return _("The littleCMS \"create_profile.color_matrix\" command lets you create ICC profiles from some given colorimetric coordinates. See the \"create_profile\" info item.");
  }
  return 0;
}

/** @instance lcm2_api10_cmm
 *  @brief    littleCMS oyCMMapi10_s implementation
 *
 *  a filter for simple profile creation
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/12/11 (Oyranos: 0.1.10)
 *  @date    2009/12/11
 */
oyCMMapi10_s_    lcm2_api10_cmm = {

  oyOBJECT_CMM_API10_S,
  0,0,0,
  (oyCMMapi_s*) & lcm2_api10_cmm2,

  lcm2CMMInit,
  lcm2CMMMessageFuncSet,

  OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD OY_SLASH
  "create_profile.color_matrix.icc._" CMM_NICK "._CPU",

  CMM_VERSION,
  CMM_API_VERSION,                  /**< int32_t module_api[3] */
  0,   /* id_; keep empty */
  0,   /* api5_; keep empty */
  0,   /* runtime_context */
 
  lcm2InfoGetTextProfileC,             /**< getText */
  (char**)lcm2_texts_profile_create,   /**<texts; list of arguments to getText*/
 
  lcm2MOptions_Handle                  /**< oyMOptions_Handle_f oyMOptions_Handle */
};


/** @instance lcm2_api6
 *  @brief    littleCMS oyCMMapi6_s implementation
 *
 *  a filter providing CMM API's
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/28 (Oyranos: 0.1.10)
 *  @date    2008/12/28
 */
oyCMMapi6_s_ lcm2_api6_cmm = {

  oyOBJECT_CMM_API6_S,
  0,0,0,
  (oyCMMapi_s*) & lcm2_api10_cmm,

  lcm2CMMInit,
  lcm2CMMMessageFuncSet,

  OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD OY_SLASH
  "icc_color._" CMM_NICK "._CPU." oyCOLOR_ICC_DEVICE_LINK "_" lcm2TRANSFORM,

  CMM_VERSION,
  CMM_API_VERSION,                  /**< int32_t module_api[3] */
  0,   /* id_; keep empty */
  0,   /* api5_; keep empty */
  0,   /* runtime_context */
  
  oyCOLOR_ICC_DEVICE_LINK,  /* data_type_in, "oyDL" */
  lcm2TRANSFORM,             /* data_type_out, lcm2TRANSFORM */
  lcm2ModuleData_Convert     /* oyModuleData_Convert_f oyModuleData_Convert */
};


/** @instance lcm2_api7
 *  @brief    littleCMS oyCMMapi7_s implementation
 *
 *  a filter providing CMM API's
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/27 (Oyranos: 0.1.10)
 *  @date    2008/12/27
 */
oyCMMapi7_s_ lcm2_api7_cmm = {

  oyOBJECT_CMM_API7_S,
  0,0,0,
  (oyCMMapi_s*) & lcm2_api6_cmm,

  lcm2CMMInit,
  lcm2CMMMessageFuncSet,

  OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD OY_SLASH
  "icc_color._" CMM_NICK "._CPU._ACCEL",

  CMM_VERSION,
  CMM_API_VERSION,                  /**< int32_t module_api[3] */
  0,   /* id_; keep empty */
  0,   /* api5_; keep empty */
  0,   /* runtime_context */

  lcm2FilterPlug_CmmIccRun,  /* oyCMMFilterPlug_Run_f */
  lcm2TRANSFORM,             /* data_type, lcm2TRANSFORM */

  (oyConnector_s**) lcm2_cmmIccPlug_connectors,/* plugs */
  1,                         /* plugs_n */
  0,                         /* plugs_last_add */
  (oyConnector_s**) lcm2_cmmIccSocket_connectors,   /* sockets */
  1,                         /* sockets_n */
  0,                         /* sockets_last_add */
};

/**
 *  This function implements oyCMMGetText_f.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/12/22 (Oyranos: 0.1.10)
 *  @date    2009/12/22
 */
const char * lcm2Api4UiGetText (
                                       const char        * select,
                                       oyNAME_e            type,
                                       oyStruct_s        * context )
{
  static char * category = 0;
  if(strcmp(select,"name") ||
     strcmp(select,"help"))
  {
    return lcm2InfoGetText( select, type, context );
  }
  else if(strcmp(select,"category"))
  {
    if(!category)
    {
      STRING_ADD( category, _("Color") );
      STRING_ADD( category, _("/") );
      /* CMM: abbreviation for Color Matching Module */
      STRING_ADD( category, _("CMM") );
      STRING_ADD( category, _("/") );
      STRING_ADD( category, _("littleCMS") );
    }
         if(type == oyNAME_NICK)
      return "category";
    else if(type == oyNAME_NAME)
      return category;
    else
      return category;
  }
  return 0;
}
const char * lcm2_api4_ui_texts[] = {"name", "category", "help", 0};
/** @instance lcm2_api4_ui
 *  @brief    lcm2 oyCMMapi4_s::ui implementation
 *
 *  The UI for lcm2.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2009/09/09 (Oyranos: 0.1.10)
 *  @date    2009/09/09
 */
oyCMMui_s_ lcm2_api4_ui = {
  oyOBJECT_CMM_DATA_TYPES_S,           /**< oyOBJECT_e       type; */
  0,0,0,                            /* unused oyStruct_s fields; keep to zero */

  CMM_VERSION,                         /**< int32_t version[3] */
  CMM_API_VERSION,                            /**< int32_t module_api[3] */

  lcm2Filter_CmmIccValidateOptions, /* oyCMMFilter_ValidateOptions_f */
  lcm2WidgetEvent, /* oyWidgetEvent_f */

  "Color/CMM/littleCMS2", /* category */
  lcm2_extra_options,   /* const char * options */
  lcm2GetOptionsUI,     /* oyCMMuiGet_f oyCMMuiGet */

  lcm2Api4UiGetText, /* oyCMMGetText_f   getText */
  lcm2_api4_ui_texts /* const char    ** texts */
};

/** @instance lcm2_api4_cmm
 *  @brief    littleCMS oyCMMapi4_s implementation
 *
 *  a filter providing CMM API's
 *
 *  @version Oyranos: 0.1.8
 *  @since   2008/07/18 (Oyranos: 0.1.8)
 *  @date    2008/07/18
 */
oyCMMapi4_s_ lcm2_api4_cmm = {

  oyOBJECT_CMM_API4_S,
  0,0,0,
  (oyCMMapi_s*) & lcm2_api7_cmm,

  lcm2CMMInit,
  lcm2CMMMessageFuncSet,

  OY_TOP_SHARED OY_SLASH OY_DOMAIN_INTERNAL OY_SLASH OY_TYPE_STD OY_SLASH
  "icc_color._" CMM_NICK "._CPU._NOACCEL",

  CMM_VERSION,
  CMM_API_VERSION,                  /**< int32_t module_api[3] */
  0,   /* id_; keep empty */
  0,   /* api5_; keep empty */
  0,   /* runtime_context */

  lcm2FilterNode_CmmIccContextToMem, /* oyCMMFilterNode_ContextToMem_f */
  lcm2FilterNode_GetText, /* oyCMMFilterNode_GetText_f */
  oyCOLOR_ICC_DEVICE_LINK, /* context data_type */

  &lcm2_api4_ui                        /**< oyCMMui_s *ui */
};



/**
 *  This function implements oyCMMinfoGetText_f.
 *
 *  @version Oyranos: 0.1.10
 *  @since   2008/12/23 (Oyranos: 0.1.10)
 *  @date    2008/12/30
 */
const char * lcm2InfoGetText         ( const char        * select,
                                       oyNAME_e            type,
                                       oyStruct_s        * context )
{
         if(strcmp(select, "name")==0)
  {
         if(type == oyNAME_NICK)
      return CMM_NICK;
    else if(type == oyNAME_NAME)
      return _("Little CMS 2");
    else
      return _("LittleCMS 2 is a CMM, a color management engine; it implements fast transforms between ICC profiles. \"Little\" stands for its small overhead. With a typical footprint of about 100K including C runtime, you can color-enable your application without the pain of ActiveX, OCX, redistributables or binaries of any kind. We are using little cms in several commercial projects, however, we are offering lcms library free for anybody under an extremely liberal open source license.");
  } else if(strcmp(select, "manufacturer")==0)
  {
         if(type == oyNAME_NICK)
      return "Marti";
    else if(type == oyNAME_NAME)
      return "Marti Maria";
    else
      return _("littleCMS 2 project; www: http://www.littlecms.com; support/email: support@littlecms.com; sources: http://www.littlecms.com/downloads.htm; Oyranos wrapper: Kai-Uwe Behrmann for the Oyranos project");
  } else if(strcmp(select, "copyright")==0)
  {
         if(type == oyNAME_NICK)
      return "MIT";
    else if(type == oyNAME_NAME)
      return _("Copyright (c) 1998-2016 Marti Maria Saguer; MIT");
    else
      return _("MIT license: http://www.opensource.org/licenses/mit-license.php");
  } else if(strcmp(select, "help")==0)
  {
         if(type == oyNAME_NICK)
      return "help";
    else if(type == oyNAME_NAME)
      return _("The lcms \"color_icc\" filter is a one dimensional color conversion filter. It can both create a color conversion context, some precalculated for processing speed up, and the color conversion with the help of that context. The adaption part of this filter transforms the Oyranos color context, which is ICC device link based, to the internal lcms format.");
    else
      return _("The following options are available to create color contexts:\n \"profiles_simulation\", a option of type oyProfiles_s, can contain device profiles for proofing.\n \"profiles_effect\", a option of type oyProfiles_s, can contain abstract color profiles.\n The following Oyranos options are supported: \"rendering_gamut_warning\", \"rendering_intent_proof\", \"rendering_bpc\", \"rendering_intent\", \"proof_soft\" and \"proof_hard\".\n The additional lcms option is supported \"cmyk_cmyk_black_preservation\" [0 - none; 1 - LCMS_PRESERVE_PURE_K; 2 - LCMS_PRESERVE_K_PLANE], \"precalculation\": [0 - normal; 1 - cmsFLAGS_NOOPTIMIZE; 2 - cmsFLAGS_HIGHRESPRECALC, 3 - cmsFLAGS_LOWRESPRECALC], \"precalculation_curves\": [0 - none; 1 - cmsFLAGS_CLUT_POST_LINEARIZATION + cmsFLAGS_CLUT_PRE_LINEARIZATION], \"adaption_state\": [0.0 - not adapted to screen, 1.0 - full adapted to screen] and \"no_white_on_white_fixup\": [0 - force white on white, 1 - keep as is]." );
  }
  return 0;
}
const char *lcm2_texts[5] = {"name","copyright","manufacturer","help",0};
oyIcon_s lcm2_icon = {oyOBJECT_ICON_S, 0,0,0, 0,0,0, "lcms_logo2.png"};

/** @instance lcm2_cmm_module
 *  @brief    lcm2 module infos
 *
 *  @version Oyranos: 0.1.10
 *  @since   2007/11/00 (Oyranos: 0.1.8)
 *  @date    2008/12/30
 */
oyCMM_s lcm2_cmm_module = {

  oyOBJECT_CMM_INFO_S,                 /**< type, struct type */
  0,0,0,                               /**< ,dynamic object functions */
  CMM_NICK,                            /**< cmm, ICC signature */
  "0.6",                               /**< backend_version */
  lcm2InfoGetText,                     /**< getText */
  (char**)lcm2_texts,                  /**<texts; list of arguments to getText*/
  OYRANOS_VERSION,                     /**< oy_compatibility */

  (oyCMMapi_s*) & lcm2_api4_cmm,       /**< api */

  &lcm2_icon, /**< icon */
  lcm2CMMInit                          /**< oyCMMinfoInit_f */
};

