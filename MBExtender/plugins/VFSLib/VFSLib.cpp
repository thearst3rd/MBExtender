#include "VFSLib.h"

#include <unordered_map>

#include <MBExtender/MBExtender.h>
#include <TorqueLib/console/console.h>
#include <TorqueLib/core/bitStream.h>
#include <TorqueLib/core/stringTable.h>
#include <TorqueLib/core/resManager.h>
#include <TorqueLib/core/stream.h>
#include <TorqueLib/core/stringTable.h>
#include <TorqueLib/core/fileStream.h>
#include <TorqueLib/core/fileio.h>
#include <TorqueLib/core/tVector.h>
#include <regex>

MBX_MODULE(VFSLib);

std::unordered_map<const char *, VFSFileEntry *> vfs;
std::unordered_map<TGE::File*, VFSFileEntry*> openFiles;

bool initPlugin(MBX::Plugin& plugin)
{
	MBX_INSTALL(plugin, VFSLib);
	return true;
}

std::string getFileName(const std::string& path) {
	size_t lastSlashPos = path.find_last_of("/");
	return (lastSlashPos != std::string::npos) ? path.substr(lastSlashPos + 1) : path;
}

std::string getFileDir(std::string filename) {
    std::string directory;
    const size_t last_slash_idx = filename.rfind('/');
    if (std::string::npos != last_slash_idx) {
        directory = filename.substr(0, last_slash_idx);
    }
    return directory;
}

std::string combine(const std::string& left, const std::string& right) {
	if (left.empty()) {
		return right;
	}
	else if (right.empty()) {
		return left;
	}
	char leftEnd = left[left.length() - 1];
	char rightStart = right[0];
	if (strchr("/\\", leftEnd) || strchr("/\\", rightStart)) {
		return left + right;  // A separator already exists between the two paths
	}
	return left + "/" + right;
}

VFSFileEntry::VFSFileEntry(std::string& path, VFSFileType filetype) {
    this->path = path;
    this->filetype = filetype;
    this->open = false;

	std::string dpath = combine("__vfs", getFileDir(path));
    std::string fname = getFileName(path);

	TGE::ResourceObject *ro = TGE::ResourceManager->createResource(TGE::StringTable->insert(dpath.c_str(), false),
                                         TGE::StringTable->insert(fname.c_str(), false));
    ro->flags = TGE::ResourceObject::Flags::File;
    ro->fileOffset = 0;
    ro->fileSize = 0;

	this->resObject = ro;

	if (this->filetype == MemoryFile) {
        this->mem = new MemoryStream();
	}

	vfs[TGE::StringTable->insert(path.c_str(), false)] = this;
}

VFSFileEntry::~VFSFileEntry() {
	if (this->filetype == MemoryFile) {
        delete this->mem;
	}

	TGE::ResourceManager->freeResource(this->resObject);
    vfs.erase(TGE::StringTable->insert(this->path.c_str(), false));
}

void VFSFileEntry::setSymlinkPath(std::string symlinkPath) {
    const char *symlinkSTE = TGE::StringTable->insert(symlinkPath.c_str(), false);
    TGE::ResourceObject *symRO = TGE::ResourceManager->find(symlinkSTE);
	if (symRO != NULL) {
        this->symlinkDest = symRO;
	}
}

long VFSFileEntry::getFileSize() {
	if (this->filetype == MemoryFile) {
        return this->mem->length();
	}
	if (this->filetype == Symlink) {
        return this->resObject->fileSize;
	}
}

MBX_OVERRIDE_MEMBERFN(TGE::Stream*, TGE::ResManager::openStream, (TGE::ResManager* thisptr, const char* path), origOpenStream) {
	std::string fn = std::string(path);
	if (fn.rfind("vfs://", 0) == 0 || fn.rfind("__vfs/", 0) == 0) {
		fn = fn.substr(6);
		const char* fnSTE = TGE::StringTable->insert(fn.c_str(), false);

		auto& findf = vfs.find(fnSTE);
		if (findf != vfs.end()) {
			fn = combine("__vfs", fn);
			path = TGE::StringTable->insert(fn.c_str(), false);
		}
	}
	return origOpenStream(thisptr, path);
}

