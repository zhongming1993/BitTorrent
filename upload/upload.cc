#include "../util.h"

int NUM_SEG = 0;
string PATH, IP, FILENAME, TOR, HASH;
mutex threads_l;
vector<thread> threads;

void tor_create()
{
	output("Creating " + TOR);
	int l;
	char buf[SEG_SIZE];
	ifstream ifs;
	ofstream ofs;
	ifs.open(PATH);
	ofs.open(TOR);
	ofs << FILENAME << endl;
	ifs.seekg(0, ifs.end);
	l = ifs.tellg();
	ofs << l << endl;
	ofs << (l - 1) / SEG_SIZE + 1 << endl;
	ifs.seekg(0, ifs.beg);
	while (1)
	{
		ifs.read(buf, SEG_SIZE);
		l = ifs.gcount();
		if (!l) break;
		ofs << SHA1::hash(buf, l) << endl;
		++NUM_SEG;
	}
	ifs.close();
	ofs.close();
}

void tor_publish()
{
	output("Publishing " + TOR);
	Socket s;
	ifstream ifs;
	stringstream ss;
	ifs.open(TOR);
	HASH = SHA1::hash(ifs);
	s.conn(IP, PUBLISH_PORT);
	ifs.open(TOR);
	s << ifs;
	s >> ss;
	if (HASH.compare(ss.str()))
	{
		output("Cannot publish " + TOR);
		s.shut();
		exit(1);
	}
	s.shut();
}

void download_run(Socket s)
{
	output("A downloader from " + s.ip());
	int idx;
	ifstream ifs;
	stringstream ss;
	s >> ss;
	if (ss.str().compare(HASH))
	{
		ss.str(INVALID_FILE);
		s << ss;
		s.shut();
		return;
	}
	ss.str(string(NUM_SEG, '1'));
	s << ss;
	while (1)
	{
		s >> ss;
		ss >> idx;
		if (idx == -1) break;
		ifs.open(PATH);
		ifs.seekg(idx * SEG_SIZE);
		s << ifs;
		ss.str("-1");
		s << ss;
	}
	output("Sent data to a downloader from " + s.ip());
	s.shut();
}

void download_port(Socket s, int port)
{
	output("Listening downloaders at port " + to_string(port));
	Socket c;
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

void peer_register()
{
	output("Registering");
	int port;
	string msg;
	Socket s, r;
	stringstream ss;
	s.list(0);
	port = s.port();
	msg = to_string(port) + " " + HASH;
	threads_l.lock();
	threads.push_back(thread(download_port, s, port));
	threads_l.unlock();
	r.conn(IP, REGISTER_PORT);
	ss.str(msg);
	r << ss;
	r.shut();
}

string get_filename()
{
	size_t idx = PATH.rfind('/');
	idx = (idx == string::npos) ? 0 : idx + 1;
	return PATH.substr(idx);
}

int main()
{
	cin >> PATH >> IP;
	FILENAME = get_filename();
	TOR = FILENAME + TORRENT;
	tor_create();
	tor_publish();
	peer_register();
	while (1);
}
