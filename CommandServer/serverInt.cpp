#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <signal.h>
#include "troll.hpp"
#include "config.hpp"


int main (int argc, char*args[])
{
	std::ifstream pidFile("serverPID", std::ios::in);
	if (!pidFile)
	{
		std::cerr << "serverPID file is inaccessible!" << std::endl;
		return 1;
	}
	ConfigFile configs;
	if (!configs.importFile("serverconfig"))
	{
		std::cerr << "Failed to import configuration!" << std::endl;
		return 2;
	}
	std::string pidS;
	getline(pidFile, pidS);
	pidFile.close();
	int serverPID = -1;
	try
	{
		serverPID = std::stoi(pidS);
	}
	catch (...)
	{
		std::cerr << "serverPID file does not contain a PID!" << std::endl;
	}
	if (serverPID == -1)
	{
		return 3;
	}
	if (kill(serverPID, 0) < 0)
	{
		if (errno == ESRCH)
		{
			std::cerr << "PID from serverPID file does not represent a valid process! Please make sure it is running!" << std::endl;
		}
		else if (errno == EPERM)
		{
			std::cerr << "No permission to send signals to server! Please make sure this program is running with the same user as the server, or is root!" << std::endl;
		}
		return 4;
	}
	bool quit = false;
	while (!quit)
	{
		std::cout << ">>> ";
		std::string input;
		getline(std::cin, input);
		if (input == "q" || input == "quit")
		{
			quit = true;
			continue;
		}
		std::string command = sepstr(input, 0, ' ');
		if (command == "s" || command == "save")
		{
			std::string saveFile = sepstr(input, 1, ' ');
			if (!configs.modifyConfig("SaveFile", saveFile))
			{
				std::cerr << "Couldn't modify save file setting!" << std::endl;
				quit = true;
			}
			if (kill(serverPID, SIGUSR1) < 0)
			{
				std::cerr << "Failed to send signal!" << std::endl;
				quit = true;
			}
		}
		else if (command == "h" || command == "help")
		{
			std::cout << "trollServerInterrupter - Send appropriate signals to the server with context" << std::endl;
			std::cout << "Commands:\n\t-save (s) [FILE]. Sends SIGUSR1 to the server, causing it to save in [FILE]" << std::endl;
			std::cout << "\t-load (l) [FILE]. Sends SIGUSR2 to the server, causing it to load from [FILE]" << std::endl;
			std::cout << "\t-kill (k). Sends SIGINT to server, stopping it. (This can also be done by using ^C on the CLI)." << std::endl;
		}
		else if (command == "l" || command == "load")
		{
			std::string loadFile = sepstr(input, 1, ' ');
			if (!configs.modifyConfig("LoadFile", loadFile))
			{
				std::cerr << "Couldn't modify save file setting!" << std::endl;
				quit = true;
			}
			if (kill(serverPID, SIGUSR2) < 0)
			{
				std::cerr << "Failed to send signal!" << std::endl;
				quit = true;
			}
		}
		else if (command == "k" || command == "kill")
		{
			if (kill(serverPID, SIGINT) < 0)
			{
				std::cerr << "Failed to send signal!" << std::endl;
			}
			quit = true;
		}
	}
}