MBX_OVERRIDE_MEMBERFN(TGE::ResourceObject*, TGE::ResManager::find, (TGE::ResManager* thisptr, const char* path), origFind) {
	std::string fn = std::string(path);
	if (fn.rfind("vfs://", 0) == 0 || fn.rfind("__vfs/", 0) == 0) {
		fn = fn.substr(6);
		const char* fnSTE = TGE::StringTable->insert(fn.c_str(), false);

		auto& findf = vfs.find(fnSTE);
		if (findf != vfs.end()) {
			return (*findf).second->resObject;
		}
	}
	return origFind(thisptr, path);
}

MBX_OVERRIDE_MEMBERFN(TGE::ResourceInstance*, TGE::ResManager::loadInstance, (TGE::ResManager* thisptr, const char* path, bool computeCRC), origLoadInstance) {
	std::string fn = std::string(path);
	if (fn.rfind("vfs://", 0) == 0 || fn.rfind("__vfs/", 0) == 0) {
		fn = fn.substr(6);
		const char* fnSTE = TGE::StringTable->insert(fn.c_str(), false);

		auto& findf = vfs.find(fnSTE);
		if (findf != vfs.end()) {
			fn = combine("__vfs", fn);
			path = TGE::StringTable->insert(fn.c_str(), false);
		}
	}
	return origLoadInstance(thisptr, path, computeCRC);
}

MBX_OVERRIDE_MEMBERFN(U32, TGE::ResManager::getSize, (TGE::ResManager* thisptr, const char* path), origRGetSize) {
	std::string fn = std::string(path);
	if (fn.rfind("vfs://", 0) == 0 || fn.rfind("__vfs/", 0) == 0) {
		fn = fn.substr(6);
		const char* fnSTE = TGE::StringTable->insert(fn.c_str(), false);

		auto& findf = vfs.find(fnSTE);
		if (findf != vfs.end()) {
			return (*findf).second->getFileSize();
		}
	}
	return origRGetSize(thisptr, path);
}

MBX_OVERRIDE_MEMBERFN(bool, TGE::ResManager::openFileForWrite, (TGE::ResManager* thisptr, TGE::FileStream& stream, const char* fileName, U32 accessMode), origOpenForWrite) {
	std::string fn = std::string(fileName);
	if (fn.rfind("vfs://", 0) == 0 || fn.rfind("__vfs/", 0) == 0) {
		fn = fn.substr(6);
		const char* fnSTE = TGE::StringTable->insert(fn.c_str(), false);
		
		if (vfs.find(fnSTE) == vfs.end()) {
			new VFSFileEntry(fn, VFSFileType::MemoryFile);  // Create vfs file in memory
		}

		if (!stream.open(fileName, accessMode)) {
			return false;
		}
		return true;
	}
	return origOpenForWrite(thisptr, stream, fileName, accessMode);
}

MBX_OVERRIDE_MEMBERFN(TGE::File::FileStatus, TGE::File::open,
                      (TGE::File * thisptr, const char *filename, const TGE::File::AccessMode openMode), origOpen) {

    std::string fn = std::string(filename);
	if (fn.rfind("vfs://", 0) == 0 || fn.rfind("__vfs/", 0) == 0) {
		fn = fn.substr(6);
		const char* fnSTE = TGE::StringTable->insert(fn.c_str(), false);

		if (openMode == TGE::File::AccessMode::Write || openMode == TGE::File::AccessMode::WriteAppend) {
			if (vfs.find(fnSTE) == vfs.end()) {
				new VFSFileEntry(fn, VFSFileType::MemoryFile); // Create vfs file in memory
			}
		}

		if (vfs.find(fnSTE) != vfs.end()) {
			VFSFileEntry*& entry = vfs[fnSTE];

			if (entry->open) {
				thisptr->currentStatus = TGE::File::FileStatus::IOError;
				thisptr->capability = 0;
			} else {
				if (entry->filetype == MemoryFile) {
					if (openMode == TGE::File::AccessMode::Read)
						thisptr->capability = TGE::File::Capability::FileRead;
					if (openMode == TGE::File::AccessMode::ReadWrite)
						thisptr->capability = TGE::File::Capability::FileRead | TGE::File::Capability::FileWrite;
					if (openMode == TGE::File::AccessMode::Write || openMode == TGE::File::AccessMode::WriteAppend)
						thisptr->capability = TGE::File::Capability::FileWrite;

					entry->mem->seek(0);
					if (openMode == TGE::File::AccessMode::WriteAppend)
						entry->mem->seek(entry->mem->length());

					entry->open = true;

					openFiles[thisptr] = entry;

					thisptr->setStatus(TGE::File::FileStatus::Ok);

					return TGE::File::FileStatus::Ok;
				}

				if (entry->filetype == Symlink) {
					return origOpen(thisptr, entry->symlinkDest->path, openMode);
				}
			}
		}
	}
    return origOpen(thisptr, filename, openMode);
}

