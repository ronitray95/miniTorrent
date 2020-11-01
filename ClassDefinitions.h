#include <string>
#include <iostream>
#include <set>
using namespace std;

class user
{
public:
	user() {}
	user(string userID, string password)
	{
		this->userID = userID;
		this->password = password;
	}
	string userID;
	string password;
};

class peer
{
public:
	peer()
	{
		ip = "";
		port = 0;
		userID = "";
	}
	peer(string ip, int port, string userID)
	{
		this->ip = ip;
		this->port = port;
		this->userID = userID;
	}
	peer(const peer &p)
	{
		ip = p.ip;
		port = p.port;
		userID = p.userID;
	}
	string ip;
	int port;
	string userID;
};

class group
{
public:
	group(string name, string adminUserID)
	{
		this->name = name;
		this->adminUserID = adminUserID;
	}
	string name;
	string adminUserID;
	set<string> members;
};

class group_pending_request
{
public:
	group_pending_request()
	{
		grpname = "";
		adminname = "";
	}
	group_pending_request(string gName, string admin, set<string> members)
	{
		grpname = gName;
		adminname = admin;
		pendingID = members;
	}
	group_pending_request(const group_pending_request &g)
	{
		grpname = g.grpname;
		adminname = g.adminname;
		pendingID = g.pendingID;
	}
	string grpname;
	string adminname;
	set<string> pendingID;
};
class file_properties
{
public:
	file_properties()
	{
		name = "";
		path = "";
		groupName = "";
		pieces = 0;
		hash = "";
	}
	file_properties(string n, string p, string grp, int pi, string hh)
	{
		name = n;
		path = p;
		groupName = grp;
		pieces = pi;
		hash = hh;
	}
	file_properties(const file_properties &f)
	{
		name = f.name;
		path = f.path;
		groupName = f.groupName;
		pieces = f.pieces;
		hash = f.hash;
	}
	string name;
	string path;
	string groupName;
	int pieces;
	string hash;
};

/*class connectedClient

{
public:
	connectedClient()
	{
		ip = "";
		port = 0;
	}
	connectedClient(string i, int p)
	{
		ip = i;
		port = p;
	}
	connectedClient(const connectedClient &c)
	{
		ip = c.ip;
		port = c.port;
	}
	string ip;
	int port;
};*/