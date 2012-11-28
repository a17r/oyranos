FIND_PACKAGE( LibXml2 REQUIRED )
INCLUDE_DIRECTORIES( ${INCLUDE_DIRECTORIES} ${LIBXML2_INCLUDE_DIRS} )
LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${LIBXML2_LIBRARY_DIRS} )


SET ( CFILES_OBJECT_PUBLIC
	${GENDIR}oyBlob_s.c
	${GENDIR}oyPointer_s.c
	${GENDIR}oyRectangle_s.c
	${GENDIR}oyHash_s.c
	${GENDIR}oyObject_s.c
	${GENDIR}oyObserver_s.c
	${GENDIR}oyOption_s.c
	${GENDIR}oyOptions_s.c
	${GENDIR}oyStruct_s.c
	${GENDIR}oyStructList_s.c
	${GENDIR}${PACKAGE_NAME}_object.c
	${GENDIR}${PACKAGE_NAME}_generic.c
    )
SET ( CFILES_OBJECT  ${CFILES_OBJECT_PUBLIC}
	${GENDIR}oyBlob_s_.c
	${GENDIR}oyPointer_s_.c
	${GENDIR}oyHash_s_.c
	${GENDIR}oyName_s_.c
	${GENDIR}oyName_s.c
	${GENDIR}oyObject_s_.c
	${GENDIR}oyObserver_s_.c
	${GENDIR}oyOption_s_.c
	${GENDIR}oyOptions_s_.c
	${GENDIR}oyRectangle_s_.c
	${GENDIR}oyStructList_s_.c
    )
SET ( CHEADERS_OBJECT_PUBLIC
	${GENDIR}oyBlob_s.h
	${GENDIR}oyPointer_s.h
	${GENDIR}oyRectangle_s.h
	${GENDIR}oyHash_s.h
	${GENDIR}oyName_s.h
	${GENDIR}oyObject_s.h
	${GENDIR}oyObserver_s.h
	${GENDIR}oyOption_s.h
	${GENDIR}oyOptions_s.h
	${GENDIR}oyStruct_s.h
	${GENDIR}oyStructList_s.h
	${GENDIR}${PACKAGE_NAME}_object.h
	${GENDIR}${PACKAGE_NAME}_generic.h
    )
SET ( CHEADERS_OBJECT
	${CHEADERS_OBJECT_PUBLIC}
	${GENDIR}oyBlob_s_.h
	${GENDIR}oyPointer_s_.h
	${GENDIR}oyRectangle_s_.h
	${GENDIR}oyHash_s_.h
	${GENDIR}oyName_s_.h
	${GENDIR}oyObject_s_.h
	${GENDIR}oyObserver_s_.h
	${GENDIR}oyOption_s_.h
	${GENDIR}oyOptions_s_.h
	${GENDIR}oyStructList_s_.h
	${GENDIR}${PACKAGE_NAME}_generic_internal.h
	${GENDIR}${PACKAGE_NAME}_module_internal.h
	${GENDIR}${PACKAGE_NAME}_object_internal.h
    )

SET( SOURCES ${CFILES_OBJECT} ${SRC_FILES} ${CHEADERS_OBJECT} )

SET( EXTRA_LIBS ${EXTRA_LIBS} ${LIBXML2_LIBRARIES} )
ADD_LIBRARY( oyranos_object SHARED ${SOURCES} )
TARGET_LINK_LIBRARIES (
   oyranos_object
   oyranos_core ${EXTRA_LIBS} )

INSTALL (TARGETS oyranos_object DESTINATION lib${LIB_SUFFIX})
INSTALL (FILES CHEADERS_OBJECT_PUBLIC DESTINATION include/${PACKAGE_NAME})