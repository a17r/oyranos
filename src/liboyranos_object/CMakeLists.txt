FIND_PACKAGE( LibXml2 )
IF( LIBXML2_INCLUDE_DIR )
  INCLUDE_DIRECTORIES( ${INCLUDE_DIRECTORIES} ${LIBXML2_INCLUDE_DIR} )
ENDIF()
LINK_DIRECTORIES( ${LINK_DIRECTORIES} ${LIBXML2_LIBRARY_DIRS} )


SET ( CFILES_OBJECT_PUBLIC
	oyranos_json.c
	oyranos_tree.c
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
SET( CFILES_OBJECT_PUBLIC ${CFILES_OBJECT_PUBLIC} PARENT_SCOPE )
SET ( CFILES_OBJECT  ${CFILES_OBJECT_PUBLIC}
	${GENDIR}oyBlob_s_.c
	${GENDIR}oyPointer_s_.c
	${GENDIR}oyHash_s_.c
	${GENDIR}oyList_s.c
	${GENDIR}oyList_s_.c
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
SET( CHEADERS_OBJECT_PUBLIC ${CHEADERS_OBJECT_PUBLIC} PARENT_SCOPE )
SET ( CHEADERS_OBJECT
	${CHEADERS_OBJECT_PUBLIC}
	${GENDIR}oyBlob_s_.h
	${GENDIR}oyPointer_s_.h
	${GENDIR}oyRectangle_s_.h
	${GENDIR}oyHash_s_.h
	${GENDIR}oyList_s.h
	${GENDIR}oyList_s_.h
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

SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS_CORE}" )
SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_CORE}" )

IF( LIBXML2_INCLUDE_DIR )
  SET( EXTRA_LIBS_CORE ${EXTRA_LIBS_CORE} ${LIBXML2_LIBRARIES} )
ENDIF()

SET( EXTRA_LIBS_CORE ${EXTRA_LIBS_CORE} ${YAJL_LIBRARIES} )

ADD_LIBRARY( ${PROJECT_NAME}Object SHARED ${SOURCES} )
TARGET_LINK_LIBRARIES (
   ${PROJECT_NAME}Object
   ${PROJECT_NAME}Core ${EXTRA_LIBS_CORE} )
ADD_LIBRARY( oyranos_object-static STATIC ${SOURCES} )
TARGET_LINK_LIBRARIES (
   oyranos_object-static
   oyranos_core-static ${EXTRA_LIBS_CORE} )

# OS X Framework
SET_TARGET_PROPERTIES( ${PROJECT_NAME}Object PROPERTIES
  FRAMEWORK TRUE
  FRAMEWORK_VERSION ${OYRANOS_VERSION}
  PRIVATE_HEADER ""
  PUBLIC_HEADER "${CHEADERS_OBJECT_PUBLIC}"
  RESOURCE ""
#  INSTALL_NAME_DIR "@executable_path/../../../../${FRAMEWORK_DIR}"
#  DEBUG_POSTFIX -d
)

INSTALL( TARGETS ${PROJECT_NAME}Object oyranos_object-static
         RUNTIME DESTINATION bin
         FRAMEWORK DESTINATION ${FRAMEWORK_DIR}
         LIBRARY DESTINATION ${LIB_DIR}
         ARCHIVE DESTINATION lib${LIB_SUFFIX}
         PUBLIC_HEADER DESTINATION include/${PACKAGE_NAME}
       )
