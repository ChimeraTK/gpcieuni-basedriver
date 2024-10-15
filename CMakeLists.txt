cmake_minimum_required(VERSION 3.16)
project("gpcieuni-basedriver")

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 01)
set(${PROJECT_NAME}_PATCH_VERSION 09)
set(CMAKE_PROJECT_VERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})

set(DKMS_DEST /usr/src/gpcieuni-${CMAKE_PROJECT_VERSION})

install(DIRECTORY DESTINATION ${DKMS_DEST})

install(FILES
  "${CMAKE_SOURCE_DIR}/dkms.conf"
  DESTINATION ${DKMS_DEST}
)

install(PROGRAMS
  "${CMAKE_SOURCE_DIR}/dkms.post_build"
  "${CMAKE_SOURCE_DIR}/dkms.post_install"
  "${CMAKE_SOURCE_DIR}/dkms.post_remove"
  DESTINATION ${DKMS_DEST}
)

install(FILES
  "${CMAKE_SOURCE_DIR}/Makefile.kernel"
  DESTINATION ${DKMS_DEST}
  RENAME "Makefile"
)

configure_file(${CMAKE_SOURCE_DIR}/gpcieuni_drv.c.in gpcieuni_drv.c @ONLY)
file(GLOB gpcie_headers ${CMAKE_SOURCE_DIR}/*.h)
file(GLOB gpcie_sources ${CMAKE_SOURCE_DIR}/*.c ${CMAKE_BINARY_DIR}/gpcieuni_drv.c)

install(FILES
  ${gpcie_headers}
  ${gpcie_sources}
  DESTINATION ${DKMS_DEST}
)

install(FILES
  ${gpcie_headers}
  DESTINATION /usr/include/gpcieuni
)

configure_file(${CMAKE_SOURCE_DIR}/dkms-run.sh.in dkms-run.sh @ONLY)
install(CODE
  "execute_process(COMMAND ${CMAKE_COMMAND} -E env CMAKE_PROJECT_VERSION=${CMAKE_PROJECT_VERSION} BINDIR=${CMAKE_BINARY_DIR} bash ${CMAKE_BINARY_DIR}/dkms-run.sh)"
)