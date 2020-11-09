// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <math.h>

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include "ClassDefinitions.h"

bool IS_LOGGED_IN = false;
bool LOGIN_ID = "";
bool IS_PEER_OR_SEEDER = false;
int listenSocket;
const int MAX_CONNECTIONS = 3;
string command_string = "";
string fileUploadPath = "";
string fileUploadPathGroup = "";
string fileDownloadPath = "";
string fileDownloadName = "";
struct sockaddr_in trackerAddress, peerAddress;

map<int, file_properties> downloadedFiles;
map<int, vector<peer>> currentSeederList; //file ID,peer

using namespace std; //DOWNLOAD FORMAT = d <FILEID> <PIECE RANGE START> <PIECE RANGE END>

void sendPiece(string ip, int port, int acc, string filePath, int startPiece, int endPiece)
{
	ifstream fp(filePath, ios::in | ios::binary);
	struct stat sb;
	unsigned long long rc = stat(filePath.c_str(), &sb);
	rc = (rc == 0 ? sb.st_size : 0ll);
	int chunks = (int)::ceil(rc / (PIECE_SIZE));

	//cout << "No of 512 KB chunks in file " << chunks << endl;
	//cout << "Total file size(bytes) " << rc << endl;

	size_t CHUNK_SIZE = PIECE_SIZE;
	if (rc < CHUNK_SIZE)
		CHUNK_SIZE = rc;
	char *buff = new char[CHUNK_SIZE];
	int rer = 0;
	fp.seekg(startPiece * PIECE_SIZE, ios::beg);

	while (fp.tellg() <= endPiece * PIECE_SIZE && fp.read(buff, CHUNK_SIZE))
	{
		rc -= CHUNK_SIZE;
		int z = send(acc, buff, CHUNK_SIZE, 0);
		cout << "Sending " << z << " bytes\n";
		rer += z;
		if (rc == 0)
			break;
		if (rc < CHUNK_SIZE)
			CHUNK_SIZE = rc;
	}
	fp.close();
	cout << "Sent " << rer << " bytes to " << ip << ":" << port << endl;
	int sta = close(acc);
	if (sta == 0)
		cout << "Connection closed with " << ip << ":" << port << endl;
}

void getPiece(int seedSocket, string filePath, int pieceLocation)
{
	ofstream fp(filePath, ios::out | ios::binary | ios::app);
	fp.seekp(PIECE_SIZE * pieceLocation, ios::beg);
	int i = 0;
	int n;
	do
	{
		char *buff = new char[PIECE_SIZE];
		memset(buff, 0, PIECE_SIZE);
		n = read(listenSocket, buff, PIECE_SIZE);
		fp.write(buff, n);
		i += n;
		//cout << "Wrote " << n << " bytes\n";
	} while (n > 0);
	fp.close();
}

void listenForConnections()
{
	if (listen(listenSocket, 3) < 0)
	{
		cout << ("Listen Failed \n");
		return;
	}
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(peerAddress.sin_addr), ip, INET_ADDRSTRLEN);
	int port = ntohs(peerAddress.sin_port);
	printf("Listen started on %s:%d\n", ip, port);
	socklen_t addr_size = sizeof(struct sockaddr_in);
	while (IS_PEER_OR_SEEDER)
	{
		memset(ip, 0, INET_ADDRSTRLEN);
		struct sockaddr_in clientAddress;
		int acc = accept(listenSocket, (struct sockaddr *)&clientAddress, &addr_size);
		inet_ntop(AF_INET, &(clientAddress.sin_addr), ip, INET_ADDRSTRLEN);
		port = ntohs(clientAddress.sin_port);
		string fullAddress = string(ip) + ":" + to_string(port);
		printf("connection established with peer IP : %s and PORT : %d\n", ip, port);
		char tmp[4096] = {0};
		recv(acc, tmp, sizeof(tmp), 0);
		string req = string(tmp);
		if (req[0] == 'd')
		{
			stringstream x(req);
			string t;
			vector<string> argsFromPeer;
			while (getline(x, t, ' '))
				argsFromPeer.push_back(t);

			printf("Peer %s:%d requested for %s with piece range from %s-%s\n", ip, port, downloadedFiles[stoi(argsFromPeer[1])].path.c_str(), argsFromPeer[2].c_str(), argsFromPeer[3].c_str());
			thread sendDataToPeer(sendPiece, string(ip), port, acc, downloadedFiles[stoi(argsFromPeer[1])].path, stoi(argsFromPeer[2]), stoi(argsFromPeer[3]));
			sendDataToPeer.detach();
		}
		else
			cout << "Invalid command\n";
	}
	cout << "Listen stopped\n";
}

