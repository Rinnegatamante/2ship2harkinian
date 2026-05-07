#pragma once
#include "ZResource.h"
#include "ZArray.h"
#include "stdint.h"
#include <Utils/BinaryWriter.h>
#include <libultraship/bridge.h>
#include "VersionInfo.h"
#ifdef GAME_MM
#include "../../mm/2s2h/resource/type/2shResourceType.h"
#elif GAME_OOT
#include "../../soh/soh/resource/type/SohResourceType.h"
#endif
class OTRExporter : public ZResourceExporter
{
protected:
	static void WriteHeader(ZResource* res, const fs::path& outPath, BinaryWriter* writer, uint32_t resType, int32_t resVersion = 0);
};
