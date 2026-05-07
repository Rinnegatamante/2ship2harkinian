#include "PathExporter.h"
#include "../ZAPD/ZFile.h"
#include "Globals.h"

void OTRExporter_Path::Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer)
{
	ZPath* path = (ZPath*)res;

	WriteHeader(res, outPath, writer, static_cast<uint32_t>(SOH::ResourceType::SOH_Path));
	
	writer->Write((uint32_t)path->pathways.size());

	for (size_t k = 0; k < path->pathways.size(); k++)
	{
		writer->Write((uint32_t)path->pathways[k].points.size());
		if (Globals::Instance->game == ZGame::MM_RETAIL) {
			writer->Write(path->pathways[k].unk1);
			writer->Write(path->pathways[k].unk2);
		}
		for (size_t i = 0; i < path->pathways[k].points.size(); i++)
		{
			writer->Write(path->pathways[k].points[i].scalars[0].scalarData.s16);
			writer->Write(path->pathways[k].points[i].scalars[1].scalarData.s16);
			writer->Write(path->pathways[k].points[i].scalars[2].scalarData.s16);
		}
	}
}