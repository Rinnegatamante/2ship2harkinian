#pragma once
#ifdef INCLUDE_MPQ_SUPPORT
#undef _DLL

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <StormLib.h>
#include "ExporterArchive.h"

class ExporterArchiveOtr : public ExporterArchive {
  public:
    ExporterArchiveOtr(const std::string& path, bool enableWriting);
    ~ExporterArchiveOtr();

    int CreateArchive(size_t fileCapacity) override;
    bool AddFile(const std::string& filePath, void* fileData, size_t fileSize) override;

    bool Load(bool enableWriting) override;
    bool Unload() override;

  private:
    std::unordered_map<uint64_t, std::string> mHashes;
    std::vector<std::string> mAddedFiles;
    HANDLE mMpq;
};
#endif