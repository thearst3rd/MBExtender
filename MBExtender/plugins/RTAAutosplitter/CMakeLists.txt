add_plugin(RTAAutosplitter
  RTAAutosplitter.cpp)

target_include_directories(RTAAutosplitter
  PRIVATE
    ../MathExtension)

target_link_libraries(RTAAutosplitter
  PRIVATE
    MathLib)