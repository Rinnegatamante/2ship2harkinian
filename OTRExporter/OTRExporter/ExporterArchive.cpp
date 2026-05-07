#include "ExporterArchive.h"
#include "Utils/StringHelper.h"
#include <ship/utils/StrHash64.h>
#include <filesystem>

ExporterArchive::ExporterArchive(const std::string& path, bool enableWriting) : mPath(path) {
}

ExporterArchive::~ExporterArchive() {
}

