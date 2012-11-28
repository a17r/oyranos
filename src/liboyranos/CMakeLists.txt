FIND_PACKAGE( Elektra REQUIRED )
INCLUDE_DIRECTORIES( ${INCLUDE_DIRECTORIES} ${ELEKTRA_INCLUDE_DIRS} )
LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${ELEKTRA_LIBRARY_DIRS} )

FIND_PACKAGE( Yajl REQUIRED )
INCLUDE_DIRECTORIES( ${INCLUDE_DIRECTORIES} ${YAJL_INCLUDE_DIRS} )
LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${YAJL_LIBRARY_DIRS} )

SET( CFILES_OBJECT_IMAGING_PUBLIC
	${GENDIR}oyArray2d_s.c
	${GENDIR}oyConnectorImaging_s.c
	${GENDIR}oyConversion_s.c
	${GENDIR}oyImage_s.c
	${GENDIR}oyPixelAccess_s.c
	${GENDIR}${PACKAGE_NAME}_conversion.c
	${GENDIR}${PACKAGE_NAME}_image.c
   )
IF( ${PLATFORM} MATCHES OSX )
  SET( CFILES_OBJECT_IMAGING_CS ${PACKAGE_NAME}_cs.c )
ENDIF()
SET( CFILES_OBJECT_IMAGING
	${CFILES_OBJECT_IMAGING_PUBLIC}
	${GENDIR}oyArray2d_s_.c
	${GENDIR}oyConnectorImaging_s_.c
	${GENDIR}oyConversion_s_.c
	${GENDIR}oyImage_s_.c
	${GENDIR}oyPixelAccess_s_.c
	${PACKAGE_NAME}_alpha.c
	${PACKAGE_NAME}_cmm.c
	${CFILES_OBJECT_IMAGING_CS}
   )
SET( CHEADERS_OBJECT_IMAGING_PUBLIC
	${GENDIR}oyArray2d_s.h
	${GENDIR}oyConnectorImaging_s.h
	${GENDIR}oyConversion_s.h
	${GENDIR}oyImage_s.h
	${GENDIR}oyPixelAccess_s.h
	${GENDIR}${PACKAGE_NAME}_conversion.h
	${GENDIR}${PACKAGE_NAME}_image.h
	${GENDIR}${PACKAGE_NAME}_profile.h
   )
SET( CHEADERS_OBJECT_IMAGING
	${GENDIR}oyArray2d_s_.h
	${GENDIR}oyConnectorImaging_s_.h
	${GENDIR}oyConversion_s_.h
	${GENDIR}oyImage_s_.h
	${GENDIR}oyPixelAccess_s_.h
	${GENDIR}${PACKAGE_NAME}_conversion_internal.h
	${GENDIR}${PACKAGE_NAME}_image_internal.h
	${CHEADERS_OBJECT_IMAGING_PUBLIC}
   )

SET( SOURCES ${CFILES_OBJECT_IMAGING} ${CHEADERS_OBJECT_IMAGING} )

#SET( EXTRA_LIBS ${EXTRA_LIBS} ${ELEKTRA_LIBRARIES} ${YAJL_LIBRARIES} )
ADD_LIBRARY( oyranos SHARED ${SOURCES} )
TARGET_LINK_LIBRARIES (
   oyranos
   oyranos_config oyranos_modules oyranos_object oyranos_core ${EXTRA_LIBS} )

INSTALL (TARGETS oyranos DESTINATION lib${LIB_SUFFIX})
INSTALL (FILES CHEADERS_OBJECT_IMAGING_PUBLIC DESTINATION include/${PACKAGE_NAME})


