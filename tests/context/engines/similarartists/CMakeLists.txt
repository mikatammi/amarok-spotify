include_directories(
  ${AMAROK_SOURCE_TREE}/context/engines/similarartists
  ${AMAROK_SOURCE_TREE}/context/applets/similarartists
  ${AMAROK_SOURCE_TREE}/network
  ${AMAROK_TEST_TREE}/context/engines
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
)


set( testsimilarartistsengine_SRCS
    TestSimilarArtistsEngine.cpp
    ${AMAROK_TEST_TREE}/context/engines/ContextView.cpp
    ${AMAROK_TEST_TREE}/context/engines/TestDataEngine.cpp
    ${AMAROK_SOURCE_TREE}/context/ContextObserver.cpp
    ${AMAROK_SOURCE_TREE}/context/engines/similarartists/SimilarArtistsEngine.cpp
    ${AMAROK_SOURCE_TREE}/context/applets/similarartists/SimilarArtist.cpp
)

kde4_add_unit_test( testsimilarartistsengine ${testsimilarartistsengine_SRCS} )

add_dependencies( testsimilarartistsengine amaroklib)

target_link_libraries( testsimilarartistsengine amarokcore amaroklib ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_PLASMA_LIBS} ${LIBLASTFM_LIBRARY} ${QT_QTTEST_LIBRARY})
