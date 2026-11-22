// Sexy::QuestDataParser (vtable 0x864328 (bejeweled3), 0xa0 bytes, made by 0x404560): reads a quest definition file
// (properties\quest.cfg, defaultquest.cfg, secret.cfg and speed.cfg, 0x76bc30). A file is a list of commands ending
// in semicolons; "Quest <name>" begins a quest and its settings follow as "<setting> = <value>".
#pragma once

#include "DescParser.h"

#include <map>
#include <string>
#include <vector>

namespace Sexy {

class QuestDataParser : public DescParser {
public:
	struct Quest {
		std::string mName;                              // the second word of the command
		std::map<std::string, std::string> mSettings;   // by setting name; "Title" is the quest's name and "TitleText" its text
	};

	std::vector<Quest> mQuests;                         // in the file's order

	QuestDataParser();                                  // 0x404560
	// 0x791c80 (slot 11): "Quest" with a name makes a quest; any other command is an error
	bool HandleCommand(const ListDataElement& theParams) override;
};

}  // namespace Sexy
