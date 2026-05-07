#include "SkeletonLimbExporter.h"
#include "DisplayListExporter.h"
#include <libultraship/bridge.h>
#include <Globals.h>

void OTRExporter_SkeletonLimb::Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer)
{
	ZLimb* limb = (ZLimb*)res;

	WriteHeader(res, outPath, writer, static_cast<uint32_t>(SOH::ResourceType::SOH_SkeletonLimb));

	writer->Write((uint8_t)limb->type);
	writer->Write((uint8_t)limb->skinSegmentType);

	if (limb->skinSegmentType == ZLimbSkinType::SkinType_Normal && limb->type == ZLimbType::Skin)
	{
		auto childDecl = limb->parent->GetDeclaration(GETSEGOFFSET(limb->skinSegment));

		if (childDecl != nullptr)
			writer->Write(OTRExporter_DisplayList::GetPathToRes(limb, childDecl->declName));
		else
			writer->Write("");
	}
	else
	{
		writer->Write("");
	}

	writer->Write((uint16_t)limb->segmentStruct.totalVtxCount);
	writer->Write((uint32_t)limb->segmentStruct.limbModifications_arr.size());
	
	for (auto item : limb->segmentStruct.limbModifications_arr)
	{
		writer->Write(item.unk_4);

		writer->Write((uint32_t)item.skinVertices_arr.size());

		for (auto item2 : item.skinVertices_arr)
		{
			writer->Write(item2.index);
			writer->Write(item2.s);
			writer->Write(item2.t);
			writer->Write(item2.normX);
			writer->Write(item2.normY);
			writer->Write(item2.normZ);
			writer->Write(item2.alpha);
		}

		writer->Write((uint32_t)item.limbTransformations_arr.size());

		for (auto item2 : item.limbTransformations_arr)
		{
			writer->Write(item2.limbIndex);
			writer->Write(item2.x);
			writer->Write(item2.y);
			writer->Write(item2.z);
			writer->Write(item2.scale);
		}
	}

	if (limb->segmentStruct.dlist != SEGMENTED_NULL)
	{
		auto skinGfxDecl = limb->parent->GetDeclaration(GETSEGOFFSET(limb->segmentStruct.dlist));

		if (skinGfxDecl != nullptr)
		{
			writer->Write(OTRExporter_DisplayList::GetPathToRes(limb, skinGfxDecl->declName));
		}
		else
		{
			writer->Write("");
		}
	}
	else
	{
		writer->Write("");
	}

	writer->Write(limb->legTransX);
	writer->Write(limb->legTransY);
	writer->Write(limb->legTransZ);
	writer->Write(limb->rotX);
	writer->Write(limb->rotY);
	writer->Write(limb->rotZ);

	if (limb->childPtr != 0)
	{
		std::string name;
		bool foundDecl = Globals::Instance->GetSegmentedPtrName(limb->childPtr, limb->parent, "", name, res->parent->workerID);
		if (foundDecl)
		{
			if (name.at(0) == '&')
				name.erase(0, 1);

			writer->Write(OTRExporter_DisplayList::GetPathToRes(limb, name));
		}
		else
		{
			writer->Write("");
		}
	}
	else
	{
		writer->Write("");
	}

	if (limb->siblingPtr != 0)
	{
		std::string name;
		bool foundDecl = Globals::Instance->GetSegmentedPtrName(limb->siblingPtr, limb->parent, "", name, res->parent->workerID);
		if (foundDecl)
		{
			if (name.at(0) == '&')
				name.erase(0, 1);

			writer->Write(OTRExporter_DisplayList::GetPathToRes(limb, name));
		}
		else
		{
			writer->Write("");
		}
	}
	else
	{
		writer->Write("");
	}

	if (limb->dListPtr != 0)
	{
		std::string name;
		bool foundDecl = Globals::Instance->GetSegmentedPtrName(limb->dListPtr, limb->parent, "", name, res->parent->workerID);
		if (foundDecl)
		{
			if (name.at(0) == '&')
				name.erase(0, 1);

			ZFile* assocFile = Globals::Instance->GetSegment(GETSEGNUM(limb->dListPtr), res->parent->workerID);

			writer->Write(OTRExporter_DisplayList::GetPathToRes(assocFile->resources[0], name));
		}
		else
		{
			writer->Write("");
		}
	}
	else
	{
		writer->Write("");
	}

	if (limb->dList2Ptr != 0)
	{
		std::string name;
		bool foundDecl = Globals::Instance->GetSegmentedPtrName(limb->dList2Ptr, limb->parent, "", name, res->parent->workerID);
		if (foundDecl)
		{
			if (name.at(0) == '&')
				name.erase(0, 1);

			ZFile* assocFile = Globals::Instance->GetSegment(GETSEGNUM(limb->dList2Ptr), res->parent->workerID);

			writer->Write(OTRExporter_DisplayList::GetPathToRes(assocFile->resources[0], name));
		}
		else
		{
			writer->Write("");
		}
	}
	else
	{
		writer->Write("");
	}

	writer->Write(limb->transX);
	writer->Write(limb->transY);
	writer->Write(limb->transZ);

	writer->Write(limb->childIndex);
	writer->Write(limb->siblingIndex);
}
