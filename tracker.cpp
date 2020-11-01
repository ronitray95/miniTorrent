// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <openssl/sha.h>

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <thread>

#include "ClassDefinitions.h"

using namespace std;

vector<user> USER_DB; //uid,pass
vector<group> GROUPS; //grp id,owner
//map<string, set<string>> PENDING_REQUESTS;		  //user id,list of groups
//vector<pair<string, string>> UNACCEPTED_REQUESTS; //grp id,usr id
map<string, peer> peerList;
map<string, group_pending_request> groupPendingRequests;
map<string, vector<file_properties>> filesInGroup;
vector<string> connectedClients;
map<string, user> peerDetailsList;

void handlePeerCommunication(string ip, int p, int acc)
{
	cout << "Thread launch\n";
	char buffer[4096] = {0};
	bool IS_LOGGED_IN = false;
	string LOGIN_ID = "";
	while (true)
	{
		memset(buffer, 0, 4096);
		recv(acc, buffer, 4096, 0);
		printf("Client %s:%d said %s\n", ip.c_str(), p, buffer);
		string cmdRecvd = string(buffer), t, msg = "";
		stringstream x(cmdRecvd);
		vector<string> cmds;
		while (getline(x, t, ' '))
		{
			cmds.push_back(t);
		}
		if (cmds[0] == "a")
		{
			user u(cmds[1], cmds[2]);
			bool flag = false;
			for (int i = 0; i < USER_DB.size(); i++)
			{
				if (USER_DB[i].userID == cmds[1])
				{
					msg = "User already exists";
					flag = true;
					break;
				}
			}
			if (!flag)
			{
				msg = "User added";
				USER_DB.push_back(u);
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "b")
		{
			peer client(ip, p, cmds[1]);
			int i;
			for (i = 0; i < USER_DB.size(); i++)
			{
				if (USER_DB[i].userID == cmds[1] && USER_DB[i].password == cmds[2])
					break;
			}
			if (i == USER_DB.size())
			{
				msg = "User does not exist/Wrong password";
			}
			else
			{
				if (peerList.count(cmds[1]) == 0)
				{
					LOGIN_ID = cmds[1];
					peerList[LOGIN_ID] = client;
					IS_LOGGED_IN = true;
					msg = "Logging in with user ID " + cmds[1];
				}
				else
				{
					msg = "Already logged in another instance";
				}
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "c")
		{
			group grp(cmds[1], LOGIN_ID);
			set<string> mems;//add admin to group
			mems.insert(LOGIN_ID);
			grp.members = mems;
			int i;
			bool flag = false;
			for (i = 0; i < GROUPS.size(); i++)
			{
				if (GROUPS[i].name == cmds[1])
				{
					break;
					flag = true;
				}
			}
			if (flag)
			{
				msg = "Group already exists";
			}
			else
			{
				GROUPS.push_back(grp);
				msg = "Group " + grp.name + " created with admin " + grp.adminUserID;
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "d")
		{
			int i;
			bool flag = false;
			for (i = 0; i < GROUPS.size(); i++)
			{
				if (GROUPS[i].name == cmds[1])
				{
					break;
					flag = true;
				}
			}
			if (!flag)
			{
				msg = "Group not found";
			}
			else
			{
				set<string> existingMembers = GROUPS[i].members;
				if (existingMembers.find(LOGIN_ID) != existingMembers.end())
					msg = LOGIN_ID + " is already a part of group " + GROUPS[i].name;
				else
				{
					set<string> xxx = groupPendingRequests[cmds[1]].pendingID;
					xxx.insert(LOGIN_ID);
					group_pending_request gg(cmds[1], GROUPS[i].adminUserID, xxx);
					groupPendingRequests[cmds[1]] = gg;
					msg = "Request sent";
				}
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "e")
		{
			string grpName = cmds[1];
			int i;
			for (i = 0; i < GROUPS.size(); i++)
			{
				if (GROUPS[i].name == grpName)
					break;
			}
			if (i == GROUPS.size())
			{
				msg = "Group not found";
			}
			else
			{
				set<string> grpmembers = GROUPS[i].members;
				if (grpmembers.find(LOGIN_ID) == grpmembers.end())
					msg = "User " + LOGIN_ID + " not present";
				else
				{
					grpmembers.erase(LOGIN_ID);
					GROUPS[i].members = grpmembers;
					msg = "User " + LOGIN_ID + " removed from group " + GROUPS[i].name;
				}
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "f")
		{
			group_pending_request grpp = groupPendingRequests[cmds[1]];
			if (grpp.grpname == "")
				msg = "Group not found/No pending requests";
			else
			{
				string pp = "";
				for (auto i = grpp.pendingID.begin(); i != grpp.pendingID.end(); ++i)
					pp += (*i + " ");
				msg = "For group " + grpp.grpname + " pending requests are: " + pp;
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "g")
		{
			group_pending_request grpp = groupPendingRequests[cmds[1]];
			if (grpp.grpname == "")
				msg = "Group not found/No pending requests";
			else if (grpp.adminname != LOGIN_ID)
				msg = LOGIN_ID + " is not the admin for group " + grpp.grpname;
			else
			{
				set<string> pendID = grpp.pendingID;
				if (pendID.find(cmds[2]) == pendID.end())
					msg = "Group join request for " + cmds[2] + " not found with group " + grpp.grpname;
				else
				{
					pendID.erase(cmds[2]);
					grpp.pendingID = pendID;
					groupPendingRequests[cmds[1]] = grpp;
					msg = "For group " + grpp.grpname + ", join request approved for " + cmds[2];
				}
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "h")
		{
			msg = "All groups in the network: ";
			for (int i = 0; i < GROUPS.size(); i++)
				msg += ("\n" + GROUPS[i].name);
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "i")
		{
			vector<file_properties> grpFiles = filesInGroup[cmds[1]];
			if (grpFiles.size() == 0)
				msg = "No files present in group " + cmds[1];
			else
			{
				msg = "Files present ";
				for (int i = 0; i < grpFiles.size(); i++)
				{
					msg += ("\n" + grpFiles[i].path);
				}
			}
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "j")
		{
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "k")
		{
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "l")
		{
			msg = "Logged out. Bye!";
			peerList.erase(LOGIN_ID);
			auto pos = find(connectedClients.begin(), connectedClients.end(), ip + ":" + to_string(p));
			connectedClients.erase(pos);
			send(acc, msg.c_str(), msg.length(), 0);
			break;
		}
		else if (cmds[0] == "m")
		{
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else if (cmds[0] == "n")
		{
			send(acc, msg.c_str(), msg.length(), 0);
		}
		else
		{
			msg = "Unknown value";
			send(acc, msg.c_str(), msg.length(), 0);
		}
	}
}
/*void getCommand()
{
	
	else if (cmds[0] == "login")
	{
		if (IS_LOGGED_IN)
		{
			cout << "Already logged in.\n";
			return;
		}
		auto x = make_pair(cmds[1], cmds[2]);
		if (find(USER_DB.begin(), USER_DB.end(), x) != USER_DB.end())
		{
			cout << "Login successful.\n";
			IS_LOGGED_IN = true;
			LOGIN_ID = cmds[1];
		}
		else
			cout << "Incorrect username/password\n";
	}
	else if (cmds[0] == "create_group")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return;
		}
		string grpName = cmds[1];
		for (int i = 0; i < GROUPS.size(); i++)
		{
			string existname = GROUPS[i].first;
			if (existname == grpName)
			{

				cout << "Group name must be unique\n";
				return;
			}
		}
		GROUPS.push_back(make_pair(grpName, LOGIN_ID));
	}
	else if (cmds[0] == "join_group")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return;
		}
		string grpName = cmds[1];
		bool flag = true;
		for (int i = 0; i < GROUPS.size(); i++)
		{
			string existname = GROUPS[i].first;
			if (existname == grpName)
			{

				flag = false;
			}
		}
		if (flag)
		{
			cout << "Group not found\n";
			return;
		}
		set<string> existUsers = GROUP_INFO[grpName];
		if (existUsers.empty() || existUsers.find(LOGIN_ID) == existUsers.end())
		{
			existUsers.insert(LOGIN_ID);
			set<string> alsoPending = PENDING_REQUESTS[LOGIN_ID];
			alsoPending.insert(grpName);
			PENDING_REQUESTS[LOGIN_ID] = alsoPending;
			UNACCEPTED_REQUESTS.push_back(make_pair(grpName, LOGIN_ID));
		}
	}
	else if (cmds[0] == "leave_group")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return;
		}
		string grpName = cmds[1];
		bool flag = true;
		for (int i = 0; i < GROUPS.size(); i++)
		{
			string existname = GROUPS[i].first;
			if (existname == grpName)
			{

				flag = false;
			}
		}
		if (flag)
		{
			cout << "Group not found\n";
			return;
		}
		set<string> existUsers = GROUP_INFO[grpName];
		existUsers.erase(LOGIN_ID);
	}
	else if (cmds[0] == "list_requests")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in\n";
			return;
		}
		string grpName = cmds[1];
		bool flag = true;
		for (int i = 0; i < GROUPS.size(); i++)
		{
			string existname = GROUPS[i].first;
			if (existname == grpName)
			{

				flag = false;
			}
		}
		if (flag)
		{
			cout << "Group not found\n";
			return;
		}
	}
	else if (cmds[0] == "accept_request")
	{
	}
	else if (cmds[0] == "list_groups")
	{
	}
	else if (cmds[0] == "list_files")
	{
	}
	else if (cmds[0] == "upload_file")
	{
	}
	else if (cmds[0] == "download_file")
	{
	}
	else if (cmds[0] == "logout")
	{
	}
	else if (cmds[0] == "show_downloads")
	{
	}
	else if (cmds[0] == "stop_share")
	{
	}
	else
	{
		cout << "Invalid command\n";
		return;
	}
}
*/
int main(int argc, char **argv)
{
	if (argc < 3)
	{
		cout << "Parameters not provided.Exiting...\n";
		return -1;
	}
	int reuseAddress = 1;
	struct sockaddr_in trackerAddress, peerAddress;
	trackerAddress.sin_family = AF_INET;
	trackerAddress.sin_port = htons(stoi(argv[2]));
	trackerAddress.sin_addr.s_addr = inet_addr(argv[1]);

	//char buffer[4096] = {0};

	int addrlen = sizeof(trackerAddress);
	int socketStatus = socket(AF_INET, SOCK_STREAM, 0);
	int listenSocketOptions = setsockopt(socketStatus, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress));
	if (socketStatus < 0 || listenSocketOptions < 0)
	{
		cout << "Socket creation error \n";
		return -1;
	}

	int bindStatus = ::bind(socketStatus, (struct sockaddr *)&trackerAddress, sizeof(trackerAddress));
	if (bindStatus < 0)
	{
		printf("%s %s\n", argv[0], argv[1]);
		printf("Bind failed with status: %d\n", bindStatus);
		return -1;
	}
	int listenStatus = listen(socketStatus, 3);
	if (listenStatus < 0)
	{
		cout << ("Listen Failed \n");
		return -1;
	}
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(trackerAddress.sin_addr), ip, INET_ADDRSTRLEN);
	int port = ntohs(trackerAddress.sin_port);
	printf("Listen started on %s:%d\n", ip, port);
	socklen_t addr_size = sizeof(struct sockaddr_in);
	while (true)
	{
		int acc = accept(socketStatus, (struct sockaddr *)&peerAddress, &addr_size);
		/*string exitCheck;
		cin >> exitCheck;
		if (exitCheck == "quit")
			break;*/
		inet_ntop(AF_INET, &(peerAddress.sin_addr), ip, INET_ADDRSTRLEN);
		port = ntohs(peerAddress.sin_port);
		string fullAddress = string(ip) + ":" + to_string(port);
		if (find(connectedClients.begin(), connectedClients.end(), fullAddress) != connectedClients.end())
			continue;
		else
			connectedClients.push_back(fullAddress);
		printf("connection established with IP : %s and PORT : %d\n", ip, port);
		//char buffer[4096] = {0};
		//recv(acc, buffer, 4096, 0);
		//printf("Client : %s\n", buffer);
		string temp = "You are connected to " + string(argv[1]) + ":" + string(argv[2]) + " with IP " + string(ip) + ":" + to_string(port);
		send(acc, temp.c_str(), temp.length(), 0);
		thread launchPeer(handlePeerCommunication, string(ip), port, acc); //, string(ip), port,descriptor
		launchPeer.detach();
	}
	return 0; //compile with g++ -std=c++17 -o tracker tracker.cpp
}