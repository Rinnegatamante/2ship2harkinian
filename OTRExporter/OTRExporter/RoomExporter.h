#pragma once
#define NO_GDI
#define WIN32_LEAN_AND_MEAN
#include "ZResource.h"
#include "Exporter.h"
#include "ZRoom/ZRoom.h"

class RoomShapeDListsEntry;

class OTRExporter_Room : public OTRExporter
{
public:
	void WritePolyDList(BinaryWriter* writer, ZRoom* room, RoomShapeDListsEntry* dlist);
	virtual void Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer) override;
};