MBX_OVERRIDE_MEMBERFN(TGE::File::FileStatus, TGE::File::close, (TGE::File* thisptr), origClose)
{
	if (openFiles.find(thisptr) != openFiles.end()) {
		VFSFileEntry* ptr = openFiles[thisptr];
		ptr->open = false;
		openFiles.erase(thisptr); // Close existing file cause bruh
		thisptr->currentStatus = TGE::File::FileStatus::Closed;

		return TGE::File::FileStatus::Closed;
	}

	return origClose(thisptr);
}

MBX_OVERRIDE_MEMBERFN(TGE::File::FileStatus, TGE::File::read, (TGE::File* thisptr, U32 size, char* dst, U32* bytesRead), origRead)
{
	if (openFiles.find(thisptr) != openFiles.end()) {
		VFSFileEntry* str = openFiles[thisptr];
		U32 bRead;

		if (str->mem->tell() + size > str->mem->length())
		{
			bRead = (str->mem->length() - str->mem->tell());
			thisptr->currentStatus = TGE::File::FileStatus::EOS;
		}
		else
		{
			bRead = size;
			thisptr->currentStatus = TGE::File::FileStatus::Ok;
		}
		uint8_t* buffer = str->mem->getBuffer();
		memcpy(dst, (buffer + str->mem->tell()), bRead);
		try
		{
			str->mem->seek(str->mem->tell() + bRead);
		}
		catch (...)
		{
			thisptr->currentStatus = TGE::File::FileStatus::EOS;
		}
		if (bytesRead != NULL) {
			*bytesRead = bRead;
		}
		return thisptr->currentStatus;
	}

	return origRead(thisptr, size, dst, bytesRead);
}

MBX_OVERRIDE_MEMBERFN(TGE::File::FileStatus, TGE::File::write, (TGE::File* thisptr, U32 size, const char* src, U32* bytesWritten), origWrite)
{
	if (openFiles.find(thisptr) != openFiles.end()) {
		VFSFileEntry* str = openFiles[thisptr];

		str->mem->writeBuffer(src, size);
		thisptr->currentStatus = TGE::File::FileStatus::Ok;
		if (bytesWritten != NULL)
			*bytesWritten = size;

		return thisptr->currentStatus;
	}

	return origWrite(thisptr, size, src, bytesWritten);
}

MBX_OVERRIDE_MEMBERFN(TGE::File::FileStatus, TGE::File::setPosition, (TGE::File* thisptr, S32 position, bool absolutePos), origSetPosition)
{
	if (openFiles.find(thisptr) != openFiles.end()) {
		VFSFileEntry* str = openFiles[thisptr];
		if (absolutePos)
			str->mem->seek(position);
		else
			str->mem->seek(str->mem->tell() + position);

		if (str->mem->length() == str->mem->tell())
		{
			thisptr->currentStatus = TGE::File::FileStatus::EOS;
			return TGE::File::FileStatus::EOS;
		}
		thisptr->currentStatus = TGE::File::FileStatus::Ok;
		return TGE::File::FileStatus::Ok;
	}

	return origSetPosition(thisptr, position, absolutePos);
}

MBX_OVERRIDE_MEMBERFN(U32, TGE::File::getPosition, (TGE::File* thisptr), origGetPosition)
{
	if (openFiles.find(thisptr) != openFiles.end()) {
		VFSFileEntry* str = openFiles[thisptr];
		return (U32)str->mem->tell();
	}

	return origGetPosition(thisptr);
}

MBX_OVERRIDE_MEMBERFN(U32, TGE::File::getSize, (TGE::File* thisptr), origGetSize)
{
	if (openFiles.find(thisptr) != openFiles.end()) {
		VFSFileEntry* str = openFiles[thisptr];
		return (U32)str->mem->length();
	}

	return origGetSize(thisptr);
}

MBX_OVERRIDE_DESTRUCTOR(TGE::File, (TGE::File* thisptr), origDtor)
{
	if (openFiles.find(thisptr) != openFiles.end()) {
		VFSFileEntry* str = openFiles[thisptr];
		str->open = false;
		openFiles.erase(thisptr); // Close existing file cause bruh
		thisptr->currentStatus = TGE::File::FileStatus::Closed;
	}

	origDtor(thisptr);
}