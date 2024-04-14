// ADI DON'T YOU DARE TOUCH MY ERROR MESSAGES
// Also hi how are you doing person who is looking at this code?
// https://youtu.be/dQw4w9WgXcQ
#include <iostream>
#include <vector>
#include <fstream>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#define SETTINGTYPE_NONE -1
#define SETTINGTYPE_BOOL 0
#define SETTINGTYPE_DOUBLE 1
#define SETTINGTYPE_STEAMID 2


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
	int type = SETTINGTYPE_NONE;
	bool bValue;
	double dValue;
	long steamIDValue;
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
		switch (t.settings[i].type)
		{
			case SETTINGTYPE_NONE:
				// Null setting :(
				break;
			case SETTINGTYPE_BOOL:
				out += (char)t.settings[i].bValue;
				out += '\0';
				break;
			case SETTINGTYPE_DOUBLE:
				out += std::to_string(t.settings[i].dValue);
				out += '\0';
				break;
			case SETTINGTYPE_STEAMID:
			       	out += std::to_string(t.settings[i].steamIDValue);
				out += '\0';
				break;
		}
	}
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
	switch (out.type)
	{
		case SETTINGTYPE_BOOL:
			out.bValue = (bool)in[j];
			return out;
		case SETTINGTYPE_DOUBLE: {
			std::string sDouble = "";
			for (int i = j; i < in.size(); i++)
			{
				sDouble += in[i];
			}
			out.dValue = std::stod(sDouble);
			return out;
			}
		case SETTINGTYPE_STEAMID: {
			std::string sID = "";
			for (int i = j; i < in.size(); i++)
			{
				sID += in[i];
			}
			out.dValue = std::stol(sID);
			return out;
			}
		default:
			out.type = SETTINGTYPE_NONE;
			return out;
	}
	out.type = SETTINGTYPE_NONE;
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
		std::string croppedString = in;
		croppedString.erase(0, i);
		if (croppedString.find('\0') == std::string::npos)
		{
			std::cerr << "Invalid format for string while trying to convert to troll" << std::endl;
			return 2;
		}
		i += croppedString.find('\0');
		croppedString.erase(croppedString.find('\0'), std::string::npos);
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


bool quit = false;
TrollState*frameptr;


void signalHandler(int signum)
{
	if (signum == SIGINT) // SIGHUP
	{
		quit = true;
	}
	else if (signum == SIGHUP)
	{
		frameptr->exportState("savedState");
	}
}


