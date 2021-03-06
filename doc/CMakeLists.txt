CONFIGURE_FILE(
                "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
                "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
                @ONLY
       )
CONFIGURE_FILE(
                "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.files.in"
                "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.files"
                @ONLY
       )
CONFIGURE_FILE(
                "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.files.alpha.in"
                "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.files.alpha"
                @ONLY
       )
CONFIGURE_FILE(
                "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile_alpha.in"
                "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile_alpha"
                @ONLY
       )

IF( HAVE_FLTK )
  SET( MAN_FLTK
     man/oyranos-config-fltk.1
     )
ENDIF()

SET( MAN1_PAGES
     ${MAN_FLTK}
     man/oyranos-monitor.1
     man/oyranos-monitor-daemon.1
     man/oyranos-policy.1
     man/oyranos-profile.1
     man/oyranos-profile-graph.1
     man/oyranos-profile-install.1
     man/oyranos-profiles.1
     man/qcmsevents.1
     man/qcmsevents-qt4.1
   )
SET( MAN3_PAGES
     man/oyranos.3
     man/oyranos-config.3
   )

INSTALL( FILES ${MAN1_PAGES} DESTINATION share/man/man1 )
INSTALL( FILES ${MAN3_PAGES} DESTINATION share/man/man3 )

FIND_PACKAGE( Doxygen )
IF( DOXYGEN_FOUND )

  ADD_CUSTOM_COMMAND( OUTPUT html
    COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.files | sed 's%\;${CMAKE_SOURCE_DIR}/src/API_generated/% %g \; s%\;% %g' >> Doxyfile \; doxygen Doxyfile \; cp ${CMAKE_SOURCE_DIR}/extras/icons/${PACKAGE_NAME}_logo.png html/ \; cp ${CMAKE_CURRENT_SOURCE_DIR}/doxygen.css html/ \; cat ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.files.alpha | sed 's%\;${CMAKE_SOURCE_DIR}/src/API_generated/% %g \; s%\;% %g' >> Doxyfile_alpha \; doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile_alpha \; cp ${CMAKE_SOURCE_DIR}/extras/icons/${PACKAGE_NAME}_logo.png doc_alpha/html/
                    )

  ADD_CUSTOM_TARGET( docu ALL
                     DEPENDS html )

  INSTALL( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc/${PACKAGE_NAME}-${OYRANOS_VERSION} )
ENDIF()
