

TRIBITS_INCLUDE_DIRECTORIES(REQUIRED_DURING_INSTALLATION_TESTING ${CMAKE_CURRENT_SOURCE_DIR}/../UnitTest)
ASSERT_DEFINED(PACKAGE_SOURCE_DIR CMAKE_CURRENT_SOURCE_DIR)

FILE(GLOB SOURCES *.cpp)

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  CharonRythmos_UnitTest
  SOURCES ${SOURCES}
  TESTONLYLIBS rythmos_test_models
  COMM serial
  STANDARD_PASS_OUTPUT
  )
