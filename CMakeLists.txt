#Specify the version being used aswell as the language
cmake_minimum_required(VERSION 2.6)
#Name your project here
project(smartmusic)

set(
    smartmusic_project_sources
    app/linux/src/Network.cpp
    app/linux/src/Midi.cpp
    app/linux/src/ByteReader.cpp
    app/linux/src/MidiTrackReader.cpp
    app/linux/src/MidiFileDecoder.cpp
    app/linux/src/MidiTrackDecoder.cpp
)

add_executable(
    smartmusic
    app/linux/src/smartMusic.cpp
    ${smartmusic_project_sources}
)


################################
# Unit Tests
################################
include_directories(
    /Users/haleeq/dev/cpp/test/include
)

link_directories(
    /Users/haleeq/dev/cpp/test/libs
)

set(
    test_sources
    app/linux/test/test_midi.cpp
)

add_executable(
    runUnitTests
    ${smartmusic_project_sources}
    ${test_sources}
)

# Link test executable against gtest & gtest_main
target_link_libraries(runUnitTests gtest gtest_main gmock gmock_main)

add_test(runUnitTests runUnitTests)