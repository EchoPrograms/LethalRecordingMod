#include <iostream>
#include <vector>
#include <fstream>
#ifndef TROLL_HPP
#define TROLL_HPP
#define SETTINGTYPE_NONE -1
#define SETTINGTYPE_BOOL 1
#define SETTINGTYPE_DOUBLE 2
#define SETTINGTYPE_STEAMID 3


std::string sepstr(std::string str, int p, char c)
{
	std::string ret = str;
	for (int i = 0; i < p; i++)
	{
		ret.erase(0, ret.find(c));
		ret.erase(0, 1);
	}
	return ret.substr(0, ret.find(c));
}


class TrollSetting
{
	public:
	std::string name; // MUST NOT CONTAIN NULL CHARACTER
	std::string value;
	int type = SETTINGTYPE_NONE;
};


class Troll // Contains settings for trolls and names, nothing more
{
	public:
	std::string name; // MUST NOT CONTAIN NULL CHARACTER
	std::vector<TrollSetting> settings;
};


std::string toString(Troll t)
{
	std::string out = "";
	if (t.settings.size() > 255)
	{
		std::cerr << "Tried to make a troll into a string, but it had over 255 settings!" << std::endl;
		return "";
	}
	out += t.name;
	out += '\0';
	out += (char)t.settings.size();
	for (int i = 0; i < t.settings.size(); i++)
	{
		out += (char)t.settings[i].type;
		out += t.settings[i].name;
		out += '\0';
		out += out.value;
		if (i < t.settings.size() - 1)
		{
			out += '\0';
		}
	}
	return out;
}


std::string toString(TrollSetting in)
{
	std::string out;
	out += (char)in.type;
	out += in.name;
	out += '\0';
	out += in.value;
	return out;
}


TrollSetting toTrollSetting(std::string in)
{
	TrollSetting out;
	if (in.size() < 2)
	{
		std::cerr << "Tried to convert string to setting, but it was too short!";
		out.type = SETTINGTYPE_NONE;
		return out;
	}
	out.type = in[0];
	out.name = in.substr(1, in.find('\0') - 1);
	int j = in.find('\0') + 1;
	out.value = in.substr(j, std::string::npos);
	return out;
}


int toTroll(std::string in, Troll*out)
{
	out->settings.clear();
	out->name = "";
	int i;
	for (i = 0; in[i] != '\0' && i < in.size(); i++)
	{
		out->name += in[i];
	}
	if (i >= in.size())
	{
		std::cerr << "Tried converting string that does not contain enough information to Troll" << std::endl;
		return 1;
	}
	int settingCount = (int)in[++i];
	while (out->settings.size() < settingCount && i < in.size())
	{
		out->settings.reserve(1);
		std::string croppedString = sepstr(in, i, '\0') + '\0' + sepstr(in, ++i, '\0');
		TrollSetting ts = toTrollSetting(croppedString);
		if (ts.type == SETTINGTYPE_NONE)
		{
			std::cerr << "Unable to convert string to setting!" << std::endl;
			return 3;
		}
		out->settings.push_back(ts);
	}
	return 0;
}


class TrollState
{
	public:
	std::vector<Troll> trolls;
	std::string exportState()
	{
		std::string out;
		for (int i = 0; i < trolls.size(); i++)
		{
			std::string s = toString(trolls[i]);
			if (s != "")
			{
				out += s;
			}
			else
			{
				std::cerr << "Unable to transform troll into string while exporting state!" << std::endl;
				return "";
			}
			out += '\n';
		}
		return out;
	}
	void exportState(std::string filename) // Careful, this will override files!
	{
		std::string fullOutString = exportState();
		if (fullOutString == "")
		{
			std::cerr << "Failed to export" << std::endl;
			return;
		}
		std::fstream file(filename, std::ios::out | std::ios::trunc);
		file << fullOutString;
		file.close();
	}
	bool importTroll(std::string str)
	{
		Troll t;
		if (toTroll(str, &t) != 0)
		{
			std::cerr << "Failed to import troll!" << std::endl;
			return false;
		}
		trolls.reserve(1);
		trolls.push_back(t);
		return true;
	}
	bool importStateString(std::string in)
	{
		trolls.clear();
		std::string line = sepstr(in, 0, '\n');
		for (int i = 1; line != ""; i++)
		{
			if (!importTroll(line))
			{
				std::cerr << "Failed to import state!" << std::endl;
				return false;
			}
			line = sepstr(in, i, '\n');
		}
		return true;
	}
	bool importState(std::string filename)
	{
		std::fstream file(filename, std::ios::in);
		std::string line = "";
		trolls.clear();
		while (getline(file, line))
		{
			if (!importTroll(line))
			{
				std::cerr << "Failed to import state!" << std::endl;
				return false;
			}
		}
		file.close();
		return true;
	}
	bool modifySetting(std::string trollName, std::string settingName, TrollSetting newValue)
	{
		if (newValue.name == "")
		{
			return false;
		}
		bool foundAtLeastOneSetting = false;
		for (int i = 0; i < trolls.size(); i++)
		{
			if (trolls[i].name == trollName)
			{
				for (int j = 0; j < trolls[i].settings.size(); j++)
				{
					if (trolls[i].settings[j].name == settingName)
					{
						trolls[i].settings[j] = newValue;
						std::cout << "Successfully modified " << settingName << " of troll " << trollName << std::endl;
						foundAtLeastOneSetting = true;
					}
				}
			}
		}
		if (!foundAtLeastOneSetting)
		{
			std::cerr << "Either didn't find troll called " << trollName << ", or couldn't find setting " << settingName << " within it." << std::endl;
		}
		return foundAtLeastOneSetting;
	}
	bool modifyTroll(std::string trollName, std::string trollString)
	{
		for (int i = 0; i < trolls.size(); i++)
		{
			if (trolls[i].name == trollName)
			{
				Troll old = trolls[i];
				if (toTroll(trollString, &trolls[i]) != 0)
				{
					trolls[i] = old;
					std::cerr << "Invalid string used to modify troll!" << std::endl;
					return false;
				}
				return true;
			}
		}
		std::cerr << "Couldn't find setting " << trollName << std::endl;
		return false;
	}
	TrollSetting getSetting(std::string trollName, std::string settingName)
	{
		for (int i = 0; i < trolls.size(); i++)
		{
			if (trolls[i].name == trollName)
			{
				for (int j = 0; j < trolls[i].settings.size(); j++)
				{
					if (trolls[i].settings[j].name == settingName)
					{
						return trolls[i].settings[j];
					}
				}
			}
		}
		TrollSetting errSetting;
		errSetting.name = "";
		return errSetting;
	}
	std::string getTrollString(std::string trollName)
	{
		for (int i = 0; i < trolls.size(); i++)
		{
			if (trolls[i].name == trollName)
			{
				return toString(trolls[i]);
			}
		}
		return "";
	}
};
#endif
