 link_directories(../../external/python/libs)

add_plugin(pyTorque
  pytorque.cpp)

if (DEBUG)
target_link_libraries(pyTorque PRIVATE python39_d MathLib)
else()
target_link_libraries(pyTorque PRIVATE python39 MathLib)
endif()