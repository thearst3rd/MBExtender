// MBDecrypt.cpp : Defines the entry point for the application.
//

#include "MBDecrypt.h"
#include "MBPakFile.h"
#include "MBPakFileEntry.h"
#include "MemoryStream.h"
#include "KeyStore.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <map>
#include <MBExtender/MBExtender.h>
#include <TorqueLib/core/resManager.h>
#include <TorqueLib/core/stream.h>
#include <TorqueLib/core/stringTable.h>
#include <TorqueLib/core/fileStream.h>
#include <TorqueLib/core/bitStream.h>
#include <TorqueLib/core/fileio.h>
#include <TorqueLib/core/tVector.h>
#include <TorqueLib/console/console.h>
#include <string>
#include <vector>
#include <regex>

MBX_MODULE(MBCrypt)

static std::vector<MBPakFile*> loadedPackages;
static std::unordered_map<TGE::File*, MemoryStream*> openPakFiles;
static std::unordered_map<const char*, int> loadedFileRefs;
static KeyStore keyStore;

bool icompare_pred(unsigned char a, unsigned char b)
{
	return std::tolower(a) == std::tolower(b);
}

bool icompare(std::string const& a, std::string const& b)
{
	if (a.length() == b.length()) {
		return std::equal(b.begin(), b.end(),
			a.begin(), icompare_pred);
	}
	else {
		return false;
	}
}

bool initPlugin(MBX::Plugin& plugin)
{
	MBX_INSTALL(plugin, MBCrypt);
	keyStore.Load();

	std::string zipn = std::string("boot") + ".mbpak";
	std::string path = std::string("packages/") + zipn;
	try
	{
		MBPakFile* pak = new MBPakFile(path, &keyStore);
		if (pak->failed)
		{
			TGE::Con::errorf("Could not load package boot");
			delete pak;
		}
		else
		{
			loadedPackages.push_back(pak);
		}
	}
	catch (...)
	{
		TGE::Con::errorf("Could not load package boot");
	}

	return true;
}

std::string getFileName(std::string filePath, bool withExtension = true, char seperator = '/')
{
	// Get last dot position
	std::size_t dotPos = filePath.rfind('.');
	std::size_t sepPos = filePath.rfind(seperator);
	if (sepPos != std::string::npos)
	{
		return filePath.substr(sepPos + 1, filePath.size() - (withExtension || dotPos != std::string::npos ? 1 : dotPos));
	}
	return "";
}

std::string getFileDir(std::string filename)
{
	std::string directory;
	const size_t last_slash_idx = filename.rfind('/');
	if (std::string::npos != last_slash_idx)
	{
		directory = filename.substr(0, last_slash_idx);
	}
	return directory;
}

void loadPackageContents(MBPakFile* package)
{
	for (auto& entry : package->entries)
	{
		std::string fname = getFileName(entry.filepath);

		std::string dpath = getFileDir(entry.filepath);

		const char* fpath = TGE::StringTable->insert(entry.filepath.c_str(), false);

		if (TGE::ResourceManager->find(fpath) == NULL)
		{
			TGE::ResourceObject* ro = TGE::ResourceManager->createResource(TGE::StringTable->insert(dpath.c_str(), false), TGE::StringTable->insert(fname.c_str(), false));
			ro->flags = TGE::ResourceObject::Flags::File;
			ro->fileOffset = 0;
			ro->fileSize = entry.uncompressedSize;
			ro->compressedFileSize = entry.uncompressedSize;

			loadedFileRefs[fpath] = 1;
		}
		else
		{
			// Reference counting thing so we can properly delete ResourceObjects pointing to same file
			if (loadedFileRefs.find(fpath) == loadedFileRefs.end())
			{
				loadedFileRefs[fpath] = 2;
			}
			else
			{
				loadedFileRefs[fpath]++;
			}
		}
	}
}

MBX_CONSOLE_FUNCTION(loadMBPackage, void, 2, 2, "loadMBPackage(package)")
{
	TGE::Con::printf("Loading package %s", argv[1]);
	std::string zipn = std::string(argv[1]) + ".mbpak";
	std::string path = std::string("packages/") + zipn;

	try
	{
		MBPakFile* pak = new MBPakFile(path, &keyStore);
		if (pak->failed)
		{
			TGE::Con::errorf("Could not load package %s", argv[1]);
			delete pak;
		}
		else
		{
			loadedPackages.push_back(pak);
			loadPackageContents(pak);
		}
	}
	catch (...)
	{
		TGE::Con::errorf("Could not load package %s", argv[1]);
	}
	
}

