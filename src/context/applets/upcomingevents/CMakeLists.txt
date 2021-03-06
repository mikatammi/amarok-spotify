project(context-upcomingevents)

include_directories(
    ${Amarok_SOURCE_DIR}/src/context/widgets
    ${Amarok_SOURCE_DIR}/src/network
    ${Amarok_SOURCE_DIR}/src
    )

set( upcomingEvents_SRCS
     LastFmEvent.cpp
     LastFmEventXmlParser.cpp
     UpcomingEventsApplet.cpp
     UpcomingEventsWidget.cpp
     UpcomingEventsStack.cpp
     UpcomingEventsStackItem.cpp
     UpcomingEventsMapWidget.cpp
     UpcomingEventsCalendarWidget.cpp
     )

kde4_add_ui_files( upcomingEvents_SRCS
                   upcomingEventsGeneralSettings.ui
                   upcomingEventsVenueSettings.ui
                   )
kde4_add_plugin(amarok_context_applet_upcomingEvents ${upcomingEvents_SRCS})

if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_upcomingEvents PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
target_link_libraries(amarok_context_applet_upcomingEvents
    amarokcore
    amaroklib
    ${KDE4_PLASMA_LIBS}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KDEWEBKIT_LIBS}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTWEBKIT_LIBRARY}
)

install(TARGETS amarok_context_applet_upcomingEvents DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-upcomingEvents.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES upcoming-events-map.html DESTINATION ${DATA_INSTALL_DIR}/amarok/data)
