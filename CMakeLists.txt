PROJECT(easy_message_broker)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(CMAKE_C_FLAGS "-Wall -g -O2")
SET(CMAKE_CXX_FLAGS "-Wall -g -O2")

set(CMAKE_CXX_STANDARD 17)
FILE(GLOB_RECURSE LIB_LIST    "lib/*.cpp")
FILE(GLOB_RECURSE UTIL_LIST   "util/*.cpp")

INCLUDE_DIRECTORIES("include")
INCLUDE_DIRECTORIES("util")

ADD_LIBRARY(ez_stream SHARED ${UTIL_LIST})
ADD_LIBRARY(emb SHARED ${LIB_LIST})
TARGET_LINK_LIBRARIES(emb ez_stream)

ADD_EXECUTABLE(broker  "sample/cpp/broker_test.cpp")
TARGET_LINK_LIBRARIES(broker emb)

ADD_EXECUTABLE(pubsub  "sample/cpp/pub_sub_test.cpp")
TARGET_LINK_LIBRARIES(pubsub emb)
