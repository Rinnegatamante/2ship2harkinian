#include "AnimationExporter.h"
#ifdef GAME_MM
#include <resource/type/Animation.h>
#elif GAME_OOT
#include "../../soh/soh/resource/type/Animation.h"
#endif

#include <Globals.h>
#include "DisplayListExporter.h"
#undef FindResource


void OTRExporter_Animation::Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer)
{
	ZAnimation* anim = (ZAnimation*)res;

	WriteHeader(res, outPath, writer, static_cast<uint32_t>(SOH::ResourceType::SOH_Animation));

	ZNormalAnimation* normalAnim = dynamic_cast<ZNormalAnimation*>(anim);
	ZCurveAnimation* curveAnim = dynamic_cast<ZCurveAnimation*>(anim);
	ZLinkAnimation* linkAnim = dynamic_cast<ZLinkAnimation*>(anim);
	if (linkAnim != nullptr)
	{
		writer->Write((uint32_t)SOH::AnimationType::Link);
		writer->Write((uint16_t)linkAnim->frameCount);
		std::string name;
		bool found = Globals::Instance->GetSegmentedPtrName((linkAnim->segmentAddress), res->parent, "", name, res->parent->workerID);
		if (found)
		{
			if (name.at(0) == '&')
				name.erase(0, 1);

			writer->Write(StringHelper::Sprintf("__OTR__misc/link_animetion/%s", name.c_str()));
		}
		else
		{
			writer->Write("");
		}
		//writer->Write((uint32_t)linkAnim->segmentAddress);
	}
	else if (curveAnim != nullptr)
	{
		writer->Write((uint32_t)SOH::AnimationType::Curve);
		writer->Write((uint16_t)curveAnim->frameCount);

		writer->Write((uint32_t)curveAnim->refIndexArr.size());

		for (auto val : curveAnim->refIndexArr)
			writer->Write(val);

		writer->Write((uint32_t)curveAnim->transformDataArr.size());

		for (auto val : curveAnim->transformDataArr)
		{
			writer->Write(val.unk_00);
			writer->Write(val.unk_02);
			writer->Write(val.unk_04);
			writer->Write(val.unk_06);
			writer->Write(val.unk_08);
		}

		writer->Write((uint32_t)curveAnim->copyValuesArr.size());

		for (auto val : curveAnim->copyValuesArr)
			writer->Write(val);
	}
	else if (normalAnim != nullptr)
	{
		writer->Write((uint32_t)SOH::AnimationType::Normal);
		writer->Write((uint16_t)normalAnim->frameCount);

		writer->Write((uint32_t)normalAnim->rotationValues.size());

		for (size_t i = 0; i < normalAnim->rotationValues.size(); i++)
			writer->Write(normalAnim->rotationValues[i]);

		writer->Write((uint32_t)normalAnim->rotationIndices.size());

		for (size_t i = 0; i < normalAnim->rotationIndices.size(); i++)
		{
			writer->Write(normalAnim->rotationIndices[i].x);
			writer->Write(normalAnim->rotationIndices[i].y);
			writer->Write(normalAnim->rotationIndices[i].z);
		}

		writer->Write(normalAnim->limit);
	}
	else
	{
		writer->Write((uint32_t)SOH::AnimationType::Legacy);
	}
}
