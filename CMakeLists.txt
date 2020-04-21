cmake_minimum_required (VERSION 2.8)
project (bluez-samples)

find_program(GDBUSCODEGEN NAMES gdbus-codegen)
if (NOT GDBUSCODEGEN)
	message(SEND_ERROR "Could not find gdbus-codegen")
endif(NOT GDBUSCODEGEN)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bluez.c ${CMAKE_CURRENT_BINARY_DIR}/bluez.h
	COMMAND ${GDBUSCODEGEN} --c-namespace=FWBluez --interface-prefix=org.bluez --c-generate-object-manager --generate-c-code=bluez ${PROJECT_SOURCE_DIR}/bluez.xml
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	DEPENDS ${PROJECT_SOURCE_DIR}/bluez.xml)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS_X REQUIRED dbus-1)
pkg_check_modules(BLUEZ REQUIRED bluez)
pkg_check_modules(GIO REQUIRED gio-2.0 gio-unix-2.0)

#message( $ENV{PKG_CONFIG_SYSROOT_DIR})
#message( $ENV{PKG_CONFIG_PATH})
#message("Dbus:" ${DBUS_X_INCLUDE_DIRS})
#message("Bluez:" ${BLUEZ_INCLUDE_DIRS})

add_executable(server common.h common.c rfcomm-server.c dbus-bt.c)
add_executable(client common.h common.c rfcomm-client.c)

add_library( bluezlib
	${CMAKE_CURRENT_BINARY_DIR}/bluez.c
	)

#add_custom_target(bluezlib_refs SOURCES fwbluez.h)
target_link_libraries(bluezlib PUBLIC ${GIO_LIBRARIES} pthread pcre)
target_include_directories(bluezlib PUBLIC ${GIO_INCLUDE_DIRS} ${DBUS_X_INCLUDE_DIRS})
target_include_directories (bluezlib PUBLIC .)
target_include_directories (bluezlib PUBLIC ${PROJECT_BINARY_DIR})
target_include_directories (bluezlib PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(bluezlib PRIVATE -W -Wall -Wextra -Wno-unused-parameter)

target_link_libraries(server ${BLUEZ_LIBRARIES} ${DBUS_X_LIBRARIES} bluezlib pthread pcre)
target_include_directories(server PUBLIC ${BLUEZ_INCLUDE_DIRS} ${DBUS_X_INCLUDE_DIRS})
target_compile_options(server PUBLIC ${BLUEZ_CFLAGS_OTHER} ${DBUS_X_CFLAGS_OTHER})

target_link_libraries(client ${BLUEZ_LIBRARIES})
target_include_directories(client PUBLIC ${BLUEZ_INCLUDE_DIRS})
target_compile_options(client PUBLIC ${BLUEZ_CFLAGS_OTHER})

# Install target for qtcreator debugging
set(CMAKE_INSTALL_PREFIX "/opt/hgs")
set(CMAKE_BUILD_TYPE "Release")

install(TARGETS server RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/)
install(TARGETS client RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/)
