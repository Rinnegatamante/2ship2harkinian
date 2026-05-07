#include "ZAudio.h"

#include "Globals.h"
#include "Utils/BitConverter.h"
#include <Utils/DiskFile.h>
#include "Utils/Path.h"
#include "Utils/StringHelper.h"
#include "ZFile.h"

REGISTER_ZFILENODE(Audio, ZAudio);

ZAudio::ZAudio(ZFile* nParent) : ZResource(nParent)
{
	RegisterRequiredAttribute("SoundFontTableOffset");
	RegisterRequiredAttribute("SequenceTableOffset");
	RegisterRequiredAttribute("SampleBankTableOffset");
	RegisterRequiredAttribute("SequenceFontTableOffset");
}

void ZAudio::ParseXML(tinyxml2::XMLElement* reader)
{
	ZResource::ParseXML(reader);

	auto t = reader->Name();

	auto child = reader->FirstChildElement();

	while (child != nullptr)
	{
		if (strcmp(child->Value(), "Sequences") == 0)
		{
			auto seqChild = child->FirstChildElement();

			while (seqChild != nullptr)
			{
				if (strcmp(seqChild->Value(), "Sequence") == 0)
				{
					seqNames.push_back(seqChild->Attribute("Name"));
				}

				seqChild = seqChild->NextSiblingElement();
			}
		}

		if (strcmp(child->Value(), "Samples") == 0)
		{
			int bankId = child->IntAttribute("Bank", 0);
			auto sampChild = child->FirstChildElement();

			while (sampChild != nullptr)
			{
				if (strcmp(sampChild->Value(), "Sample") == 0)
				{
					uint32_t atOffset = sampChild->UnsignedAttribute("Offset");

					sampleOffsets[bankId][atOffset] = sampChild->Attribute("Name");
				}

				sampChild = sampChild->NextSiblingElement();
			}
		}

		if (strcmp(child->Value(), "Soundfont") == 0)
		{
			auto name = child->Attribute("Name");
			auto index = child->IntAttribute("Index", 0);
			soundFontNames[index] = name;
		}

		child = child->NextSiblingElement();
	}
}

std::vector<AdsrEnvelope*> ZAudio::ParseEnvelopeData(const std::vector<uint8_t>& audioBank, const std::vector<uint8_t>& audioTable, int envelopeOffset, int baseOffset)
{
	std::vector<AdsrEnvelope*> result;

	while (true)
	{
		AdsrEnvelope* env = new AdsrEnvelope();

		env->delay = BitConverter::ToInt16BE(audioBank, envelopeOffset + 0);
		env->arg = BitConverter::ToInt16BE(audioBank, envelopeOffset + 2);

		envelopeOffset += 4;

		result.push_back(env);

		if (env->delay < 0)
			break;
	}

	return result;
}

SoundFontEntry* ZAudio::ParseSoundFontEntry(const std::vector<uint8_t>& audioBank,
                                            const std::vector<uint8_t>& audioTable,
                                            AudioTableEntry audioSampleBankEntry, int bankIndex,
                                            int soundFontOffset,
                                            int baseOffset)
{
	SoundFontEntry* soundFont = new SoundFontEntry();

	int sampleOffset = BitConverter::ToInt32BE(audioBank, soundFontOffset + 0) + baseOffset;

	if (sampleOffset == 0)
		return nullptr;

	soundFont->sampleEntry = ParseSampleEntry(
		audioBank, audioTable, audioSampleBankEntry, bankIndex,
		sampleOffset, baseOffset);
	soundFont->tuning = BitConverter::ToFloatBE(audioBank, soundFontOffset + 4);

	return soundFont;
}

