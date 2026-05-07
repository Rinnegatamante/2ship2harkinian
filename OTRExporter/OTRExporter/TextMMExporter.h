#pragma once
#ifdef GAME_MM
#include "ZResource.h"
#include "ZTextMM.h"
#include "Exporter.h"
#include <Utils/BinaryWriter.h>

class OTRExporter_TextMM : public OTRExporter
{
public:
	virtual void Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer) override;
};
#endif
