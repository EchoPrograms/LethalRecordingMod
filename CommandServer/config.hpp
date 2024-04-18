#include <string>
#include "troll.hpp"
#include <fstream>
#ifndef CONFIG_HPP
#define CONFIG_HPP


class Config
{
	public:
	std::string name;
	std::string value;
	bool fromString(std::string in)
	{
		name = sepstr(in, 0, ':');
		value = sepstr(in, 1, ':');
		if (name == "" || value == "")
		{
			name = "";
			value = "";
			return false;
		}
		return true;
	}
};


class ConfigFile
{
	public:
	std::vector<Config> configs;
	bool importFile(std::string filename)
	{
		configs.clear();
		std::ifstream file(filename, std::ios::in);
		std::string line;
		while (getline(file, line))
		{
			Config cfg;
			if (!cfg.fromString(line))
			{
				configs.clear();
				return false;
			}
			configs.push_back(cfg);
		}
		file.close();
		return true;
	}
	std::string get(std::string configName)
	{
		for (int i = 0; i < configs.size(); i++)
		{
			if (configs[i].name == configName)
			{
				return configs[i].value;
			}
		}
		return "";
	}
	ConfigFile() {}
	ConfigFile(std::string fileName)
	{
		importFile(fileName);
	}
};
#endif