SampleEntry* ZAudio::ParseSampleEntry(const std::vector<uint8_t>& audioBank,
                                      const std::vector<uint8_t>& audioTable,
                                      AudioTableEntry audioSampleBankEntry, int bankIndex,
                                      int sampleOffset,
                                      int baseOffset)
{
	int sampleDataOffset = BitConverter::ToInt32BE(audioBank, sampleOffset + 4) + audioSampleBankEntry.ptr;
	int sampleSize = BitConverter::ToInt32BE(audioBank, sampleOffset + 0) & 0x00FFFFFF;
	int loopOffset = BitConverter::ToInt32BE(audioBank, sampleOffset + 8) + baseOffset;
	int bookOffset = BitConverter::ToInt32BE(audioBank, sampleOffset + 12) + baseOffset;

	if (samples.find(sampleDataOffset) == samples.end())
	{
		SampleEntry* sample = new SampleEntry();

		sample->bankId = bankIndex;


		sample->data = std::vector<uint8_t>(sampleSize);
		memcpy(sample->data.data(), audioTable.data() + sampleDataOffset, sampleSize);

		uint32_t origField = (BitConverter::ToUInt32BE(audioBank, sampleOffset + 0));
		sample->codec = (origField >> 28) & 0x0F;
		sample->medium = (origField >> 24) & 0x03;
		sample->unk_bit26 = (origField >> 22) & 0x01;
		sample->unk_bit25 = (origField >> 21) & 0x01;

		sample->loop.start = BitConverter::ToInt32BE(audioBank, loopOffset + 0);
		sample->loop.end = BitConverter::ToInt32BE(audioBank, loopOffset + 4);
		sample->loop.count = BitConverter::ToInt32BE(audioBank, loopOffset + 8);

		if (sample->loop.count != 0)
		{
			for (int i = 0; i < 16; i++)
			{
				// TODO can loop.states be an array of size 16?
				int16_t state = BitConverter::ToInt16BE(audioBank, loopOffset + 16 + (i * 2));
				sample->loop.states.push_back(state);
			}
		}

		sample->book.order = BitConverter::ToInt32BE(audioBank, bookOffset + 0);
		sample->book.npredictors = BitConverter::ToInt32BE(audioBank, bookOffset + 4);

		int32_t numBooks = sample->book.npredictors * sample->book.order * 8;
		sample->book.books.reserve(numBooks);
		for (int i = 0; i < numBooks; i++)
		{
			sample->book.books.push_back(
				BitConverter::ToInt16BE(audioBank, bookOffset + 8 + (i * 2)));
		}

		sample->sampleDataOffset = sampleDataOffset;

		sample->sampleLoopOffset = 0;


		sample->fileName = sampleOffsets[bankIndex][sampleDataOffset];

		samples[sampleDataOffset] = sample;

		return sample;
	}
	else
	{
		return samples[sampleDataOffset];
	}
}

std::vector<AudioTableEntry> ZAudio::ParseAudioTable(const std::vector<uint8_t>& codeData, int baseOffset)
{
	std::vector<AudioTableEntry> entries;

	int numEntries = BitConverter::ToInt16BE(codeData, baseOffset + 0);
	int romAddr = BitConverter::ToInt16BE(codeData, baseOffset + 4);

	int currentOffset = baseOffset + 16;
	entries.reserve(numEntries);
	for (int i = 0; i < numEntries; i++)
	{
		AudioTableEntry entry;

		entry.ptr = BitConverter::ToInt32BE(codeData, currentOffset + 0);
		entry.size = BitConverter::ToInt32BE(codeData, currentOffset + 4);
		entry.medium = codeData[currentOffset + 8];
		entry.cachePolicy = codeData[currentOffset + 9];
		entry.data1 = BitConverter::ToInt16BE(codeData, currentOffset + 10);
		entry.data2 = BitConverter::ToInt16BE(codeData, currentOffset + 12);
		entry.data3 = BitConverter::ToInt16BE(codeData, currentOffset + 14);

		entries.push_back(entry);

		currentOffset += 16;
	}

	return entries;
}

