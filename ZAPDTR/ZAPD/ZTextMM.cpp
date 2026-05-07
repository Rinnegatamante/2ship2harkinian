#include "ZTextMM.h"

#include "Globals.h"
#include "Utils/BitConverter.h"
#include <Utils/DiskFile.h>
#include "Utils/Path.h"
#include "Utils/StringHelper.h"
#include "ZFile.h"

REGISTER_ZFILENODE(TextMM, ZTextMM);

ZTextMM::ZTextMM(ZFile* nParent) : ZResource(nParent)
{
	RegisterRequiredAttribute("CodeOffset");
	RegisterOptionalAttribute("LangOffset", "0");
}

void ZTextMM::ParseRawData()
{
	ZResource::ParseRawData();

	ParseMM();
}

void ZTextMM::ParseMM()
{
	const auto& rawData = parent->GetRawData();
	uint32_t currentPtr = StringHelper::StrToL(registeredAttributes.at("CodeOffset").value, 16);
	uint32_t langPtr = currentPtr;
	bool isPalLang = false;

	if (StringHelper::StrToL(registeredAttributes.at("LangOffset").value, 16) != 0)
	{
		langPtr = StringHelper::StrToL(registeredAttributes.at("LangOffset").value, 16);

		if (langPtr != currentPtr)
			isPalLang = true;
	}

	std::vector<uint8_t> codeData;

	if (Globals::Instance->fileMode == ZFileMode::ExtractDirectory)
		codeData = Globals::Instance->GetBaseromFile("code");
	else
		codeData =
			Globals::Instance->GetBaseromFile(Globals::Instance->baseRomPath.string() + "code");

	while (true)
	{
		MessageEntryMM msgEntry;
		msgEntry.id = BitConverter::ToInt16BE(codeData, currentPtr + 0);
		msgEntry.msgOffset = BitConverter::ToInt32BE(codeData, langPtr + 4) & 0x00FFFFFF;

		uint32_t msgPtr = msgEntry.msgOffset;

		msgEntry.segmentId = (codeData[langPtr + 4]);

		// NES and STAFF has different control codes and arg sizes, so we parse them separately
		if (parent->GetName() == "staff_message_data_static") { // STAFF
			// Credits has no header
			// The textbox Type and Pos are in the code table just after the text ID as a single byte
			uint8_t typePos = (codeData[currentPtr + 2]);
			msgEntry.textboxType = (typePos & 0xF0) >> 4;
			msgEntry.textboxYPos = typePos & 0x0F;

			// The rest of these are unused and not provided, so we set them to zero;
			msgEntry.textboxType = 0;
			msgEntry.icon = 0;
			msgEntry.nextMessageID = 0;
			msgEntry.firstItemCost = 0;
			msgEntry.secondItemCost = 0;
		
			unsigned char c = rawData[msgPtr];

			// Read until end marker (0x02)
			while (true) {
				msgEntry.msg += c;

				if (c == 0x02) { // End marker
					break;
				}

				switch (c) {
					case 0x05: // Color: Change text color
					case 0x06: // Shift: Print xx Spaces
					case 0x0E: // Fade: Keep Text on Screen for xxxx Before Closing
					case 0x13: // Item Icon:
					case 0x14: // Text Speed:
					case 0x1E: // Highscores: Prints xx highscore value
						msgEntry.msg += rawData[msgPtr + 1];
						msgPtr++;
						break;
					case 0x07: // TextID: Open xxxx textID
					case 0x0C: // Box Break Delay: Delay for xxxx Before Printing Remaining Text
					case 0x11: // Fade2: Alternate delay
					case 0x12: // SFX: Play Sound Effect xxxx
						msgEntry.msg += rawData[msgPtr + 1];
						msgEntry.msg += rawData[msgPtr + 2];
						msgPtr += 2;
						break;
					case 0x15: // Background
						msgEntry.msg += rawData[msgPtr + 1];
						msgEntry.msg += rawData[msgPtr + 2];
						msgEntry.msg += rawData[msgPtr + 3];
						msgPtr += 3;
						break;
				}

				msgPtr++;
				c = rawData[msgPtr];
			}
		} else if (parent->GetName() == "message_data_static_jp") {
			msgEntry.textboxType = (rawData[msgPtr + 0]);
			msgEntry.textboxYPos = (rawData[msgPtr + 1]);
			msgEntry.icon = BitConverter::ToUInt16BE(rawData, msgPtr + 2);
			msgEntry.nextMessageID = BitConverter::ToInt16BE(rawData, msgPtr + 4);
			msgEntry.firstItemCost = BitConverter::ToInt16BE(rawData, msgPtr + 6);
			msgEntry.secondItemCost = BitConverter::ToInt16BE(rawData, msgPtr + 8);

			msgPtr += 12;

			uint16_t c = BitConverter::ToUInt16BE(rawData, msgPtr);

			// Read until end marker (0x0500)
			while (true) {
				msgEntry.msg += rawData[msgPtr + 1];
				msgEntry.msg += rawData[msgPtr];

				if (c == 0x0500) { // End marker
					break;
				}

				switch (c) {
					case 0x001F: // Shift: Print 00xx Spaces
						msgEntry.msg += rawData[msgPtr + 3];
						msgEntry.msg += rawData[msgPtr + 2];
						msgPtr += 2;
						break;
					case 0x0110: // Box Break Delay: Delay for xxxx Before Printing Remaining Text
					case 0x0111: // Fade: Keep Text on Screen for xxxx Before Closing
					case 0x0112: // Fade Skippable: Delay for xxxx Before Ending Conversation
					case 0x0120: // SFX: Play Sound Effect xxxx
					case 0x0128: // Delay: Delay for xxxx Before Resuming Text Flow
						msgEntry.msg += rawData[msgPtr + 3];
						msgEntry.msg += rawData[msgPtr + 2];
						msgPtr += 2;
						break;
				}

				msgPtr += 2;
				c = BitConverter::ToUInt16BE(rawData, msgPtr);
			}
		} else { // NES
			// NES has a header with extra information, parse that and move the ptr forward
			msgEntry.textboxType = (rawData[msgPtr + 0]);
			msgEntry.textboxYPos = (rawData[msgPtr + 1]);
			msgEntry.icon = (rawData[msgPtr + 2]);
			msgEntry.nextMessageID = BitConverter::ToInt16BE(rawData, msgPtr + 3);
			msgEntry.firstItemCost = BitConverter::ToInt16BE(rawData, msgPtr + 5);
			msgEntry.secondItemCost = BitConverter::ToInt16BE(rawData, msgPtr + 7);

			msgPtr += 11;

			unsigned char c = rawData[msgPtr];

			// Read until end marker (0xBF)
			while (true) {
				msgEntry.msg += c;

				if (c == 0xBF) { // End marker
					break;
				}

				switch (c) {
					case 0x14: // Shift: Print xx Spaces
						msgEntry.msg += rawData[msgPtr + 1];
						msgPtr++;
						break;
					case 0x1B: // Box Break Delay: Delay for xxxx Before Printing Remaining Text
					case 0x1C: // Fade: Keep Text on Screen for xxxx Before Closing
					case 0x1D: // Fade Skippable: Delay for xxxx Before Ending Conversation
					case 0x1E: // SFX: Play Sound Effect xxxx
					case 0x1F: // Delay: Delay for xxxx Before Resuming Text Flow
						msgEntry.msg += rawData[msgPtr + 1];
						msgEntry.msg += rawData[msgPtr + 2];
						msgPtr += 2;
						break;
				}

				msgPtr++;
				c = rawData[msgPtr];
			}
		}

		messages.push_back(msgEntry);

		if (msgEntry.id == 0xFFFC || msgEntry.id == 0xFFFF)
			break;

		currentPtr += 8;

		if (isPalLang)
			langPtr += 4;
		else
			langPtr += 8;
	}
}

std::string ZTextMM::GetSourceTypeName() const
{
	return "u8";
}

size_t ZTextMM::GetRawDataSize() const
{
	return 1;
}

ZResourceType ZTextMM::GetResourceType() const
{
	return ZResourceType::TextMM;
}