void unloadPackageContents(MBPakFile* package)
{
	for (auto& entry : package->entries)
	{
		std::string fname = getFileName(entry.filepath);
		std::string dpath = getFileDir(entry.filepath);

		const char* fpath = TGE::StringTable->insert(entry.filepath.c_str(), false);

		if (loadedFileRefs.find(fpath) != loadedFileRefs.end())
		{
			loadedFileRefs[fpath]--;

			if (loadedFileRefs[fpath] <= 0)
			{
				// Delete the thing then

				TGE::ResourceObject* ro = TGE::ResourceManager->find(fpath);
				if (ro != NULL)
					TGE::ResourceManager->freeResource(ro);

				loadedFileRefs.erase(fpath);
			}
		}
	}
}

MBX_CONSOLE_FUNCTION(unloadMBPackage, void, 2, 2, "unloadMBPackage(package)")
{
	TGE::Con::printf("Unloading package %s", argv[1]);
	std::string zipn = std::string(argv[1]) + ".mbpak";
	std::string path = std::string("packages/") + zipn;

	int idx = -1;
	for (int i = 0; i < loadedPackages.size(); i++)
	{
		if (loadedPackages[i]->path == path)
		{
			idx = i;
			break;
		}
	}

	if (idx != -1)
	{
		MBPakFile* pak = loadedPackages[idx];
		unloadPackageContents(pak);
		delete pak;
		loadedPackages.erase(loadedPackages.begin() + idx);
	}
}

MBX_CONSOLE_FUNCTION(isLoadedMBPackage, bool, 2, 2, "isLoadedMBPackage(package)")
{
	std::string zipn = std::string(argv[1]) + ".mbpak";
	std::string path = std::string("packages/") + zipn;

	for (auto& pak : loadedPackages)
	{
		if (pak->path == path)
		{
			return true;
		}
	}
	return false;
}

MBX_CONSOLE_FUNCTION(deletePackage, bool, 2, 2, "deletePackage(file)")
{
	std::string zipn = std::string(argv[1]) + ".mbpak";
	std::string path = std::string("packages/") + zipn;

	//Burp
	if (remove(path.c_str()) != 0)
	{
		return false;
	}

	return true;
}

MBX_OVERRIDE_MEMBERFN(TGE::File::FileStatus, TGE::File::open, (TGE::File* thisptr, const char* filename, const TGE::File::AccessMode openMode), origOpen)
{
	if (openPakFiles.find(thisptr) != openPakFiles.end()) {
		MemoryStream* ptr = openPakFiles[thisptr];
		delete ptr;
		openPakFiles.erase(thisptr); // Close existing file cause bruh
	}

	std::string fn = std::string(filename);
	const char* fnSTE = TGE::StringTable->insert(filename, false);
	for (auto& pak : loadedPackages)
	{
		if (pak->entryMap.find(fnSTE) != pak->entryMap.end())
		{
			MBPakFileEntry* entry = &pak->entries[pak->entryMap[fnSTE]];

			if (openMode == TGE::File::AccessMode::Read)
			{
				int64_t bufSize;
				char* buf = pak->ReadFile(entry, keyStore.aesKey, &bufSize);
				MemoryStream* str = new MemoryStream();
				str->useBuffer((uint8_t*)buf, bufSize);
				openPakFiles.insert(std::make_pair(thisptr, str));
				thisptr->currentStatus = TGE::File::FileStatus::Ok;
				thisptr->capability = TGE::File::Capability::FileRead;
				return TGE::File::FileStatus::Ok;
			}
			else
			{
				thisptr->currentStatus = TGE::File::FileStatus::IOError;
				thisptr->capability = 0;
				return TGE::File::FileStatus::IOError;
			}
		}
	}
	return origOpen(thisptr, filename, openMode);
}

MBX_OVERRIDE_MEMBERFN(TGE::File::FileStatus, TGE::File::close, (TGE::File* thisptr), origClose)
{
	if (openPakFiles.find(thisptr) != openPakFiles.end()) {
		MemoryStream* ptr = openPakFiles[thisptr];
		delete ptr;
		openPakFiles.erase(thisptr); // Close existing file cause bruh

		thisptr->currentStatus = TGE::File::FileStatus::Closed;

		return TGE::File::FileStatus::Closed;
	}

	return origClose(thisptr);
}

MBX_OVERRIDE_MEMBERFN(TGE::File::FileStatus, TGE::File::read, (TGE::File* thisptr, U32 size, char* dst, U32* bytesRead), origRead)
{
	if (openPakFiles.find(thisptr) != openPakFiles.end()) {
		MemoryStream* str = openPakFiles[thisptr];

		if (str->tell() + size > str->length())
		{
			*bytesRead = (str->length() - str->tell());
			thisptr->currentStatus = TGE::File::FileStatus::EOS;
		}
		else
		{
			*bytesRead = size;
			thisptr->currentStatus = TGE::File::FileStatus::Ok;
		}
		uint8_t* buffer = str->getBuffer();
		memcpy(dst, (buffer + str->tell()), *bytesRead);
		try 
		{
			str->seek(str->tell() + *bytesRead);
		} 
		catch (...)
		{
			thisptr->currentStatus = TGE::File::FileStatus::EOS;
		}
		return thisptr->currentStatus;
	}

	return origRead(thisptr, size, dst, bytesRead);
}

