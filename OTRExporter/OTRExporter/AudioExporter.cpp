#include "AudioExporter.h"
#include "Main.h"
#include <Utils/MemoryStream.h>
#include <Globals.h>
#include <Utils/DiskFile.h>
#include "DisplayListExporter.h"

const char* OTRExporter_Audio::GetMediumStr(uint8_t medium) {
    switch (medium) {
        case 0:
            return "Ram";
        case 1:
            return "Unk";
        case 2:
            return "Cart";
        case 3:
            return "Disk";
        case 5:
            return "RamUnloaded";
        default:
            return "ERROR";
    }
}

const char* OTRExporter_Audio::GetCachePolicyStr(uint8_t policy) {
    switch (policy) {
        case 0:
            return "Temporary";
        case 1:
            return "Persistent";
        case 2:
            return "Either";
        case 3:
            return "Permanent";
        default:
            return "ERROR";
    }
}

const char* OTRExporter_Audio::GetCodecStr(uint8_t codec) {
    switch (codec) {
        case 0:
            return "ADPCM";
        case 1:
            return "S8";
        case 2:
            return "S16MEM";
        case 3:
            return "ADPCMSMALL";
        case 4:
            return "REVERB";
        case 5:
            return "S16";
        case 6:
            return "UNK6";
        case 7:
            return "UNK7";
        default:
            return "ERROR";
    }
}

std::string OTRExporter_Audio::GetSampleEntryReference(ZAudio* audio, SampleEntry* entry)
{
    if (entry != nullptr)
    {
        if (audio->sampleOffsets[entry->bankId].contains(entry->sampleDataOffset) &&
            audio->sampleOffsets[entry->bankId][entry->sampleDataOffset] != "") {
                return (StringHelper::Sprintf("audio/samples/%s_META",
                                          audio->sampleOffsets[entry->bankId][entry->sampleDataOffset].c_str()));
        }
        else
            return StringHelper::Sprintf("audio/samples/sample_%d_%08X_META", entry->bankId, entry->sampleDataOffset);
    }
    else
        return("");
}

void OTRExporter_Audio::WriteSampleEntry(SampleEntry* entry, BinaryWriter* writer)
{
    WriteHeader(nullptr, "", writer, static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSample), 2);

    writer->Write(entry->codec);
    writer->Write(entry->medium);
    writer->Write(entry->unk_bit26);
    writer->Write(entry->unk_bit25);

    writer->Write((uint32_t)entry->data.size());
    writer->Write((char*)entry->data.data(), entry->data.size());

    writer->Write((uint32_t)(entry->loop.start));
    writer->Write((uint32_t)(entry->loop.end));
    writer->Write((uint32_t)(entry->loop.count));
    writer->Write((uint32_t)entry->loop.states.size());

    for (size_t i = 0; i < entry->loop.states.size(); i++)
        writer->Write((entry->loop.states[i]));

    writer->Write((uint32_t)(entry->book.order));
    writer->Write((uint32_t)(entry->book.npredictors));
    writer->Write((uint32_t)entry->book.books.size());

    for (size_t i = 0; i < entry->book.books.size(); i++)
        writer->Write((entry->book.books[i]));
}

void OTRExporter_Audio::WriteSampleEntry(SampleEntry* entry, tinyxml2::XMLElement* xmlDoc) {
    tinyxml2::XMLElement* sEntry = xmlDoc;

    sEntry->SetAttribute("Codec", GetCodecStr(entry->codec));
    sEntry->SetAttribute("Medium", GetMediumStr(entry->medium));
    sEntry->SetAttribute("bit26", entry->unk_bit26);
    sEntry->SetAttribute("Relocated", entry->unk_bit25);
    
    tinyxml2::XMLElement* loopRoot = sEntry->InsertNewChildElement("ADPCMLoop");
    loopRoot->SetAttribute("Start", entry->loop.start);
    loopRoot->SetAttribute("End", entry->loop.end);
    loopRoot->SetAttribute("Count", (int)entry->loop.count); // Cast to int to -1 shows as -1.

    for (size_t i = 0; i < entry->loop.states.size(); i++) {
        tinyxml2::XMLElement* loop = loopRoot->InsertNewChildElement("Predictor");
        loop->SetAttribute("State", entry->loop.states[i]);
        loopRoot->InsertEndChild(loop);
    }
    sEntry->InsertEndChild(loopRoot);

    tinyxml2::XMLElement* bookRoot = sEntry->InsertNewChildElement("ADPCMBook");
    bookRoot->SetAttribute("Order", entry->book.order);
    bookRoot->SetAttribute("Npredictors", entry->book.npredictors);

    for (size_t i = 0; i < entry->book.books.size(); i++) {
        tinyxml2::XMLElement* book = bookRoot->InsertNewChildElement("Book");
        book->SetAttribute("Page", entry->book.books[i]);
        bookRoot->InsertEndChild(book);
    }
    sEntry->InsertEndChild(bookRoot);

}

