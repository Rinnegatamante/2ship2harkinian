#include "ZText.h"

#include "Globals.h"
#include "Utils/BitConverter.h"
#include <Utils/DiskFile.h>
#include "Utils/Path.h"
#include "Utils/StringHelper.h"
#include "ZFile.h"

REGISTER_ZFILENODE(Text, ZText);

ZText::ZText(ZFile* nParent) : ZResource(nParent)
{
	RegisterRequiredAttribute("CodeOffset");
	RegisterOptionalAttribute("LangOffset", "0");
	RegisterOptionalAttribute("Language", "English");
}

void ZText::ParseRawData()
{
	ZResource::ParseRawData();

	const auto& rawData = parent->GetRawData();
	uint32_t currentPtr = StringHelper::StrToL(registeredAttributes.at("CodeOffset").value, 16);
	uint32_t langPtr = currentPtr;
	bool isPalLang = false;
	bool isJpnLang = false;

	if (registeredAttributes.at("Language").value == "Japanese")
	{
		isJpnLang = true;
	}

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
		codeData = Globals::Instance->GetBaseromFile(Globals::Instance->baseRomPath.string() + "code");

	while (true)
	{
		MessageEntry msgEntry;
		msgEntry.id = BitConverter::ToInt16BE(codeData, currentPtr + 0);

		if (isPalLang)
		{
			msgEntry.segmentId = (codeData[langPtr + 0]);
			msgEntry.msgOffset = BitConverter::ToInt32BE(codeData, langPtr + 0) & 0x00FFFFFF;
		}
		else
		{
			msgEntry.segmentId = (codeData[langPtr + 4]);
			msgEntry.msgOffset = BitConverter::ToInt32BE(codeData, langPtr + 4) & 0x00FFFFFF;
		}

		uint32_t msgPtr = msgEntry.msgOffset;

		if (isJpnLang)
		{
			msgEntry.textboxType = (codeData[currentPtr + 2] & 0xF0) >> 4;
			msgEntry.textboxYPos = (codeData[currentPtr + 2] & 0x0F);

			uint16_t c = BitConverter::ToUInt16BE(rawData, msgPtr);
			unsigned int extra = 0;
			bool stop = false;

			while (!stop || extra > 0) {
				msgEntry.msg += rawData[msgPtr + 1];
				msgEntry.msg += rawData[msgPtr];
				msgPtr+=2;

				// Some control codes require reading extra bytes
				if (extra == 0)
				{
					// End marker, so stop this message and do not read anything else
					if (c == 0x8170)
					{
						stop = true;
					}
					else if (c == 0x000B || c == 0x819A || c == 0x819E || c == 0x81A3 || c == 0x869F || 
							 c == 0x86C7 || c == 0x86C9 || c == 0x81F3)
					{
						extra = 1;
					}
					// "Continue to new text ID", so stop this message and read one more short for the text ID
					else if (c == 0x81CB)
					{
						extra = 1;
						stop = true;
					}
					else if (c == 0x86B3)
					{
						extra = 2;
					}
				}
				else
				{
					extra--;
				}

				c = BitConverter::ToUInt16BE(rawData, msgPtr);
			}
		}
		else
		{
			unsigned char c = rawData[msgPtr];
			unsigned int extra = 0;
			bool stop = false;

			msgEntry.textboxType = (codeData[currentPtr + 2] & 0xF0) >> 4;
			msgEntry.textboxYPos = (codeData[currentPtr + 2] & 0x0F);
			// Continue parsing until we are told to stop and all extra bytes are read
			while ((c != '\0' && !stop) || extra > 0)
			{
				msgEntry.msg += c;
				msgPtr++;

				// Some control codes require reading extra bytes
				if (extra == 0)
				{
					// End marker, so stop this message and do not read anything else
					if (c == 0x02)
					{
						stop = true;
					}
					else if (c == 0x05 || c == 0x13 || c == 0x0E || c == 0x0C || c == 0x1E || c == 0x06 ||
						c == 0x14)
					{
						extra = 1;
					}
					// "Continue to new text ID", so stop this message and read two more bytes for the text ID
					else if (c == 0x07)
					{
						extra = 2;
						stop = true;
					}
					else if (c == 0x12 || c == 0x11)
					{
						extra = 2;
					}
					else if (c == 0x15)
					{
						extra = 3;
					}
				}
				else
				{
					extra--;
				}

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

	int bp2 = 0;
}

std::string ZText::GetSourceTypeName() const
{
	return "u8";
}

size_t ZText::GetRawDataSize() const
{
	return 1;
}

ZResourceType ZText::GetResourceType() const
{
	return ZResourceType::Text;
}