void ZAudio::ParseSoundFont(const std::vector<uint8_t>& codeData, const std::vector<uint8_t>& audioTable,
                            const std::vector<AudioTableEntry>& audioSampleBank,
                            AudioTableEntry& entry)
{
	int ptr = entry.ptr;
	int size = entry.size;
	int sampleBankId1 = (entry.data1 >> 8) & 0xFF;
	int sampleBankId2 = (entry.data1) & 0xFF;
	int numInstruments = (entry.data2 >> 8) & 0xFF;
	int numDrums = entry.data2 & 0xFF;
	int numSfx = entry.data3;

	entry.drums.reserve(numDrums);
	entry.soundEffects.reserve(numSfx);
	entry.instruments.reserve(numInstruments);

	int currentOffset = BitConverter::ToInt32BE(codeData, ptr) + ptr;
	for (int i = 0; i < numDrums; i++)
	{
		DrumEntry drum = {0};

		int samplePtr = BitConverter::ToInt32BE(codeData, currentOffset);

		if (samplePtr != 0)
		{
			samplePtr += ptr;

			drum.sample = ParseSampleEntry(codeData, audioTable, audioSampleBank[sampleBankId1], sampleBankId1,
			                               BitConverter::ToInt32BE(codeData, samplePtr + 4) + ptr, ptr);

			drum.releaseRate = codeData[samplePtr + 0];
			drum.pan = codeData[samplePtr + 1];
			drum.loaded = codeData[samplePtr + 2];
			drum.tuning = BitConverter::ToFloatBE(codeData, samplePtr + 8);
			drum.env = ParseEnvelopeData(codeData, audioTable, BitConverter::ToInt32BE(codeData, samplePtr + 12) + ptr, ptr);
		}
		entry.drums.push_back(drum);


		currentOffset += 4;
	}

	currentOffset = BitConverter::ToInt32BE(codeData, ptr + 4) + ptr;
	for (int i = 0; i < numSfx; i++)
	{
		SoundFontEntry* sfx;
		sfx = ParseSoundFontEntry(codeData, audioTable, audioSampleBank[sampleBankId1], sampleBankId1,
		                          currentOffset, ptr);

		//if (sfx != nullptr)
			entry.soundEffects.push_back(sfx);

		currentOffset += 8;
	}

	for (int i = 0; i < numInstruments; i++)
	{
		InstrumentEntry instrument;

		currentOffset = BitConverter::ToInt32BE(codeData, ptr + 8 + (i * 4));

		instrument.isValidInstrument = currentOffset != 0;

		if (currentOffset != 0)
		{
			currentOffset += ptr;

			instrument.loaded = codeData[currentOffset + 0];
			instrument.normalRangeLo = codeData[currentOffset + 1];
			instrument.normalRangeHi = codeData[currentOffset + 2];
			instrument.releaseRate = codeData[currentOffset + 3];
			instrument.env = ParseEnvelopeData(codeData, audioTable, BitConverter::ToInt32BE(codeData, currentOffset + 4) + ptr, ptr);

			if (BitConverter::ToInt32BE(codeData, currentOffset + 8) != 0)
				instrument.lowNotesSound = ParseSoundFontEntry(
					codeData, audioTable, audioSampleBank[sampleBankId1], sampleBankId1, currentOffset + 8, ptr);

			if (BitConverter::ToInt32BE(codeData, currentOffset + 16) != 0)
				instrument.normalNotesSound = ParseSoundFontEntry(
					codeData, audioTable, audioSampleBank[sampleBankId1], sampleBankId1, currentOffset + 16, ptr);

			if (BitConverter::ToInt32BE(codeData, currentOffset + 24) != 0 &&
			    instrument.normalRangeHi != 0x7F)
				instrument.highNotesSound = ParseSoundFontEntry(
					codeData, audioTable, audioSampleBank[sampleBankId1], sampleBankId1, currentOffset + 24, ptr);
		}
		// Interesting audio bug if you put this next line in the if block
		entry.instruments.push_back(instrument);

	}
}