void OTRExporter_Audio::WriteSoundFontEntry(ZAudio* audio, SoundFontEntry* entry, BinaryWriter* writer)
{
    writer->Write((uint8_t)(entry != nullptr ? 1 : 0));

    if (entry != nullptr)
    {
        // This second byte isn't used but is needed to maintain compatibility with the V2 format.
        writer->Write((uint8_t)(entry != nullptr ? 1 : 0));
        writer->Write(GetSampleEntryReference(audio, entry->sampleEntry));
        writer->Write(entry->tuning);
    }
}

void OTRExporter_Audio::WriteSoundFontEntry(ZAudio* audio, SoundFontEntry* entry, tinyxml2::XMLElement* xmlDoc,
                                            const char* name) {
    tinyxml2::XMLElement* sfEntry = xmlDoc->InsertNewChildElement(name);

    if (entry != nullptr)
    {
        sfEntry->SetAttribute("SampleRef", GetSampleEntryReference(audio, entry->sampleEntry).c_str());
        sfEntry->SetAttribute("Tuning", entry->tuning);
    }
    xmlDoc->InsertEndChild(sfEntry);
}

void OTRExporter_Audio::WriteEnvData(std::vector<AdsrEnvelope*> envelopes, BinaryWriter* writer)
{
    writer->Write((uint32_t)envelopes.size());

    for (auto env : envelopes)
    {
        writer->Write(env->delay);
        writer->Write(env->arg);
    }
}

void OTRExporter_Audio::WriteEnvData(std::vector<AdsrEnvelope*> envelopes, tinyxml2::XMLElement* xmlDoc) {
    tinyxml2::XMLElement* envs = xmlDoc->InsertNewChildElement("Envelopes");
    envs->SetAttribute("Count", (uint32_t)envelopes.size());

    for (auto e : envelopes) {
        tinyxml2::XMLElement* env = envs->InsertNewChildElement("Envelope");
        env->SetAttribute("Delay", e->delay);
        env->SetAttribute("Arg", e->arg);
    }
    xmlDoc->InsertEndChild(envs);
}


