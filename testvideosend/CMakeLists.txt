cmake_minimum_required(VERSION 2.8)

project(RtpVideoSend)

add_executable(RtpVideoSend videosend.c)
target_include_directories(RtpVideoSend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(RtpVideoSend LINK_PUBLIC rtpvideo)
