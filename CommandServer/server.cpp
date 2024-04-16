// ADI DON'T YOU DARE TOUCH MY ERROR MESSAGES
// Also hi how are you doing person who is looking at this code?
// https://youtu.be/dQw4w9WgXcQ
#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include "troll.hpp"


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
	std::string serverVersion;
	std::ifstream serverVersionF("serverVersion", std::ios::in);
	getline(serverVersionF, serverVersion);
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
				next = '\0';
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
		std::cerr << "Failed to bind! Check if port " << port << " is open." << std::endl;
		return 3;
	}
	if (listen(sockfd, 10) < 0)
	{
		std::cerr << "Listen Failed!" << std::endl;
	}
	std::cout << "Listening!" << std::endl;
	while (!quit)
	{
		socklen_t addrlen;
		int clientfd = accept(sockfd, (struct sockaddr*)&addr, &addrlen);
		std::string receivedData = "";
		char buffer[1025];
		int sizeRead;
		int finish = -1; // To know when to exit
		auto startTime = std::chrono::steady_clock::now();
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
						/*
						else if (std::chrono::steady_clock::duration(std::chrono::steady_clock::now() - startTime).count() > 0.5) // The funny chrono thing is DDOS protection, if it tries to read for more than 0.1 seconds it exits
						{
							finish = -2; // Timeout finish
						}
						*/
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
		else if (finish == -2)
		{
			std::cerr << "Error: client timeout (skill issue just send messages within 0.1 seconds :) )" << std::endl;
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
			stdMessage += ":";
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
						break;
					}
					if (!frame.modifyTroll(trollName, trollData))
					{
						std::string message = "EC: HA, YOU MADE AN INVALID MESSAGE! IDIOT!";
						write(clientfd, message.c_str(), message.length());
						error = true;
						break;
					}
					std::string message = "S:Modified troll";
					write(clientfd, message.c_str(), message.length());
					break;
					}
				case 's': {
					std::string trollName = sepstr(receivedData, 0, '/');
					std::string settingName = sepstr(receivedData, 1, '/');
					std::string settingData = sepstr(receivedData, 2, '/');
					if (trollName == "" || settingName == "" || settingData == "")
					{
						std::string message = "EC: That message is not messageing\n";
						write(clientfd, message.c_str(), message.length());
						error = true;
						break;
					}
					std::cout << trollName << ", " << settingName << ", " << settingData << std::endl;
					if (!frame.modifySettingValue(trollName, settingName, settingData))
					{
						std::string message = "EC: HA, YOU MADE AN INVALID MESSAGE! IDIOT!";
						write(clientfd, message.c_str(), message.length());
						error = true;
						break;
					}
					std::string message = "S:Modified setting.";
					write(clientfd, message.c_str(), message.length());
					break;
					}
				case 'v': {
					std::string message = "S:" + serverVersion;
					write(clientfd, message.c_str(), message.length());
					break;
				}
				default: {
					// Unknown command
					std::string message = "EC:That's not a command my guy\n";
					write(clientfd, message.c_str(), message.length());
					error = true;
					break;
					}
			}
			}
			if (error)
			{
				close(clientfd);
				continue;
			}
		}
		close(clientfd);
	}
}