MBX_OVERRIDE_MEMBERFN(TGE::File::FileStatus, TGE::File::setPosition, (TGE::File* thisptr, S32 position, bool absolutePos), origSetPosition)
{
	if (openPakFiles.find(thisptr) != openPakFiles.end()) {
		MemoryStream* str = openPakFiles[thisptr];
		if (absolutePos)
			str->seek(position);
		else
			str->seek(str->tell() + position);

		if (str->length() == str->tell())
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
	if (openPakFiles.find(thisptr) != openPakFiles.end()) {
		MemoryStream* str = openPakFiles[thisptr];
		return (U32)str->tell();
	}

	return origGetPosition(thisptr);
}

MBX_OVERRIDE_MEMBERFN(U32, TGE::File::getSize, (TGE::File* thisptr), origGetSize)
{
	if (openPakFiles.find(thisptr) != openPakFiles.end()) {
		MemoryStream* str = openPakFiles[thisptr];
		return (U32)str->length();
	}

	return origGetSize(thisptr);
}

MBX_OVERRIDE_DESTRUCTOR(TGE::File, (TGE::File* thisptr), origDtor)
{
	if (openPakFiles.find(thisptr) != openPakFiles.end()) {
		MemoryStream* str = openPakFiles[thisptr];
		delete str;
		openPakFiles.erase(thisptr); // Close existing file cause bruh
		thisptr->currentStatus = TGE::File::FileStatus::Closed;
	}

	origDtor(thisptr);
}

MBX_OVERRIDE_FN(bool, TGE::Platform::isSubDirectory, (const char* parent, const char* child), origIsSubdirectory)
{
	std::string workingDir = std::string(TGE::Platform::getWorkingDirectory());
	std::string relativeDir = std::string(parent);

	std::regex pattern(workingDir);
	relativeDir = std::regex_replace(relativeDir, pattern, "");

	std::string dirStr = relativeDir + child;

	for (auto& pak : loadedPackages)
	{
		for (auto& file : pak->entries)
		{
			if (file.filepath.find(dirStr) == 0)
			{
				return true;
			}
		}
	}
	return origIsSubdirectory(parent, child);
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
	if (strchr("/", leftEnd) || strchr("/", rightStart)) {
		return left + right;  // A separator already exists between the two paths
	}
	return left + "/" + right;
}

MBX_OVERRIDE_FN(bool, TGE::Platform::dumpPath, (const char* path, TGE::Vector<TGE::FileInfo>& fileVector), origDumpPath)
{
	bool ret = origDumpPath(path, fileVector);

	std::string workingDir = std::string(TGE::Platform::getWorkingDirectory());
	std::string relativeDir = std::string(path);
	std::regex pattern(workingDir);
	relativeDir = std::regex_replace(relativeDir, pattern, "");

	for (auto& pak : loadedPackages)
	{
		for (auto& file : pak->entries)
		{
			if (file.filepath.find(relativeDir) == 0)
			{
				TGE::FileInfo f;
				f.fileSize = file.uncompressedSize;

				std::string dpath = file.filepath.substr(0, file.filepath.rfind('/'));
				dpath = dpath.substr(relativeDir.length());
				dpath = combine(std::string(path), dpath);

				f.pFullPath = TGE::StringTable->insert(dpath.c_str(), true);
				std::string fname = getFileName(file.filepath);
				f.pFileName = TGE::StringTable->insert(fname.c_str(), false);
				fileVector.push_back(f);
			}
		}
	}

	return fileVector.size() != 0;
}

MBX_OVERRIDE_FN(bool, TGE::Platform::getFileTimes, (const char* path, TGE::FileTime* createTime, TGE::FileTime* modifyTime), origGetFileTimes)
{
	std::string workingDir = std::string(TGE::Platform::getWorkingDirectory()) + "/";
	std::string relativeDir = std::string(path);
	std::regex pattern(workingDir);
	relativeDir = std::regex_replace(relativeDir, pattern, "");

	for (auto& pak : loadedPackages)
	{
		for (auto& file : pak->entries)
		{
			if (file.filepath == relativeDir)
			{
				if (createTime)
				{
#ifdef _WIN32
					createTime->low = 0;
					createTime->high = 0;
#else
					*createTime = 0;
#endif
				}
				if (modifyTime)
				{
#ifdef _WIN32
					modifyTime->low = 0;
					modifyTime->high = 0;
#else
					*modifyTime = 0;
#endif
				}
				return true;
			}
		}
	}
	return origGetFileTimes(path, createTime, modifyTime);
}