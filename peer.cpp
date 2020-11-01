// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include "ClassDefinitions.h"

bool IS_LOGGED_IN = false;
bool LOGIN_ID = "";
string command_string = "";

using namespace std;

int getCommand()
{
	string s, t;
	getline(cin, s);
	stringstream x(s);
	vector<string> cmds;
	while (getline(x, t, ' '))
	{
		cmds.push_back(t);
	}
	if (cmds[0] == "create_user")
	{
		if (cmds.size() < 3)
		{
			cout << "Too few parameters\n";
			return 0;
		}
		command_string = "a " + cmds[1] + " " + cmds[2];
	}
	else if (cmds[0] == "login")
	{
		if (IS_LOGGED_IN)
		{
			cout << "Already logged in.\n";
			return 0;
		}
		if (cmds.size() < 3)
		{
			cout << "Too few parameters\n";
			return 0;
		}
		command_string = "b " + cmds[1] + " " + cmds[2];
		return 10;
	}
	else if (cmds[0] == "create_group")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return 0;
		}
		if (cmds.size() < 2)
		{
			cout << "Too few parameters\n";
			return 0;
		}
		command_string = "c " + cmds[1];
	}
	else if (cmds[0] == "join_group")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return 0;
		}
		if (cmds.size() < 2)
		{
			cout << "Too few parameters\n";
			return 0;
		}
		command_string = "d " + cmds[1];
	}
	else if (cmds[0] == "leave_group")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return 0;
		}
		if (cmds.size() < 2)
		{
			cout << "Too few parameters\n";
			return 0;
		}
		command_string = "e " + cmds[1];
	}
	else if (cmds[0] == "list_requests")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return 0;
		}
		if (cmds.size() < 2)
		{
			cout << "Too few parameters\n";
			return 0;
		}
		command_string = "f " + cmds[1];
	}
	else if (cmds[0] == "accept_request")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return 0;
		}
		if (cmds.size() < 3)
		{
			cout << "Too few parameters\n";
			return 0;
		}
		command_string = "g " + cmds[1] + " " + cmds[2];
	}
	else if (cmds[0] == "list_groups")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return 0;
		}
		command_string = "h";
	}
	else if (cmds[0] == "list_files")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return 0;
		}
		if (cmds.size() < 2)
		{
			cout << "Too few parameters\n";
			return 0;
		}
		command_string = "i " + cmds[1];
	}
	else if (cmds[0] == "upload_file")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return 0;
		}
		if (cmds.size() < 3)
		{
			cout << "Too few parameters\n";
			return 0;
		}
		char path[4096] = {0};
		string filePath = "";
		getcwd(path, 4096);
		if (cmds[1][0] != '~')
		{
			filePath = (string(path) + (cmds[1][0] == '/' ? "" : "/") + cmds[1]);
		}
		if (FILE *file = fopen(filePath.c_str(), "r"))
		{
			fclose(file);
			command_string = "j " + cmds[1] + " " + cmds[2];
		}
		else
		{
			cout << "File not found\n";
			return 0;
		}
	}
	else if (cmds[0] == "download_file")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return 0;
		}
		if (cmds.size() < 4)
		{
			cout << "Too few parameters\n";
			return 0;
		}
		command_string = "k " + cmds[1] + " " + cmds[2] + " " + cmds[3];
	}
	else if (cmds[0] == "logout")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in but quitting anyway\n";
		}
		command_string = "l";
		return 1;
	}
	else if (cmds[0] == "show_downloads")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return 0;
		}
		command_string = "m";
	}
	else if (cmds[0] == "stop_share")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return 0;
		}
		if (cmds.size() < 3)
		{
			cout << "Too few parameters\n";
			return 0;
		}
		command_string = "n " + cmds[1] + " " + cmds[2];
	}
	else{
		cout << "Invalid command\n";
		return 0;
	}
	//return 0;
}

int main(int argc, char **argv)
{
	if (argc < 5)
	{
		cout << "Parameters not provided.Exiting...\n";
		return -1;
	}
	int reuseAddress = 1;
	struct sockaddr_in trackerAddress, peerAddress;
	trackerAddress.sin_family = AF_INET;
	trackerAddress.sin_port = htons(stoi(argv[4]));
	trackerAddress.sin_addr.s_addr = inet_addr(argv[3]);

	peerAddress.sin_family = AF_INET;
	peerAddress.sin_port = htons(stoi(argv[2]));
	peerAddress.sin_addr.s_addr = inet_addr(argv[1]);

	char buffer[4096] = {0};
	int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	int listenSocketOptions = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress));
	if (listenSocket < 0 || listenSocketOptions < 0)
	{
		cout << "Socket creation error \n";
		return -1;
	}
	int bindStatus = ::bind(listenSocket, (struct sockaddr *)&peerAddress, sizeof(struct sockaddr_in));
	if (bindStatus < 0)
	{
		cout << ("Bind Failed \n");
		return -1;
	}
	int trackerConnectStatus = connect(listenSocket, (struct sockaddr *)&trackerAddress, sizeof(trackerAddress));
	if (trackerConnectStatus < 0)
	{
		cout << ("Tracker connection Failed \n");
		return -1;
	}
	int valread = read(listenSocket, buffer, 4096);
	cout << string(buffer) << endl;
	while (true)
	{
		memset(buffer, 0, 4096);
		int cmdFlag = getCommand();
		if (cmdFlag != 0)
		{
			send(listenSocket, command_string.c_str(), command_string.length(), 0);
			valread = read(listenSocket, buffer, 4096);
			cout << string(buffer) << endl;
		}
		if (cmdFlag == 1)
			break;
		else if (cmdFlag == 10 && string(buffer).substr(0, 3) == "Log")
			IS_LOGGED_IN = true;
	}

	return 0;
}