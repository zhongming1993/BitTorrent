#ifndef UTIL_H
#define UTIL_H

#include <mutex>
#include <thread>
#include <vector>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <unordered_map>

using namespace std;

static int BUF_SIZE = 1024, SEG_SIZE = 4 * 1024 * 1024, PUBLISH_PORT = 8080,
	DOWNLOAD_PORT = 8081, REGISTER_PORT = 8082, LIST_PORT = 8083;
static mutex output_l;
static string TORRENT(".torrent"), INVALID_FILE("Invalid file");

void output(string s)
{
	output_l.lock();
	cout << s << endl;
	output_l.unlock();
}

class Socket
{
private:
	int s;

public:
	Socket() : s(socket(AF_INET, SOCK_STREAM, 0)) {}

	Socket(int sock) : s(sock) {}

	int sock() { return s; }

	int port()
	{
		sockaddr_in sai;
		socklen_t l = sizeof(sai);
		getsockname(s, (sockaddr*)&sai, &l);
		return ntohs(sai.sin_port);
	}

	string ip()
	{
		char buf[INET_ADDRSTRLEN];
		sockaddr_in sai;
		socklen_t l = sizeof(sai);
		getpeername(s, (sockaddr*)&sai, &l);
		inet_ntop(AF_INET, &sai.sin_addr, buf, INET_ADDRSTRLEN);
		return buf;
	}

	bool conn(string ip, int port)
	{
		sockaddr_in sai;
		sai.sin_family = AF_INET;
		sai.sin_addr.s_addr = inet_addr(ip.c_str());
		sai.sin_port = htons(port);
		return !connect(s, (sockaddr*)&sai, sizeof(sai));
	}

	void list(int port)
	{
		int on = 1;
		sockaddr_in sai;
		sai.sin_family = AF_INET;
		sai.sin_addr.s_addr = INADDR_ANY;
		sai.sin_port = htons(port);
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		bind(s, (sockaddr*)&sai, sizeof(sai));
		listen(s, 3);
	}

	void shut() { close(s); }

	Socket acpt()
	{
		sockaddr_in sai;
		socklen_t l = sizeof(sai);
		return Socket(accept(s, (sockaddr*)&sai, &l));
	}

	void operator<<(stringstream &ss)
	{
		int l;
		char buf[BUF_SIZE];
		string h;
		ss.seekg(0, ss.end);
		l = ss.tellg();
		h = to_string(l);
		ss.seekg(0, ss.beg);
		send(s, h.c_str(), h.size(), 0);
		while (1)
		{
			recv(s, buf, BUF_SIZE, 0);
			ss.read(buf, BUF_SIZE);
			l = ss.gcount();
			if (!l) break;
			send(s, buf, l, 0);
		}
		ss.clear();
		ss.str("");
	}

	void operator>>(stringstream &ss)
	{
		int l, r;
		char buf[BUF_SIZE];
		string h;
		ss.str("");
		l = recv(s, buf, BUF_SIZE, 0);
		buf[l] = '\0';
		l = atoi(buf);
		while (1)
		{
			h = to_string(l);
			send(s, h.c_str(), h.size(), 0);
			if (!l) break;
			r = recv(s, buf, BUF_SIZE, 0);
			ss.write(buf, r);
			l -= r;
		}
	}

	void operator<<(ifstream &ifs)
	{
		int l, r = SEG_SIZE, i = ifs.tellg();
		char buf[BUF_SIZE];
		string h;
		ifs.seekg(0, ifs.end);
		l = ifs.tellg();
		l = min(l - i, r);
		h = to_string(l);
		ifs.seekg(i);
		send(s, h.c_str(), h.size(), 0);
		while (1)
		{
			recv(s, buf, BUF_SIZE, 0);
			ifs.read(buf, BUF_SIZE);
			l = ifs.gcount();
			if (!l || !r) break;
			send(s, buf, l, 0);
			r -= l;
		}
		ifs.close();
	}

	void operator>>(ofstream &ofs)
	{
		int l, r;
		char buf[BUF_SIZE];
		string h;
		l = recv(s, buf, BUF_SIZE, 0);
		buf[l] = '\0';
		l = atoi(buf);
		while (1)
		{
			h = to_string(l);
			send(s, h.c_str(), h.size(), 0);
			if (!l) break;
			r = recv(s, buf, BUF_SIZE, 0);
			ofs.write(buf, r);
			l -= r;
		}
		ofs.close();
	}
};

class SHA1
{
private:
	static string hex_hash(unsigned char *md)
	{
		stringstream ss;
		for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
			ss << setw(2) << setfill('0') << hex << static_cast<int>(md[i]);
		return ss.str();
	}

public:
	SHA1() = delete;
	static string hash(ifstream &ifs)
	{
		int l;
		char buf[BUF_SIZE];
		unsigned char md[SHA_DIGEST_LENGTH];
		SHA_CTX sha1;
		SHA1_Init(&sha1);
		while (1)
		{
			ifs.read(buf, BUF_SIZE);
			l = ifs.gcount();
			if (!l) break;
			SHA1_Update(&sha1, buf, l);
		}
		SHA1_Final(md, &sha1);
		ifs.close();
		return hex_hash(md);
	}

	static string hash(string &s)
	{
		unsigned char md[SHA_DIGEST_LENGTH];
		SHA_CTX sha1;
		SHA1_Init(&sha1);
		SHA1_Update(&sha1, s.c_str(), s.size());
		SHA1_Final(md, &sha1);
		return hex_hash(md);
	}

	static string hash(const char *s, int l)
	{
		unsigned char md[SHA_DIGEST_LENGTH];
		SHA_CTX sha1;
		SHA1_Init(&sha1);
		SHA1_Update(&sha1, s, l);
		SHA1_Final(md, &sha1);
		return hex_hash(md);
	}
};

#endif