void OTRExporter_Audio::WriteSoundFontTableXML(ZAudio* audio) {
    for (size_t i = 0; i < audio->soundFontTable.size(); i++) {
        tinyxml2::XMLDocument soundFont;
        tinyxml2::XMLElement* root = soundFont.NewElement("SoundFont");
        root->SetAttribute("Version", 0);
        root->SetAttribute("Num", (uint32_t)i);
        root->SetAttribute("Medium", GetMediumStr(audio->soundFontTable[i].medium));
        root->SetAttribute("CachePolicy", GetCachePolicyStr(audio->soundFontTable[i].cachePolicy));
        root->SetAttribute("Data1", audio->soundFontTable[i].data1);
        root->SetAttribute("Data2", audio->soundFontTable[i].data2);
        root->SetAttribute("Data3", audio->soundFontTable[i].data3);
        soundFont.InsertFirstChild(root);
        
        tinyxml2::XMLElement* drums = root->InsertNewChildElement("Drums");
        drums->SetAttribute("Count", (uint32_t)audio->soundFontTable[i].drums.size());

        for (const auto& d : audio->soundFontTable[i].drums) {
            tinyxml2::XMLElement* drum = drums->InsertNewChildElement("Drum");
            drum->SetAttribute("ReleaseRate", d.releaseRate);
            drum->SetAttribute("Pan", d.pan);
            drum->SetAttribute("Loaded", d.loaded);
            drum->SetAttribute("SampleRef", GetSampleEntryReference(audio, d.sample).c_str());
            drum->SetAttribute("Tuning", d.tuning);

            WriteEnvData(d.env, drum);
            drums->InsertEndChild(drum);
        }
        root->InsertEndChild(drums);

        tinyxml2::XMLElement* instruments = root->InsertNewChildElement("Instruments");
        instruments->SetAttribute("Count", (uint32_t)audio->soundFontTable[i].instruments.size());

        //for (size_t k = 0; k < audio->soundFontTable[i].instruments.size(); k++) {
        for (const auto i : audio->soundFontTable[i].instruments) {
            tinyxml2::XMLElement* instrument = instruments->InsertNewChildElement("Instrument");

            instrument->SetAttribute("IsValid", i.isValidInstrument);
            instrument->SetAttribute("Loaded", i.loaded);
            instrument->SetAttribute("NormalRangeLo", i.normalRangeLo);
            instrument->SetAttribute("NormalRangeHi", i.normalRangeHi);
            instrument->SetAttribute("ReleaseRate", i.releaseRate);

            WriteEnvData(i.env, instrument);

            WriteSoundFontEntry(audio, i.lowNotesSound, instrument, "LowNotesSound");
            WriteSoundFontEntry(audio, i.normalNotesSound, instrument, "NormalNotesSound");
            WriteSoundFontEntry(audio, i.highNotesSound, instrument, "HighNotesSound");
        }
        root->InsertEndChild(instruments);

        tinyxml2::XMLElement* sfxTbl = root->InsertNewChildElement("SfxTable");
        sfxTbl->SetAttribute("Count", (uint32_t)audio->soundFontTable[i].soundEffects.size());

        for (const auto s : audio->soundFontTable[i].soundEffects) {
            WriteSoundFontEntry(audio, s, sfxTbl, "Sfx");
        }
        root->InsertEndChild(sfxTbl);
        soundFont.InsertEndChild(root);
        
        tinyxml2::XMLPrinter printer;
        soundFont.Accept(&printer);
        
        std::string fName = OTRExporter_DisplayList::GetPathToRes(
            (ZResource*)(audio), StringHelper::Sprintf("fonts/%s", audio->soundFontNames[i].c_str()));
        std::vector<char> xmlData((printer.CStr()), printer.CStr() + printer.CStrSize() - 1);
        AddFile(fName, xmlData);
    }
}

void OTRExporter_Audio::WriteSoundFontTableBinary(ZAudio* audio) {
    for (size_t i = 0; i < audio->soundFontTable.size(); i++) {
        MemoryStream* fntStream = new MemoryStream();
        BinaryWriter fntWriter = BinaryWriter(fntStream);

        WriteHeader(nullptr, "", &fntWriter, static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSoundFont), 2);

        fntWriter.Write((uint32_t)i);
        fntWriter.Write(audio->soundFontTable[i].medium);
        fntWriter.Write(audio->soundFontTable[i].cachePolicy);
        fntWriter.Write(audio->soundFontTable[i].data1);
        fntWriter.Write(audio->soundFontTable[i].data2);
        fntWriter.Write(audio->soundFontTable[i].data3);

        fntWriter.Write((uint32_t)audio->soundFontTable[i].drums.size());
        fntWriter.Write((uint32_t)audio->soundFontTable[i].instruments.size());
        fntWriter.Write((uint32_t)audio->soundFontTable[i].soundEffects.size());

        for (size_t k = 0; k < audio->soundFontTable[i].drums.size(); k++) {
            fntWriter.Write(audio->soundFontTable[i].drums[k].releaseRate);
            fntWriter.Write(audio->soundFontTable[i].drums[k].pan);
            fntWriter.Write(audio->soundFontTable[i].drums[k].loaded);

            WriteEnvData(audio->soundFontTable[i].drums[k].env, &fntWriter);
            fntWriter.Write((uint8_t)(audio->soundFontTable[i].drums[k].sample != nullptr ? 1 : 0));

            fntWriter.Write(GetSampleEntryReference(audio, audio->soundFontTable[i].drums[k].sample));
            fntWriter.Write(audio->soundFontTable[i].drums[k].tuning);
        }

        for (size_t k = 0; k < audio->soundFontTable[i].instruments.size(); k++) {
            fntWriter.Write((uint8_t)audio->soundFontTable[i].instruments[k].isValidInstrument);

            fntWriter.Write(audio->soundFontTable[i].instruments[k].loaded);
            fntWriter.Write(audio->soundFontTable[i].instruments[k].normalRangeLo);
            fntWriter.Write(audio->soundFontTable[i].instruments[k].normalRangeHi);
            fntWriter.Write(audio->soundFontTable[i].instruments[k].releaseRate);

            WriteEnvData(audio->soundFontTable[i].instruments[k].env, &fntWriter);

            WriteSoundFontEntry(audio, audio->soundFontTable[i].instruments[k].lowNotesSound, &fntWriter);
            WriteSoundFontEntry(audio, audio->soundFontTable[i].instruments[k].normalNotesSound, &fntWriter);
            WriteSoundFontEntry(audio, audio->soundFontTable[i].instruments[k].highNotesSound, &fntWriter);
        }

        for (size_t k = 0; k < audio->soundFontTable[i].soundEffects.size(); k++) {
            WriteSoundFontEntry(audio, audio->soundFontTable[i].soundEffects[k], &fntWriter);
        }

        std::string fName = OTRExporter_DisplayList::GetPathToRes(
            (ZResource*)(audio), StringHelper::Sprintf("fonts/%s", audio->soundFontNames[i].c_str()));
        AddFile(fName, fntStream->ToVector());
    }
}

