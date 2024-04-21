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
	std::string fileName;
	bool importFile(std::string filename)
	{
		configs.clear();
		fileName = filename;
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
	bool modifyConfig(std::string configName, std::string value)
	{
		bool found = false;
		for (int i = 0; i < configs.size(); i++)
		{
			if (configs[i].name == configName)
			{
				configs[i].value = value;
				found = true;
				break;
			}
		}
		if (!found)
		{
			return false;
		}
		std::string fullFile = "";
		std::ifstream configFileIn(fileName, std::ios::in);
		if (!configFileIn)
		{
			return false;
		}
		std::string line;
		while (getline(configFileIn, line))
		{
			if (sepstr(line, 0, ':') == configName)
			{
				line = configName + ":" + value;
			}
			fullFile += line;
			fullFile += '\n';
		}
		configFileIn.close();
		std::ofstream configFileOut(fileName, std::ios::out);
		configFileOut << fullFile;
		configFileOut.close();
		return true;
	}
	ConfigFile() {}
	ConfigFile(std::string fileName)
	{
		importFile(fileName);
	}
};
#endif
