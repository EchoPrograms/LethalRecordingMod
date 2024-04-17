#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <unistd.h>
#include <cmath>
#include <fcntl.h>
#include <termios.h>
#include "troll.hpp"
#define REQUEST_ACCEPTED 1
#define REQUEST_SUCCESSFUL 2
#define REQUEST_ERROR 4
#define REQUEST_CERROR 12
#define REQUEST_SERROR 20
#define REQUEST_RERROR 28


const std::string clientVersion = "1.0.1"; // Change this if you significantly update the client!


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
};


class RequestInfo
{
	public:
	// Request Info
	struct sockaddr_in address;
	std::string requestString;
	std::string key;
	// Response Info
	std::string user;
	unsigned int success; // Bitmask 1 -> Accepted, 2 -> Succeeded 4-> Error 8 -> Client 16 -> Server
	std::string responseData;
};


std::string readAllFromSocket(int sockfd, bool waitForStart)
{
	std::string received = "";
	char buffer[1025];
	int sizeRead;
	bool finished = false;
	do
	{
		sizeRead = recv(sockfd, buffer, 1024, MSG_DONTWAIT);
		if (sizeRead < 0)
		{
			switch (errno)
			{
				case EAGAIN: // Same as EWOULDBLOCK
					if (received.length() > 0 || !waitForStart)
					{
						finished = true;
					}
					break;
				default:
					return "";
			}
		}
		else
		{
			buffer[sizeRead] = '\0';
			received += buffer;
		}
	}
	while (sizeRead > 0 || (sizeRead < 0 && !finished));
	return received;
}


std::string sendRequest(struct sockaddr_in addr, std::string req, bool waitForServer, std::string key)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		std::cerr << "Failed to open socket!" << std::endl;
		return "\nES";
	}
	if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		std::cerr << "Failed to connect to address!" << std::endl;
		close(sockfd);
		return "\nEC";
	}
	std::string fullReq = key + req;
	write(sockfd, fullReq.c_str(), fullReq.length());
	std::string response = readAllFromSocket(sockfd, waitForServer);
	response += readAllFromSocket(sockfd, waitForServer);
	close(sockfd);
	return response;
}


int handleServerRequest(RequestInfo*req)
{
	std::string serverResponse = sendRequest(req->address, req->requestString, true, req->key);
	std::string rHeader = sepstr(serverResponse, 0, ':');
	req->success = 0;
	req->responseData = "";
	if (rHeader.length() == 0)
	{
		std::cerr << "Error while authenticating to server: response too short!" << std::endl;
		return -1;
	}
	switch (rHeader[0])
	{
		case 'A':
			req->success |= REQUEST_ACCEPTED;
			break;
		case 'E':
			if (rHeader[1] == 'C')
			{
				req->success |= REQUEST_CERROR;
			}
			else if (rHeader[1] == 'S')
			{
				req->success |= REQUEST_SERROR;
			}
			req->responseData = sepstr(serverResponse, 1, ':');
			break;
		case '\n':
			req->success |= REQUEST_RERROR;
			req->responseData = serverResponse;
			break;	
		default:
			std::cerr << "Invalid message header received!" << std::endl;
			req->success |= REQUEST_SERROR;
			req->responseData = serverResponse;
			break;
	}
	if (req->success & REQUEST_ERROR)
	{
		std::cerr << "Error while authenticating to server!" << std::endl;
		return -1;
	}
	req->user = sepstr(serverResponse, 1, ':');
	rHeader = sepstr(serverResponse, 2, ':');
	if (rHeader.length() == 0)
	{
		std::cerr << "Error while getting server response: response too short!" << std::endl;
		return -2;
	}
	switch (rHeader[0])
	{
		case 'S':
			req->success |= REQUEST_SUCCESSFUL;
			break;
		case 'E':
			if (rHeader[1] == 'C')
			{
				req->success |= REQUEST_CERROR;
			}
			else if (rHeader[2] == 'S')
			{
				req->success |= REQUEST_SERROR;
			}
			else
			{
				req->success |= REQUEST_SERROR;
			}
			break;
		default:
			req->success |= REQUEST_SERROR;
			std::cerr << "Server returned invalid header!" << std::endl;
			break;
	}
	if (req->success & REQUEST_ERROR)
	{
		std::cerr << "Either invalid command or server error occured!" << std::endl;
		req->responseData = serverResponse;
		return -2;
	}
	req->responseData = sepstr(serverResponse, 3, ':');
	return req->success;

}