void OTRExporter_Audio::WriteSequenceXML(ZAudio* audio) {
    for (size_t i = 0; i < audio->sequences.size(); i++) {
        MemoryStream* seqStream = new MemoryStream();
        BinaryWriter seqWriter = BinaryWriter(seqStream);
        auto& seq = audio->sequences[i];

        tinyxml2::XMLDocument sequence;
        tinyxml2::XMLElement* root = sequence.NewElement("Sequence");
        root->SetAttribute("Index", (uint32_t)i);
        root->SetAttribute("Medium", GetMediumStr(audio->sequenceTable[i].medium));
        root->SetAttribute("CachePolicy", GetCachePolicyStr(audio->sequenceTable[i].cachePolicy));
        root->SetAttribute("Size", (uint32_t)seq.size());

        std::string seqName = OTRExporter_DisplayList::GetPathToRes(
            (ZResource*)(audio), StringHelper::Sprintf("sequencedata/%s_RAW", audio->seqNames[i].c_str()));
        root->SetAttribute("Path", seqName.c_str());


        tinyxml2::XMLElement* fontIndicies = root->InsertNewChildElement("FontIndicies");
        for (size_t k = 0; k < audio->fontIndices[i].size(); k++) {
            tinyxml2::XMLElement* fontIndex = fontIndicies->InsertNewChildElement("FontIndex");
            fontIndex->SetAttribute("FontIdx", audio->fontIndices[i][k]);

        }
        root->InsertEndChild(fontIndicies);
        root->InsertEndChild(root);
        sequence.InsertEndChild(root);
        seqWriter.Write(seq.data(), seq.size());
        AddFile(seqName, seqStream->ToVector());

        tinyxml2::XMLPrinter printer;
        sequence.Accept(&printer);
        

        std::string seqMetaName = OTRExporter_DisplayList::GetPathToRes(
            (ZResource*)(audio), StringHelper::Sprintf("sequences/%s_META", audio->seqNames[i].c_str()));
        std::vector<char> xmlData((printer.CStr()), printer.CStr() + printer.CStrSize() - 1);
        AddFile(seqMetaName, xmlData);
    }
}

void OTRExporter_Audio::WriteSequenceBinary(ZAudio* audio) {
    for (size_t i = 0; i < audio->sequences.size(); i++) {
        auto& seq = audio->sequences[i];

        MemoryStream* seqStream = new MemoryStream();
        BinaryWriter seqWriter = BinaryWriter(seqStream);

        WriteHeader(nullptr, "", &seqWriter, static_cast<uint32_t>(SOH::ResourceType::SOH_AudioSequence), 2);

        seqWriter.Write((uint32_t)seq.size());
        seqWriter.Write(seq.data(), seq.size());
        seqWriter.Write((uint8_t)i);
        seqWriter.Write((uint8_t)audio->sequenceTable[i].medium);
        seqWriter.Write((uint8_t)audio->sequenceTable[i].cachePolicy);
        seqWriter.Write((uint32_t)audio->fontIndices[i].size());

        for (size_t k = 0; k < audio->fontIndices[i].size(); k++)
            seqWriter.Write((uint8_t)audio->fontIndices[i][k]);

        std::string fName = OTRExporter_DisplayList::GetPathToRes(
            (ZResource*)(audio), StringHelper::Sprintf("sequences/%s", audio->seqNames[i].c_str()));
        AddFile(fName, seqStream->ToVector());
    }
}