void ZAudio::ParseRawData()
{
	ZResource::ParseRawData();

	std::vector<uint8_t> codeData;
	std::vector<uint8_t> audioTableData;
	std::vector<uint8_t> audioBankData;
	std::vector<uint8_t> audioSeqData;

	if (Globals::Instance->fileMode == ZFileMode::ExtractDirectory)
		codeData = Globals::Instance->GetBaseromFile("code");
	else
		codeData = Globals::Instance->GetBaseromFile(Globals::Instance->baseRomPath.string() + "code");

	if (Globals::Instance->fileMode == ZFileMode::ExtractDirectory)
		audioTableData = Globals::Instance->GetBaseromFile("Audiotable");
	else
		audioTableData = Globals::Instance->GetBaseromFile(Globals::Instance->baseRomPath.string() + "Audiotable");

	if (Globals::Instance->fileMode == ZFileMode::ExtractDirectory)
		audioBankData = Globals::Instance->GetBaseromFile("Audiobank");
	else
		audioBankData = Globals::Instance->GetBaseromFile(Globals::Instance->baseRomPath.string() + "Audiobank");


	if (Globals::Instance->fileMode == ZFileMode::ExtractDirectory)
		audioSeqData = Globals::Instance->GetBaseromFile("Audioseq");
	else
		audioSeqData = Globals::Instance->GetBaseromFile(Globals::Instance->baseRomPath.string() +
		                                                  "Audioseq");

	// TABLE PARSING

	// MQ DBG ROM
	//int gSoundFontTableOffset = 0x138270;
	//int gSequenceTableOffset = 0x1386A0;
	//int gSampleBankTableOffset = 0x138D90;
	//int gSequenceFontTableOffset = 0x1384E0;
	
	// NTSC 1.0
	//int gSoundFontTableOffset = 0x1026A0;
	//int gSequenceTableOffset = 0x102AD0;
	//int gSampleBankTableOffset = 0x1031C0;
	//int gSequenceFontTableOffset = 0x102910;

	int gSoundFontTableOffset = StringHelper::StrToL(registeredAttributes.at("SoundFontTableOffset").value, 16);
	int gSequenceTableOffset = StringHelper::StrToL(registeredAttributes.at("SequenceTableOffset").value, 16);
	int gSampleBankTableOffset = StringHelper::StrToL(registeredAttributes.at("SampleBankTableOffset").value, 16);
	int gSequenceFontTableOffset = StringHelper::StrToL(registeredAttributes.at("SequenceFontTableOffset").value, 16);

	soundFontTable = ParseAudioTable(codeData, gSoundFontTableOffset);
	sequenceTable = ParseAudioTable(codeData, gSequenceTableOffset);
	sampleBankTable = ParseAudioTable(codeData, gSampleBankTableOffset);

	fontIndices.reserve(sequenceTable.size());
	sequences.reserve(sequenceTable.size());

	// SEQEUNCE FONT TABLE PARSING
	for (int i = 0; i < sequenceTable.size(); i++)
	{
		uint16_t idx = BitConverter::ToUInt16BE(codeData, gSequenceFontTableOffset + (i * 2));
		uint8_t numFonts = codeData[gSequenceFontTableOffset + (idx++)];
		std::vector<uint32_t> fontIds;

		for (int j = 0; j < numFonts; j++)
		{
			uint8_t fontId = codeData[gSequenceFontTableOffset + (idx++)];
			fontIds.push_back(fontId);
		}

		fontIndices.push_back(fontIds);
	}


	// SAMPLE/FONT PARSING
	for (auto& sft : soundFontTable)
	{
		ParseSoundFont(audioBankData, audioTableData, sampleBankTable, sft);
	}

	// SEQUENCE PARSING
	for (int i = 0; i < sequenceTable.size(); i++)
	{
		int seqDestIdx = i;

		if (sequenceTable[i].size == 0)
			seqDestIdx = sequenceTable[i].ptr;

		std::vector<char> seqVec = std::vector<char>(sequenceTable[seqDestIdx].size);
		memcpy(seqVec.data(), audioSeqData.data() + sequenceTable[seqDestIdx].ptr,
		       sequenceTable[seqDestIdx].size);

		sequences.push_back(seqVec);
	}
}

std::string ZAudio::GetSourceTypeName() const
{
	return "u8";
}

size_t ZAudio::GetRawDataSize() const
{
	return 1;
}

ZResourceType ZAudio::GetResourceType() const
{
	return ZResourceType::Audio;
}