void startDownload(int fileid, string fileName, string filePath)
{
	vector<peer> peerlist = currentSeederList[fileid];
	int reuseAddress = 1;
	int maxConns = (MAX_CONNECTIONS < peerlist.size() ? MAX_CONNECTIONS : peerlist.size());

	ifstream fp(fileName, ios::in | ios::binary);
	struct stat sb;
	unsigned long long rc = stat(fileName.c_str(), &sb);
	rc = (rc == 0 ? sb.st_size : 0ll);
	int chunks = (int)::ceil(rc / (PIECE_SIZE));
	fp.close();
	size_t CHUNK_SIZE = PIECE_SIZE;
	if (rc < CHUNK_SIZE)
		CHUNK_SIZE = rc;
	int startPiece = 0, endPiece = chunks / maxConns;
	ofstream output(filePath, ios::out | ios::binary | ios::app);
	for (int i = 0; i < maxConns; i++)
	{
		struct sockaddr_in seedAddress;
		seedAddress.sin_family = AF_INET;
		seedAddress.sin_port = htons(peerlist[i].port);
		seedAddress.sin_addr.s_addr = inet_addr(peerlist[i].ip.c_str());

		int seedSocket = socket(AF_INET, SOCK_STREAM, 0);
		setsockopt(seedSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress));
		setsockopt(seedSocket, SOL_SOCKET, SO_REUSEPORT, &reuseAddress, sizeof(reuseAddress));
		if (seedSocket < 0)
		{
			cout << "Unable to download. Socket creation error \n";
			return;
		}
		int trackerConnectStatus = connect(seedSocket, (struct sockaddr *)&seedAddress, sizeof(seedAddress));
		if (trackerConnectStatus < 0)
		{
			cout << "Tracker connection Failed with seeder IP " << seedAddress.sin_addr.s_addr << " and port " << seedAddress.sin_port << "\n";
			return;
		}
		//d <FILEID> <PIECE RANGE START> <PIECE RANGE END>
		string sss = "d " + to_string(fileid) + " " + to_string(startPiece) + " " + to_string(endPiece);
		send(seedSocket, sss.c_str(), sizeof(sss.c_str()), 0);

		thread writeToFile(getPiece, seedSocket, filePath, startPiece);
		writeToFile.detach();

		startPiece = endPiece + 1;
		endPiece += (chunks / maxConns);
		if (startPiece >= chunks)
			i = maxConns;
		if (endPiece >= chunks)
			endPiece = chunks - 1;
	}

	char *buffer = new char[PIECE_SIZE];
	int valread = read(listenSocket, buffer, 4096);
	cout << string(buffer) << endl;
}

int getCommand()
{
	string s, t;
	getline(cin, s);
	if (s == "" || s == "\n")
		return 0;
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
			command_string = "j " + filePath + " " + cmds[2];
			cout << command_string << endl;
			fileUploadPath = filePath;
			fileUploadPathGroup = cmds[2];
			return 20;
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
		fileDownloadName = cmds[2];
		fileDownloadPath = cmds[3];
		return 30;
	}
	else if (cmds[0] == "logout")
	{
		if (!IS_LOGGED_IN)
		{
			cout << "User is not logged in but quitting anyway\n";
		}
		command_string = "l";
		return 100;
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
	else
	{
		cout << "Invalid command\n";
		return 0;
	}
	return 1;
}

int main(int argc, char **argv)
{
	if (argc < 4)
	{
		cout << "Parameters not provided.Exiting...\n";
		return -1;
	}
	int reuseAddress = 1;
	ifstream trackInfo(argv[3]);
	string ix, px;
	trackInfo >> ix >> px;
	//struct sockaddr_in trackerAddress, peerAddress;
	trackerAddress.sin_family = AF_INET;
	trackerAddress.sin_port = htons(stoi(px));
	trackerAddress.sin_addr.s_addr = inet_addr(ix.c_str());
	//trackerAddress.sin_port = htons(stoi(argv[4]));
	//trackerAddress.sin_addr.s_addr = inet_addr(argv[3]);

	peerAddress.sin_family = AF_INET;
	peerAddress.sin_port = htons(stoi(argv[2]));
	peerAddress.sin_addr.s_addr = inet_addr(argv[1]);

	char buffer[4096] = {0};
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
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
		if (cmdFlag == 0 || command_string == "")
			continue;
		//if (cmdFlag == 1){
		send(listenSocket, command_string.c_str(), (command_string).length(), 0);
		command_string = "";
		valread = read(listenSocket, buffer, 4096);
		cout << string(buffer) << endl;
		//}
		if (cmdFlag == 100)
		{
			IS_PEER_OR_SEEDER = false;
			IS_LOGGED_IN = false;
			break;
		}
		else if (cmdFlag == 10 && string(buffer).substr(0, 3) == "Log")
		{
			IS_LOGGED_IN = true;
			//IS_PEER_OR_SEEDER = false;
		}
		else if (cmdFlag == 20 && isdigit(string(buffer)[0]))
		{
			string fileDetails = string(buffer);
			fileDetails = fileDetails.substr(0, fileDetails.find(" "));

			int totPiece = 0;
			ifstream ifs(fileUploadPath, ios::binary);
			string totalHash = "";
			unsigned char piece[PIECE_SIZE], hash[SHA_DIGEST_LENGTH];

			while (ifs.read((char *)piece, sizeof(piece)) || ifs.gcount())
			{
				SHA1(piece, strlen((char *)piece), hash);
				totalHash += string((char *)hash);
				totPiece++;
				memset(piece, 0, PIECE_SIZE);
			}

			file_properties f(stoi(fileDetails), fileUploadPath, fileUploadPath, fileUploadPathGroup, totPiece, totalHash, set<peer>());
			downloadedFiles[stoi(fileDetails)] = f;
			if (!IS_PEER_OR_SEEDER)
			{
				IS_PEER_OR_SEEDER = true;
				thread startListenOnPeer(listenForConnections);
				startListenOnPeer.detach();
			}
		}
		else if (cmdFlag == 30 && isdigit(string(buffer)[0]))
		{
			string b = string(buffer);
			stringstream x(b);
			string t;
			vector<peer> peerList;
			int i = 0, fileid = 0;
			while (getline(x, t, ' '))
			{
				if (i == 0)
					fileid = stoi(t);
				else
				{
					peerList.push_back(peer(t.substr(0, t.find(":")), stoi(t.substr(t.find(":"))), ""));
				}
				i++;
			}
			currentSeederList[fileid] = peerList;
			//START DOWNLOAD
			thread startdl(startDownload, fileid, fileDownloadName, fileDownloadPath);
			startdl.detach();
		}
	}

	return 0;
}