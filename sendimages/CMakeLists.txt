#
# Copyright (c) 2010-2015, Gilles Caulier, <caulier dot gilles at gmail dot com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

add_definitions(-DTRANSLATION_DOMAIN=\"kipiplugin_sendimages\")

set(kipiplugin_sendimages_PART_SRCS imageresize.cpp
                                    myimagelist.cpp
                                    settingswidget.cpp
                                    sendimages.cpp
                                    sendimagesdialog.cpp
                                    plugin_sendimages.cpp
)

add_library(kipiplugin_sendimages MODULE ${kipiplugin_sendimages_PART_SRCS})

target_link_libraries(kipiplugin_sendimages
                      PRIVATE

                      KF5::I18n
                      KF5::Service
                      KF5::Kipi

                      KF5kipiplugins
)

configure_file(kipiplugin_sendimages.desktop.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/kipiplugin_sendimages.desktop)

install(FILES   ${CMAKE_CURRENT_BINARY_DIR}/kipiplugin_sendimages.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(TARGETS kipiplugin_sendimages                                     DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES   kipiplugin_sendimagesui.rc                                DESTINATION ${KXMLGUI_INSTALL_DIR}/kipi)