std::string OTRExporter_Audio::GetSampleEntryStr(ZAudio* audio, SampleEntry* entry) {
    std::string basePath = "";

    if (audio->sampleOffsets[entry->bankId].contains(entry->sampleDataOffset) &&
        audio->sampleOffsets[entry->bankId][entry->sampleDataOffset] != "") {
        basePath = StringHelper::Sprintf(
            "samples/%s", audio->sampleOffsets[entry->bankId][entry->sampleDataOffset].c_str());
    } else
        basePath = StringHelper::Sprintf("samples/sample_%d_%08X", entry->bankId, entry->sampleDataOffset);
    return basePath;
}

std::string OTRExporter_Audio::GetSampleDataStr(ZAudio* audio, SampleEntry* entry) {
    std::string basePath = "";

    if (audio->sampleOffsets[entry->bankId].contains(entry->sampleDataOffset) && 
        audio->sampleOffsets[entry->bankId][entry->sampleDataOffset] != "") {
            basePath = StringHelper::Sprintf(
                "samples/%s_RAW",
                audio->sampleOffsets[entry->bankId][entry->sampleDataOffset].c_str());
    } else
        basePath = StringHelper::Sprintf("samples/sample_%d_%08X_RAW", entry->bankId, entry->sampleDataOffset);
    return basePath;
}

void OTRExporter_Audio::WriteSampleBinary(ZAudio* audio) {
    ZResource* res = audio;

    for (const auto& pair : audio->samples) {
        MemoryStream* sampleStream = new MemoryStream();
        BinaryWriter sampleWriter = BinaryWriter(sampleStream);

        WriteSampleEntry(pair.second, &sampleWriter);

        std::string basePath = GetSampleEntryStr(audio, pair.second);

        std::string fName = OTRExporter_DisplayList::GetPathToRes(res, basePath);
        fName += "_META";
        AddFile(fName, sampleStream->ToVector());
    }
}

void OTRExporter_Audio::WriteSampleXML(ZAudio* audio) {
    ZResource* res = audio;

    for (const auto& pair : audio->samples) {
        tinyxml2::XMLDocument sample;
        tinyxml2::XMLElement* root = sample.NewElement("Sample");
        root->SetAttribute("Version", 0);

        WriteSampleEntry(pair.second, root);

        // There is no overload for size_t. MSVC and GCC are fine with `size` being cast
        // to size_t and passed in, but apple clang is not.
        root->SetAttribute("Size", (uint64_t)pair.second->data.size());
        sample.InsertEndChild(root);
        
        std::string sampleDataPath = GetSampleDataStr(audio, pair.second);
        sampleDataPath = OTRExporter_DisplayList::GetPathToRes(res, sampleDataPath);

        root->SetAttribute("Path", sampleDataPath.c_str());

        std::string basePath = GetSampleEntryStr(audio, pair.second);
        std::string fName = OTRExporter_DisplayList::GetPathToRes(res, basePath);

        fName += "_META";

        tinyxml2::XMLPrinter printer;
        sample.Accept(&printer);
        std::vector<char> xmlData((printer.CStr()), printer.CStr() + printer.CStrSize() - 1);
        AddFile(fName, xmlData);

        MemoryStream* stream = new MemoryStream();
        BinaryWriter sampleDataWriter = BinaryWriter(stream);
        
        sampleDataWriter.Write((char*)pair.second->data.data(), pair.second->data.size());
        AddFile(sampleDataPath, stream->ToVector());
    }
}

void OTRExporter_Audio::Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer)
{
    ZAudio* audio = (ZAudio*)res;

    WriteHeader(res, outPath, writer, static_cast<uint32_t>(SOH::ResourceType::SOH_Audio), 2);

    // Write Samples as individual files
    if (Globals::Instance->xmlExtractModes & (1 << (int)XMLModeShift::Sample))
        WriteSampleXML(audio);
    else
        WriteSampleBinary(audio);


    // Write the soundfont table
    if (Globals::Instance->xmlExtractModes & (1 << (int)XMLModeShift::SoundFont))
        WriteSoundFontTableXML(audio);
    else
        WriteSoundFontTableBinary(audio);
    
    // Write Sequences
    if (Globals::Instance->xmlExtractModes & (1 << (int)XMLModeShift::Sequence))
        WriteSequenceXML(audio);
    else
        WriteSequenceBinary(audio);
}
