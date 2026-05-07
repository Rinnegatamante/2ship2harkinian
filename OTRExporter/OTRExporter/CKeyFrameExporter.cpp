#ifdef GAME_MM
#include "CKeyFrameExporter.h"
#include "DisplayListExporter.h"
#include "Globals.h"
#include "spdlog/spdlog.h"

// The Win32 API defines this function for its own uses.
#ifdef FindResource
#undef FindResource
#endif

void OTRExporter_CKeyFrameSkel::Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer) {
    ZKeyFrameSkel* skel = (ZKeyFrameSkel*)res;
    ZKeyFrameLimbList* list = skel->limbList.get();

    WriteHeader(res, outPath, writer, static_cast<uint32_t>(SOH::ResourceType::TSH_CKeyFrameSkel));

    writer->Write(skel->limbCount);
    writer->Write(skel->dListCount);
    writer->Write((uint8_t)skel->limbType);
    writer->Write(list->numLimbs);

    
    for (size_t i = 0; i < list->limbs.size(); i++) {
        ZKeyFrameLimb* limb = list->limbs[i];
        std::string name = "";
        if (GETSEGOFFSET(limb->dlist) != SEGMENTED_NULL) {
            bool found = Globals::Instance->GetSegmentedPtrName(GETSEGOFFSET(limb->dlist), res->parent, "", name,
                                                                res->parent->workerID);

            if (!found) {
                ZDisplayList* dl = (ZDisplayList*)res->parent->FindResource(limb->dlist & 0x00FFFFFF);
                if (dl != nullptr) {
                    name = dl->GetName();
                    found = true;
                }
            }
            if (found) {
                if (name.at(0) == '&')
                    name.erase(0, 1);

                name = OTRExporter_DisplayList::GetPathToRes(res, name);
            } else {
                spdlog::error("Texture not found: 0x{:X}", limb->dlist);
                //writer->Write("");
                name = "";
            }
        } 
        writer->Write(name);
        writer->Write(limb->numChildren);
        writer->Write(limb->flags);

        if (skel->limbType == ZKeyframeSkelType::Normal) {
            ZKeyFrameStandardLimb* stdLimb = (ZKeyFrameStandardLimb*)limb;

            writer->Write(stdLimb->translation.x);
            writer->Write(stdLimb->translation.y);
            writer->Write(stdLimb->translation.z);
        } else {
            ZKeyFrameFlexLimb* flexLimb = (ZKeyFrameFlexLimb*)limb;

            writer->Write(flexLimb->callbackIndex);
        }
    }
}

void OTRExporter_CKeyFrameAnim::Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer) {
    ZKeyFrameAnim* anim = (ZKeyFrameAnim*)res;

    WriteHeader(res, outPath, writer, static_cast<uint32_t>(SOH::ResourceType::TSH_CKeyFrameAnim));

    writer->Write((uint8_t)anim->skel->limbType);
    if (anim->skel->limbType == ZKeyframeSkelType::Normal) {
        writer->Write((uint32_t)anim->bitFlags.size());
        for (const auto b : anim->bitFlags) {
            writer->Write(b);
        }
    } else {
        writer->Write((uint32_t)anim->bitFlagsFlex.size());
        for (const auto b : anim->bitFlagsFlex) {
            writer->Write(b);
        }
    }

    writer->Write((uint32_t)anim->keyFrames.size());
    for (const auto k : anim->keyFrames) {
        writer->Write(k.frame);
        writer->Write(k.value);
        writer->Write(k.velocity);
    }

    writer->Write((uint32_t)anim->kfNums.size());
    for (const auto kf : anim->kfNums) {
        writer->Write(kf);
    }

    writer->Write((uint32_t)anim->presetValues.size());
    for (const auto pv : anim->presetValues) {
        writer->Write(pv);
    }

    writer->Write(anim->unk_10);
    writer->Write(anim->duration);
}
#endif
