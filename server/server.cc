#include "../util.h"

mutex threads_l, torrents_l;
vector<thread> threads;
unordered_map<string, string> torrents;

void publish_run(Socket s)
{
	output("Receiving torrent");
	string msg, tor, hash;
	stringstream ss;
	ofstream ofs;
	s >> ss;
	msg = ss.str();
	tor = msg.substr(0, msg.find('\n')) + TORRENT;
	hash = SHA1::hash(msg);
	ofs.open(tor);
	ofs << msg;
	ofs.close();
	torrents_l.lock();
	torrents[hash] = tor;
	torrents_l.unlock();
	ss.str(hash);
	s << ss;
	s.shut();
	output("Received " + tor);
}

void publish_port()
{
	Socket s, c;
	s.list(PUBLISH_PORT);
	output("Listening uploaders at port " + to_string(PUBLISH_PORT));
	while (1)
	{
		c = s.acpt();
		if (c.sock() >= 0)
		{
			threads_l.lock();
			threads.push_back(thread(publish_run, c));
			threads_l.unlock();
		}
	}
}

void download_run(Socket s)
{
	output("Sending torrent list");
	string msg;
	stringstream ss;
	ifstream ifs;
	torrents_l.lock();
	for (auto it = torrents.begin(); it != torrents.end(); ++it)
		msg.append(it->second + "\n");
	torrents_l.unlock();
	ss.str(msg);
	s << ss;
	s >> ss;
	msg = ss.str();
	output("Sending " + msg);
	ifs.open(msg);
	s << ifs;
	s.shut();
	output("Sent " + msg);
}

void download_port()
{
	Socket s, c;
	s.list(DOWNLOAD_PORT);
	output("Listening downloaders at port " + to_string(DOWNLOAD_PORT));
	while (1)
	{
		c = s.acpt();
		if (c.sock() >= 0)
		{
			threads_l.lock();
			threads.push_back(thread(download_run, c));
			threads_l.unlock();
		}
	}
}

int main()
{
	threads_l.lock();
	threads.push_back(thread(publish_port));
	threads.push_back(thread(download_port));
	threads_l.unlock();
	while (1);
}
