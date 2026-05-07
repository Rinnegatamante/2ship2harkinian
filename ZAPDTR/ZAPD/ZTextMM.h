#pragma once

#include "ZResource.h"
#include "tinyxml2.h"

class MessageEntryMM
{
public:
	uint16_t id;
	uint8_t textboxType;
	uint8_t textboxYPos;
	uint16_t icon;
	uint16_t nextMessageID;
	uint16_t firstItemCost;
	uint16_t secondItemCost;
	uint32_t segmentId;
	uint32_t msgOffset;
	std::string msg;
};

class ZTextMM : public ZResource
{
public:
	std::vector<MessageEntryMM> messages;

	ZTextMM(ZFile* nParent);

	void ParseRawData() override;

	void ParseMM();

	std::string GetSourceTypeName() const override;
	ZResourceType GetResourceType() const override;

	size_t GetRawDataSize() const override;
};
