#pragma once

#undef _DLL

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <StormLib.h>

class ExporterArchive : public std::enable_shared_from_this<ExporterArchive> {
  public:
    ExporterArchive() {}
    ExporterArchive(const std::string& path, bool enableWriting);
    ~ExporterArchive();

    virtual int CreateArchive(size_t fileCapacity) = 0;
    virtual bool AddFile(const std::string& filePath, void* fileData, size_t fileSize) = 0;

    std::string mPath;
    std::mutex mMutex;  

    virtual bool Load(bool enableWriting) = 0;
    virtual bool Unload() = 0;
};
