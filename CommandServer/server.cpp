// ADI DON'T YOU DARE TOUCH MY ERROR MESSAGES
// Also hi how are you doing person who is looking at this code?
// https://youtu.be/dQw4w9WgXcQ
// TODO: Someone make HTML pages for errors (400, 401, 403, 404, 405, 418 (funny), 422 and 501)
#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include "troll.hpp"
#include "config.hpp"
#define HTTP_GET 0
#define HTTP_POST 1
#define HTTP_DELETE 2
#define HTTP_OPTIONS 3
#define HTTP_HEAD 4
#define HTTP_PUT 5
#define HTTP_CONNECT 6
#define HTTP_TRACE 7
#define HTTP_PATH 8


bool quit = false;
TrollState*frameptr;
const std::string serverVersion = "1.2.0";
const std::vector<std::string> httpmethods = {"GET", "POST", "DELETE", "OPTIONS", "HEAD", "PUT", "CONNECT", "TRACE", "PATH"};
ConfigFile configs;
std::string saveFile = "";
std::string loadFile = "";


class HTTPHeader
{
	public:
	std::string name;
	std::string value;
};


class HTTPRequest
{
	public:
	int method = -1;
	std::string path;
	int valid = 0; // 0 -> valid, other means error
	std::vector<HTTPHeader> headers;
	std::string content;
	HTTPRequest()
	{
	}
	bool initialize(std::string fullReq)
	{
		std::string headLine = sepstrs(fullReq, 0, "\r\n");
		if (sepstr(headLine, 2, ' ') != "HTTP/1.1")
		{
			valid = 1;
			return false;
		}
		std::string methodS = sepstr(headLine, 0, ' ');
		for (int i = 0; i < httpmethods.size(); i++)
		{
			if (methodS == httpmethods[i])
			{
				method = i;
				break;
			}
		}
		if (method == -1)
		{
			valid = 2;
			return false;
		}
		path = sepstr(headLine, 1, ' ');
		std::string line = "a";
		long unsigned int cIndex = 0;
		for (int i = 1; line != ""; i++)
		{
			line = sepstrs(fullReq, i, "\r\n");
			if (cIndex != std::string::npos)
			{
				if (fullReq.find("\r\n", cIndex) != std::string::npos)
				{
					cIndex += fullReq.find("\r\n", cIndex) + 2;
				}
				else
				{
					cIndex = std::string::npos;
				}
			}
			if (line != "")
			{
				HTTPHeader header;
				header.name = sepstr(line, 0, ':');
				header.value = sepstr(line, 1, ':');
				if (header.value[0] == ' ')
				{
					header.value.erase(header.value.begin());
				}
				if (header.name[header.name.length() - 1] == ' ')
				{
					header.name.erase(header.name.end() - 1);
				}
				headers.push_back(header);
			}
		}
		if (cIndex != std::string::npos)
		{
			content = fullReq.substr(cIndex, std::string::npos);
		}
		else
		{
			content = "";
		}
		return 0;
	}
	HTTPRequest(std::string fullReq)
	{
		initialize(fullReq);
	}
};


class HTTPResponse
{
	public:
	int status;
	std::string description;
	std::vector<HTTPHeader> headers;
	std::string content;
	HTTPResponse() {}
	int send(int sockfd)
	{
		std::string fullResp = "HTTP/1.1 ";
		fullResp += std::to_string(status);
		fullResp += " ";
		fullResp += description;
		fullResp += "\r\n";
		for (int i = 0; i < headers.size(); i++)
		{
			fullResp += headers[i].name;
			fullResp += ": ";
			fullResp += headers[i].value;
			fullResp += "\r\n";
		}
		fullResp += "\r\n";
		fullResp += content;
		int writeRet = write(sockfd, fullResp.c_str(), fullResp.length());
		return writeRet;
	}
};


void signalHandler(int signum)
{
	if (signum != SIGINT && signum != SIGHUP)
	{
		// Reload configs
		if (!configs.importFile("serverconfig"))
		{
			std::cerr << "Failed to reload configuration file!" << std::endl;
			exit(1);
		}
		saveFile = configs.get("SaveFile");
		loadFile = configs.get("LoadFile");
	}
	if (signum == SIGINT || signum == SIGHUP)
	{
		quit = true;
	}
	else if (signum == SIGUSR1)
	{
		std::cout << "Saving state to " << saveFile << std::endl;
		if (saveFile != "")
		{
			if (!frameptr->exportState(saveFile))
			{
				std::cerr << "Failed to export state! Check if `" << saveFile << "' is a valid file!" << std::endl;
			}
		}
		else
		{
			// Probably sent by interrupter to reload configs
		}
	}
	else if (signum == SIGUSR2)
	{
		std::cout << "Loading state from " << loadFile << std::endl;
		if (loadFile != "")
		{
			if (!frameptr->importState(loadFile))
			{
				std::cerr << "Failed to import state!" << std::endl;
			}
		}
		else
		{
			// Probably sent by interrupter to reload configs
		}
	}
}


