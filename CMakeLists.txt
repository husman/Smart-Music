#Specify the version being used aswell as the language
cmake_minimum_required(VERSION 2.6)
#Name your project here
project(smartmusic)

#This tells CMake to fib.c and name it fibonacci
# add_definitions("-stdlib=libstdc++ -lstdc++")
add_executable(
    smartmusic
    app/linux/src/ByteReader.cpp
    app/linux/src/MidiTrackReader.cpp
    app/linux/src/MidiFileDecoder.cpp
    app/linux/src/MidiTrackDecoder.cpp
    app/linux/src/smartMusic.cpp
)