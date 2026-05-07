#ifdef GAME_MM
#include "TextMMExporter.h"
#include "../ZAPD/ZFile.h"

void OTRExporter_TextMM::Save(ZResource* res, const fs::path& outPath, BinaryWriter* writer)
{
	ZTextMM* txt = (ZTextMM*)res;

	WriteHeader(txt, outPath, writer, static_cast<uint32_t>(SOH::ResourceType::TSH_TextMM));

	writer->Write((uint32_t)txt->messages.size());

	for (size_t i = 0; i < txt->messages.size(); i++)
	{
		writer->Write(txt->messages[i].id);
		writer->Write(txt->messages[i].textboxType);
		writer->Write(txt->messages[i].textboxYPos);
		writer->Write(txt->messages[i].icon);
		writer->Write(txt->messages[i].nextMessageID);
		writer->Write(txt->messages[i].firstItemCost);
		writer->Write(txt->messages[i].secondItemCost);
		writer->Write(txt->messages[i].msg);
	}
}
#endif