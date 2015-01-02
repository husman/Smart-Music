cmake_minimum_required(VERSION 2.6)

# For MS Visual Studio
project(smartmusic)

set(
    smartmusic_project_sources
    application/server/posix/src/Network.cpp
    application/server/posix/src/Midi.cpp
    application/server/posix/src/ByteReader.cpp
    application/server/posix/src/MidiTrackReader.cpp
    application/server/posix/src/MidiFileDecoder.cpp
    application/server/posix/src/MidiTrackDecoder.cpp
)

add_executable(
    smartmusic
    application/server/posix/src/smartMusic.cpp
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
    application/server/posix/test/test_midi.cpp
)

add_executable(
    runUnitTests
    ${smartmusic_project_sources}
    ${test_sources}
)

# Link test executable against gtest & gtest_main
target_link_libraries(runUnitTests gtest gtest_main gmock gmock_main)

add_test(runUnitTests runUnitTests)