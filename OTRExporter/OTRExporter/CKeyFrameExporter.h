#pragma once
#ifdef GAME_MM
#include "ZResource.h"
#include "ZCKeyFrame.h"
#include "ZCkeyFrameAnim.h"
#include "Exporter.h"
#include <Utils/BinaryWriter.h>

class OTRExporter_CKeyFrameSkel : public OTRExporter 
{
public:
    virtual void Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer) override;
};

class OTRExporter_CKeyFrameAnim : public OTRExporter {
  public:
    virtual void Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer) override;
};
#endif
