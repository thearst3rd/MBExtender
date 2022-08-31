#pragma once

#include <TorqueLib/core/resManager.h>
#include <MBExtender/MBExtender.h>
#include "MemoryStream.h"
#include <string>

enum VFSFileType {
	MemoryFile,
	Symlink
};

class VFSFileEntry {
  public:
    std::string path;
    VFSFileType filetype;
    TGE::ResourceObject *symlinkDest;
    TGE::ResourceObject *resObject;
    bool open;
    MemoryStream *mem;

	VFSFileEntry(std::string& path, VFSFileType filetype);
    ~VFSFileEntry();

    void setSymlinkPath(std::string symlinkPath);
    long getFileSize();

};