int main(int argc, char*args[])
{
	ConfigFile configs;
	if (!configs.importFile("clientconfig"))
	{
		std::cerr << "Failed to import configuration file!" << std::endl;
		return 1;
	}
	char lastArg = '\0';
	int port = -1;
	std::string host;
	std::string key = "";
	bool error = false;
	for (int i = 0; i < argc; i++)
	{
		std::string arg = args[i];
		switch (lastArg)
		{
			case '\0':
				if (arg[0] == '-')
				{
					for (int j = 1; j < arg.length(); j++)
					{
						switch (arg[j])
						{
							case 'p':
								lastArg = 'p';
								break;
							case 'h':
								lastArg = 'h';
								break;
							case 'k':
								lastArg = 'k';
								break;
							default:
								std::cerr << "Invalid argument" << std::endl;
								return 1;
								break;
						}
					}
				}
				break;
			case 'p':
				try
				{
					port = std::stoi(arg);
				}
				catch (...)
				{
					std::cerr << "Invalid port!" << std::endl;
					error = true;
				}
				lastArg = '\0';
				break;
			case 'h':
				lastArg = '\0';
				host = arg;
				break;
			case 'k':
				lastArg = '\0';
				key = arg;
				break;
			default:
				std::cerr << "Invalid argument -" << lastArg << std::endl;
				return 1;
				break;
		}
	}
	if (key == "")
	{
		key = configs.get("ClientKey");
		if (key == "")
		{
			std::cout << "Client Key not found! Please enter key here: ";
			getline(std::cin, key);
		}
	}
	if (key.length() != 10)
	{
		std::cerr << "Client Key " << key << " is invalid! It should be a string containing numbers and letters with a length of 10!" << std::endl;
		return 2;
	}
	if (host == "")
	{
		host = configs.get("Host");
		if (host == "")
		{
			std::cerr << "Host is not set! Either set it in the config file or use the -h option to specify one!" << std::endl;
			return 3;
		}
	}
	if (port == -1)
	{
		std::string sPort = configs.get("Port");
		if (sPort != "")
		{
			try
			{
				port = std::stoi(sPort);
			}
			catch (...)
			{
				std::cerr << "Invalid port!" << std::endl;
				error = true;
			}
		}
		else
		{
			port = 80;
		}
	}
	if (error)
	{
		std::cerr << "Failed to start!" << std::endl;
		return 1;
	}
	TrollState frame;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0)
	{
		std::cerr << "Invalid host `" << host << "' Make sure it is an IPv4 address with format xxx.xxx.xxx.xxx!" << std::endl;
		return 4;
	}
	try
	{
		addr.sin_port = htons(port);
	}
	catch (...)
	{
		std::cerr << "Invalid port! Make sure it is a valid number!" << std::endl;
		error = true;
	}
	if (error)
	{
		return 5;
	}
	RequestInfo req;
	req.address = addr;
	req.requestString = "v";
	req.key = key;
	if (handleServerRequest(&req) < 0 || req.success & REQUEST_ERROR)
	{
		std::cerr << "Failed to fetch server version!" << std::endl;
		return 6;
	}
	std::string serverVersion = req.responseData;
	std::string user = req.user;
	std::cout << "Connected to server under user `" << user << "'.\n Server version: " << serverVersion << std::endl;
	req.requestString = "l";
	if (handleServerRequest(&req) < 0 || req.success & REQUEST_ERROR)
	{
		std::cerr << "Failed to get server status!" << std::endl;
		return 7;
	}
	std::cout << req.responseData << std::endl;
	if (!frame.importStateString(req.responseData, false))
	{
		std::cerr << "Got server status, but string was invalid!" << std::endl;
		return 8;
	}
	// pseudo GUI
	int page = 0;
	int focusedTroll = -1;
	int focusedSetting = -1;
	int prevPage = 0;
	std::string enteredValue;
	std::string lastServerMessage;
	char input;
	auto lastCheck = std::chrono::steady_clock::now();
	struct termios rawstdin;
	struct termios ogstdin;
	bool refresh = false;
	tcgetattr(STDIN_FILENO, &rawstdin);
	tcgetattr(STDIN_FILENO, &ogstdin);
	rawstdin.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawstdin);
	if (fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK) < 0)
	{
		std::cerr << "Failed to run fcntl!" << std::endl;
		return 9;
	}
	while (input != 'q')
	{
		input = '\0';
		auto currentTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> timeSinceLastCheck = (currentTime - lastCheck);
		if ((double)timeSinceLastCheck.count() >= 1.0)
		{
			lastCheck = currentTime;
			req.requestString = "l";
			refresh = true;
			if (handleServerRequest(&req) < 0 || req.success & REQUEST_ERROR)
			{
				std::cerr << "Failed to get server status!" << std::endl;
				return 7;
			}
			if (!frame.importStateString(req.responseData, false))
			{
				std::cerr << "Got server status, but string was invalid!" << std::endl;
				return 8;
			}
		}
		int errGot;
		do
		{
			errGot = read(0, &input, 1);
		}
		while (errGot > 0);
		if (errGot < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
		{
			if (errno == EINTR)
			{
				continue;
			}
			std::cerr << "Ran into error while reading from terminal!" << std::endl;
			return 10;
		}
		if ((int)input == 27) // ESCAPE
		{
			refresh = true;
			if (focusedSetting != -1)
			{
				focusedSetting = -1;
				enteredValue = "";
			}
			else if (focusedTroll != -1)
			{
				focusedTroll = -1;
				page = prevPage;
				prevPage = -1;
			}
		}
		else if ((int)input == 10) // ENTER I think
		{
			std::cout << "Pressed enter!" << std::endl;
			if (focusedSetting != -1)
			{
				std::cout << "Sending request!" << std::endl;
				req.requestString = "s" + frame.trolls[focusedTroll].name + '/' + frame.trolls[focusedTroll].settings[focusedSetting].name + '/' + enteredValue; 
				std::cout << req.requestString << std::endl;
				if (handleServerRequest(&req) < 0 || (req.success & REQUEST_ERROR))
				{
					lastServerMessage = "Error while sending data! Server response is: " + req.responseData;
				}
				else
				{
					frame.trolls[focusedTroll].settings[focusedSetting].value = enteredValue;
					focusedSetting = -1;
					enteredValue = "";
					lastServerMessage = req.responseData;
				}
			}
			refresh = true;
		}
		else if (focusedTroll != -1 && focusedSetting != -1)
		{
			if ((int)input >= 32 && (int)input < 127)
			{
				enteredValue += input;
			}
			else if ((int)input == 127)
			{
				if (enteredValue.length() > 0)
				{
					enteredValue.pop_back();
				}
				else
				{
					focusedSetting = -1;
					enteredValue = "";
				}
			}
			if ((int)input >= 32 && (int)input <= 127)
			{
				refresh = true;
			}
		}
		else
		{
			switch ((int)input)
			{
				case '\0': {
					break;
					}
				case (int)'q': {
					break;
					}
				case 0x30 ... 0x39: {
					int index = page * 10 + (int)input - 0x30;
					refresh = true;
					if (focusedTroll == -1)
					{
						if (index < frame.trolls.size())
						{
							focusedTroll = index;
							prevPage = page;
							page = 0;
						}
					}
					else if (focusedSetting == -1)
					{
						if (index < frame.trolls[focusedTroll].settings.size())
						{
							focusedSetting = index;
						}
					}
					else
					{
						// How?
					}
					break;
					}
				case ',': {
					if (frame.trolls.size() > page + 1* 10)
					{
						page++;
					}
					break;
					}
				case '.': {
					if (page > 0)
					{
						page--;
					}
					break;
					}
					
			}
		}
		if (refresh)
		{
			refresh = false;
			std::cout << "\033[2J\033[HServer version: " << serverVersion << ", Client version: " << clientVersion << "\n\n";
			if (focusedTroll != -1)
			{
				std::cout << frame.trolls[focusedTroll].name << "\n\n";
				for (int i = page * 10; i < frame.trolls[focusedTroll].settings.size() && i < (page+1)*10; i++)
				{
					std::cout << i - page * 10 << ". " << frame.trolls[focusedTroll].settings[i].name << "\n\t";
					if (i == focusedSetting)
					{
						std::cout << "(" << frame.trolls[focusedTroll].settings[i].value << "):" << enteredValue << "\n";
					}
					else
					{
						std::cout << frame.trolls[focusedTroll].settings[i].value << "\n";
					}
				}
			}
			else
			{
				for (int i = page * 10; i < frame.trolls.size() && i < (page+1)*10; i++)
				{
					std::cout << i - page*10 << ". " << frame.trolls[i].name << "\n\n";
				}
			}
			std::cout << std::flush;
			if (focusedTroll == -1)
			{
				std::cout << "\nPage " << page + 1 << " of " << ceil((double)frame.trolls.size() / 10.0) << std::endl;
			}
			else
			{
				std::cout << "\nSubpage " << page + 1 << " of " << ceil((double)frame.trolls[focusedTroll].settings.size()) << std::endl;
			}
			if (lastServerMessage != "")
			{
				std::cout << lastServerMessage << std::endl;
			}
		}
	}
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &ogstdin); // Restore stdin
	return 0;
}