int main (int argc, char*args[])
{
	if (!configs.importFile("serverconfig"))
	{
		std::cerr << "Failed to import configuration file!" << std::endl;
		return 1;
	}
	std::ofstream pidFile("serverPID", std::ios::out);
	if (!pidFile)
	{
		std::cerr << "Failed to open `serverPID' to store PID" << std::endl;
		return 1;
	}
	pidFile << getpid();
	pidFile.close();
	saveFile = configs.get("SaveFile");
	loadFile = configs.get("LoadFile");
	TrollState frame;
	frameptr = &frame;
	int sockfd = -1;
	struct sockaddr_in addr;
	char next = '\0';
	int port = -1;
	bool importedState = false;
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
	if (sigaction(SIGUSR1, &sigact, nullptr) != 0)
	{
		std::cerr << "Failed to run sigact" << std::endl;
		return 4;
	}
	if (sigaction(SIGUSR2, &sigact, nullptr) != 0)
	{
		std::cerr << "Failed to run sigact" << std::endl;
		return 4;
	}
	if (sigaction(SIGINT, &sigact, nullptr) != 0)
	{
		std::cerr << "Failed to run sigact" << std::endl;
		return 4;
	}
	if (sigaction(SIGHUP, &sigact, nullptr) != 0)
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
				importedState = true;
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
	if (!importedState)
	{
		std::string stateFile = configs.get("StateFile");
		if (stateFile != "")
		{
			if (!frame.importState(stateFile))
			{
				std::cerr << "Failed to load!" << std::endl;
				return 1;
			}

		}
		else
		{
			std::cerr << "Failed to load (no initial state file)!" << std::endl;
			return 1;
		}
	}
	if (port == -1)
	{
		std::string portString = configs.get("Port");
		if (portString == "")
		{
			std::cout << "No port defined! Using 80." << std::endl;
			port = 80;
		}
		else
		{
			try
			{
				port = std::stoi(portString);
			}
			catch (...)
			{
				std::cerr << "Invalid port string! Using port 80." << std::endl;
				port = 80;
			}
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
	std::cout << "Starting server on port " << htons(addr.sin_port) << std::endl;
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
		if (clientfd < 0)
		{
			if (errno != EINTR)
			{
				std::cerr << "Accept error! Errno: " << errno << std::endl;
			}
			continue;
		}
		std::cout << "Found client!" << std::endl;
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
			std::cerr << "Error while reading from client! Errno: " << errno << std::endl;
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
		std::string headerLine = sepstrs(receivedData, 0, "\r\n");
		std::string httpstr = sepstr(headerLine, 2, ' ');
		if (httpstr == "HTTP/1.1") // HTTP request!
		{
			HTTPRequest req(receivedData);
			if (req.valid != 0)
			{
				// Bad request
				std::cerr << "Received invalid HTTP request!" << std::endl;
				HTTPResponse errResponse;
				errResponse.status = 400;
				errResponse.description = "Invalid message!";
				errResponse.send(clientfd);
				close(clientfd);
				continue;
			}
			else if (req.method != HTTP_GET && req.method != HTTP_POST && req.method != HTTP_HEAD) // :)
			{
				std::cerr << "Received HTTP request with bad method! (" << req.method << ")" << std::endl;
				HTTPResponse errResponse;
				errResponse.status = 501;
				errResponse.description = "Unsupported method!";
				errResponse.send(clientfd);
				close(clientfd);
				continue;

			}
			else
			{
				if (req.path[0] == '/')
				{
					req.path.erase(req.path.begin());
				}
				std::string key = sepstr(req.path, 0, '/');
				if (key.length() != 10)
				{
					key = "No key found";
				}
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
				bool error = false;
				for (int i = 0; i < authUsers.size(); i++)
				{
					if (authIPs[i] == recIP)
					{
						if (recUser != "")
						{
							std::cout << "User is pretending to be mewing frfr" << std::endl;
							HTTPResponse errResponse;
							errResponse.status = 422;
							errResponse.description = "Found two users matching description!";
							errResponse.send(clientfd);
							close(clientfd);
							error = true;
						}
						recUser = authUsers[i];
						setByIP = true;
					}
					else if (authKeys[i] == key)
					{
						std::cout << key << std::endl;
						if (recUser != authUsers[i] && setByIP)
						{
							std::cout << "User is pretending to be mewing frfr (2)" << std::endl;
							HTTPResponse errResponse;
							errResponse.status = 422;
							errResponse.description = "Found IP, but key does not match!";
							errResponse.send(clientfd);
							close(clientfd);
							error = true;
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
				if (error)
				{
					continue;
				}
				if (recUser == "" && key == "No key found")
				{
					std::cout << "User is pretending to exist frfr" << std::endl;
					HTTPResponse errResponse;
					errResponse.status = 401;
					errResponse.description = "Identification required!";
					errResponse.send(clientfd);
					close(clientfd);
					continue;
				}
				if (recUser == "" && key != "No key found")
				{
					std::cout << "User has a shitty key lmao" << std::endl;
					HTTPResponse errResponse;
					errResponse.status = 403;
					errResponse.description = "HA! BAD KEY! STUPID!";
					errResponse.send(clientfd);
					close(clientfd);
					continue;
				}
				if (recUser != "" && key != "No key found") // Same again but without key, now that IP is saved!
				{
					std::cout << "HTTP user IP saved!" << std::endl;
					HTTPResponse response;
					response.status = 302;
					response.description = "Authenticated. Continue with request.";
					HTTPHeader head;
					head.name = "Location";
					head.value = "/";
					response.headers.push_back(head);
					response.send(clientfd);
					close(clientfd);
					continue;
				}
				if (sepstr(req.path, 0, '/').length() == 1) // Under the hood request that should be made by JS code
				{
					char cmdType = sepstr(req.path, 0, '/')[0];
					HTTPResponse response;
					HTTPHeader head;
					head.name = "Content-Type";
					head.value = "text/text";
					response.headers.push_back(head);
					response.status = 200;
					response.description = "OK";
					switch (cmdType)
					{
						case 'v': {
							if (req.method == HTTP_HEAD)
							{
								response.send(clientfd);
							}
							else if (req.method == HTTP_GET)
							{
								response.content = serverVersion;
								response.send(clientfd);
							}
							else
							{
								response.status = 405;
								response.description = "Invalid method";
								response.send(clientfd);
							}
							break;}
						case 'l': {
							if (req.method == HTTP_HEAD)
							{
								response.send(clientfd);
							}
							else if (req.method == HTTP_GET)
							{
								response.content = frame.exportState(true);
								response.send(clientfd);
							}
							else
							{
								response.status = 405;
								response.description = "Invalid method";
								response.send(clientfd);
							}
							break;}
						case 's': {
							if (req.method == HTTP_POST)
							{
								std::string trollName = sepstr(req.path, 1, '/');
								std::string settingName = sepstr(req.path, 2, '/');
								std::string settingValue = sepstr(req.path, 3, '/');
								if (!frame.modifySettingValue(trollName, settingName, settingValue))
								{
									response.status = 422;
									response.description = "Bad troll name or setting name";
									response.send(clientfd);
								}
								else
								{
									response.content = "Success";
									response.send(clientfd);
								}
							}
							else
							{
								response.status = 405;
								response.description = "Invalid method";
								response.send(clientfd);
							}
							break;}
						case 't': {
							if (req.method == HTTP_POST)
							{
								std::string trollName = sepstr(req.path, 1, '/');
								std::string trollData = sepstre(req.path, 2, '/');
								if (!frame.modifyTroll(trollName, trollData))
								{
									std::cout << "Invalid or empty name / string for request t" << std::endl;
									response.status = 422;
									response.description = "Bad troll name or troll data";
									response.send(clientfd);
									break;
								}
								response.content = "Success";
								response.send(clientfd);
							}
							else
							{
								response.status = 405;
								response.description = "Invalid method";
								response.send(clientfd);
							}
							break;}
						case 'g': {
							std::string trollName = sepstr(req.path, 0, '/');
							if (req.method == HTTP_GET)
							{
								std::string trollData = frame.getTrollString(trollName);
								if (trollData == "")
								{
									response.status = 422;
									response.description = "Bad troll name";
									response.send(clientfd);
								}
								else
								{
									response.content = trollData;
									response.send(clientfd);
								}
							}
							else if (req.method == HTTP_HEAD)
							{
								if (frame.getTrollString(trollName) == "")
								{
									response.status = 422;
									response.description = "Bad troll name";
									response.send(clientfd);
								}
								else
								{
									response.send(clientfd);
								}
							}
							else
							{
								response.status = 405;
								response.description = "Invalid method";
								response.send(clientfd);
							}
							break;}
						default: {
							response.status = 422;
							response.description = "Bad command";
							response.send(clientfd);
							break; }
					}
				}
				else
				{
					// Generate html file
					std::string htmlFile = "<html><head><title>Troll server client</title></head><body>";
					for (int i = 0; i < frame.trolls.size(); i++)
					{
						htmlFile += "<h1>" + frame.trolls[i].name + "</h1>";
						htmlFile += "<h3>Settings:</h3>";
						htmlFile += "<ul>";
						for (int j = 0; j < frame.trolls[i].settings.size(); j++)
						{
							htmlFile += "<li>" + frame.trolls[i].settings[j].name + ": " + frame.trolls[i].settings[j].value + "</li>";
						}
						htmlFile += "</ul>";
					}
					HTTPResponse response;
					response.status = 200;
					response.description = "OK";
					HTTPHeader head;
					head.name = "Content-Type";
					head.value = "text/html";
					response.headers.push_back(head);
					response.content = htmlFile;
					response.send(clientfd);
				}
			}
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
						std::cout << "User is pretending to be mewing frfr" << std::endl;
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
						std::cout << "User is pretending to be mewing frfr (2)" << std::endl;
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
				std::cout << "User is pretending exist frfr" << std::endl;
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
					std::string trollData = frame.exportState(false);
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
	close(sockfd);
}