int main (int argc, char*args[])
{
	TrollState frame;
	frameptr = &frame;
	int sockfd = -1;
	struct sockaddr_in addr;
	char next = '\0';
	int port = 80;
	struct sigaction sigact;
	sigact.sa_handler = &signalHandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	std::fstream authUsersF("./Auth/users", std::ios::in);
	std::vector<std::string> authUsers;
	std::vector<std::string> authKeys;
	std::vector<std::string> authIPs;
	std::string line;
	while (getline(authUsersF, line))
	{
		authUsers.push_back(line);
		std::fstream keyFile("./Auth/" + line + "Key", std::ios::in);
		std::string data = "";
		getline(keyFile, data);
		authKeys.push_back(data);
		keyFile.close();
		std::fstream IPFile("./Auth/" + line + "IP", std::ios::in);
		getline(IPFile, data);
		authIPs.push_back(data);
		IPFile.close();
	}
	authUsersF.close();
	if (sigaction(SIGHUP, &sigact, nullptr) != 0)
	{
		std::cerr << "Failed to run sigact" << std::endl;
		return 4;
	}
	if (sigaction(SIGINT, &sigact, nullptr) != 0)
	{
		std::cerr << "Failed to run sigact" << std::endl;
		return 4;
	}
	for (int i = 0; i < argc; i++)
	{
		std::string arg = args[i];
		switch (next)
		{
			case 'f':
				
				if (!frame.importState(arg))
				{
					std::cerr << "Failed to load!" << std::endl;
					return 1;
				}
				next = '\0';
				break;
			case 'p':
				port = std::stoi(arg);
				break;
			default:
				if (arg[0] == '-')
				{
					for (int j = 0; j < arg.length(); j++)
					{
						switch (arg[j])
						{
							case 'f':
								next = 'f';
								break;
							case 'p':
								next = 'p';
								break;
						}
					}
				}
				break;
		}
	}
	std::cout << "Loading server..." << std::endl;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		std::cerr << "Failed to create socket!" << std::endl;
		return 2;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		std::cerr << "Failed to bind! Check if your port is open." << std::endl;
		return 3;
	}
	if (listen(sockfd, 10) < 0)
	{
		std::cerr << "Listen Failed!" << std::endl;
	}
	while (!quit)
	{
		socklen_t addrlen;
		int clientfd = accept(sockfd, (struct sockaddr*)&addr, &addrlen);
		std::string receivedData = "";
		char buffer[1025];
		int sizeRead;
		int finish = -1; // To know when to exit
		do
		{
			sizeRead = recv(clientfd, buffer, 1024, MSG_DONTWAIT);
			if (sizeRead < 0)
			{
				switch (errno)
				{
					case EWOULDBLOCK:
						if (receivedData.length() > 0)
						{
							finish = 0; // Normal finish
						}
						break;
					default:
						finish = errno; // Error finish
						break;
				}
			}
			else
			{
				buffer[sizeRead] = '\0';
				receivedData += buffer;
			}
		}
		while (sizeRead > 0 || (sizeRead == -1 && finish == -1));
		if (finish > 0)
		{
			std::cerr << "Error while reading from client!" << std::endl;
			close(clientfd);
			continue;
		}
		// Read successful, process data
		std::string start = receivedData.substr(0, receivedData.find(' ') - 1);
		if (start == "GET" || start == "POST" || start == "PUT" || start == "PATCH" || start == "DELETE") // HTTP request!
		{
		}
		else // Client request!
		{
			std::string recAuthKey = receivedData.substr(0, 10); // 10 first characters for authentication
			char recIPBuffer[INET_ADDRSTRLEN + 1];
			bool IPFound = true;
			if (inet_ntop(AF_INET, &addr.sin_addr, recIPBuffer, addrlen) == nullptr)
			{
				std::cerr << "Could not find IP of client! Using AuthKey." << std::endl;
				IPFound = false;
			}
			recIPBuffer[INET_ADDRSTRLEN] = '\0';
			std::string recIP = recIPBuffer;
			std::string recUser = "";
			bool setByIP = false;
			for (int i = 0; i < authUsers.size(); i++)
			{
				if (authIPs[i] == recIP)
				{
					if (recUser != "")
					{
						std::string message = "EC:Stop pretending to be someone else, bozo!\n";
						write(clientfd, message.c_str(), message.length());
						close(clientfd);
						continue;
					}
					recUser = authUsers[i];
					setByIP = true;
				}
				else if (authKeys[i] == recAuthKey)
				{
					if (recUser != authUsers[i] && setByIP)
					{
						std::string message = "EC:Stop pretending to be someone else, bozo!\n";
						write(clientfd, message.c_str(), message.length());
						close(clientfd);
						continue;
					}
					recUser = authUsers[i];
					if (authIPs[i] == "Unknown" && recUser != "Unity") // Unity is the game, and shouldn't have an assigned IP!
					{
						authIPs[i] = recIP;
						std::ofstream userIPFile("./Auth/" + recUser + "IP", std::ios::out);
						userIPFile << recIP << '\n';
						userIPFile.close();
					}
				}
			}
			if (recUser == "")
			{
				std::string message = "EC:Who are you? Go get a key or something.\n";
				write(clientfd, message.c_str(), message.length());
				close(clientfd);
				continue;
			}
			std::string stdMessage = "A:";
			stdMessage += recUser;
			write(clientfd, stdMessage.c_str(), stdMessage.length());
			bool error = false;
			char cmdType;
			try
			{
				cmdType = receivedData[10];
				std::cout << "Command " << cmdType << " received from user " << recUser << std::endl;
				if (receivedData.length() > 11)
				{
					receivedData = receivedData.substr(11, std::string::npos);
				}
				else
				{
					receivedData = "";
				}
			}
			catch (...)
			{
				std::string message = "EC: That message is not messageing (not enough shit)\n";
				write(clientfd, message.c_str(), message.length());
				error = true;
				close(clientfd);
			}
			if (!error)
			{
			switch (cmdType)
			{
				case 'g': {
					std::string trollName;
					try
					{
						trollName = receivedData.substr(11, receivedData.find('\0', 11) - 12);
					}
					catch (int err)
					{
						std::string message = "EC: That message is not messageing\n";
						write(clientfd, message.c_str(), message.length());
						error = true;
						close(clientfd);
						break;
					}
					std::string trollData = frame.getTrollString(trollName);
					std::string message;
					if (trollData == "")
					{
						message = "EC:Can't find that troll. oop\n";
						error = true;
					}
					else
					{
						message = "S:" + trollData;
					}
					write(clientfd, message.c_str(), message.length());
					break;
					}
				case 'l': {
					std::string trollData = frame.exportState();
					std::string message;
					if (trollData == "")
					{
						message = "ES:I did an oopsie, check my logs\n";
						error = true;
					}
					else
					{
						message = "S:" + trollData;
					}
					write(clientfd, message.c_str(), message.length());
					break;
					}
				case 't': {
					std::string trollName;
					std::string trollData;
					try
					{
						trollName = receivedData.substr(11, receivedData.find('\0', 11) - 12);
						trollData = receivedData.substr(receivedData.find('\0', 11) + 1, std::string::npos);
					}
					catch (...)
					{
						std::string message = "EC: That message is not messageing\n";
						write(clientfd, message.c_str(), message.length());
						error = true;
						close(clientfd);
						break;
					}
					if (!frame.modifyTroll(trollName, trollData))
					{
						std::string message = "EC: HA, YOU MADE AN INVALID MESSAGE! IDIOT!";
						write(clientfd, message.c_str(), message.length());
						error = true;
						close(clientfd);
						break;
					}
					std::string message = "S";
					write(clientfd, message.c_str(), message.length());
					close(clientfd);
					break;
					}
				case 's': {
					std::string trollName = sepstr(receivedData, 0, '\0');
					std::string settingName = sepstr(receivedData, 1, '\0');
					std::string settingData = sepstr(receivedData, 2, '\0');
					if (trollName == "" || settingName == "" || settingData == "")
					{
						std::string message = "EC: That message is not messageing\n";
						write(clientfd, message.c_str(), message.length());
						error = true;
						close(clientfd);
						break;
					}
					if (!frame.modifySetting(trollName, settingName, toTrollSetting(settingData)))
					{
						std::string message = "EC: HA, YOU MADE AN INVALID MESSAGE! IDIOT!";
						write(clientfd, message.c_str(), message.length());
						error = true;
						close(clientfd);
						break;
					}
					std::string message = "S";
					write(clientfd, message.c_str(), message.length());
					close(clientfd);
					break;
					}
				default: {
					// Unknown command
					std::string message = "EC:That's not a command my guy\n";
					write(clientfd, message.c_str(), message.length());
					close(clientfd);
					error = true;
					break;
					}
			}
			}
			if (error)
			{
				continue;
			}
		}
		close(clientfd);
	}
}
