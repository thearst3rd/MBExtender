add_plugin(MBCrypt
  MBDecrypt.cpp
  MBDecrypt.h
  MBPakFile.cpp
  MBPakFile.h
  MBPakFileEntry.h
  MemoryStream.cpp
  MemoryStream.h
  KeyStore.cpp
  KeyStore.h
  )

target_link_libraries(MBCrypt
  PRIVATE
	cryptopp-static
	z)