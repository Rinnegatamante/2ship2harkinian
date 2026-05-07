#pragma once

#undef _DLL

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <zip.h>
#include "ExporterArchive.h"

class ExporterArchiveO2R : public ExporterArchive {
  public:
    ExporterArchiveO2R(const std::string& path, bool enableWriting);
    ~ExporterArchiveO2R();

    int CreateArchive(size_t fileCapacity) override;
    bool AddFile(const std::string& filePath, void* fileData, size_t fileSize) override;

    zip_t* mZip;
    bool Load(bool enableWriting) override;
    bool Unload() override;
};
