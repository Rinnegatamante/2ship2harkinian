#pragma once

#include <libultraship/bridge.h>
#include "ExporterArchive.h"

extern std::shared_ptr<ExporterArchive> archive;
extern std::map<std::string, std::vector<char>> files;

void AddFile(std::string fName, std::vector<char> data);
