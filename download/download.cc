#include "../util.h"
#include <condition_variable>

mutex threads_l, bitmap_l, haves_l;
condition_variable cv;
vector<int> haves;
vector<int> bitmap;
vector<string> HASHES;
vector<thread> threads;
unordered_map<string, int> peers;
string IP, TOR, FILENAME, SIZE, HASH;

void tor_download()
{
	output("Receiving torrent list");
	int num_seg;
	Socket s;
	ofstream ofs;
	stringstream ss;
	s.conn(IP, DOWNLOAD_PORT);
	s >> ss;
	output(ss.str());
	cin >> TOR;
	output("Receiving " + TOR);
	ss.str(TOR);
	s << ss;
	s >> ss;
	ofs.open(TOR);
	ofs << ss.str();
	ofs.close();
	ss >> FILENAME >> SIZE >> num_seg;
	HASHES.resize(num_seg);
	bitmap.resize(num_seg);
	for (int i = 0; i < num_seg; ++i) ss >> HASHES[i];
	s.shut();
}

void peer_list()
{
	output("Receiving peer list");
	size_t idx;
	Socket s;
	string line;
	ifstream ifs;
	stringstream ss;
	ifs.open(TOR);
	HASH = SHA1::hash(ifs);
	s.conn(IP, LIST_PORT);
	ss.str(HASH);
	s << ss;
	s >> ss;
	while (ss)
	{
		getline(ss, line);
		if (line.empty()) break;
		idx = line.find(' ');
		peers[line.substr(0, idx)] = stoi(line.substr(idx + 1));
	}
	s.shut();
}

void download_run(Socket s)
{
	output("A downloader from " + s.ip());
	int idx;
	size_t sent = 0;
	string msg;
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
	bitmap_l.lock();
	for (size_t i = 0; i < bitmap.size(); ++i)
		msg += bitmap[i] == 2 ? '1' : '0';
	bitmap_l.unlock();
	ss.str(msg);
	s << ss;
	while (1)
	{
		s >> ss;
		ss >> idx;
		if (idx == -1) break;
		ifs.open(FILENAME + ".seg." + to_string(idx));
		s << ifs;
		msg = "";
		haves_l.lock();
		while (sent < haves.size()) msg += to_string(haves[sent++]) + " ";
		haves_l.unlock();
		msg += "-1";
		ss.str(msg);
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

bool check_done()
{
	bitmap_l.lock();
	for (size_t i = 0; i < bitmap.size(); ++i)
	{
		if (bitmap[i] != 2)
		{
			bitmap_l.unlock();
			return 0;
		}
	}
	bitmap_l.unlock();
	return 1;
}

void file_download(string ip)
{
	output("Downloading from " + ip);
	int port = peers[ip], seg, idx;
	Socket s;
	string msg;
	stringstream ss;
	ifstream ifs;
	ofstream ofs;
	vector<int> segments;
	if (!s.conn(ip, port))
	{
		output("Cannot connect to " + ip);
		return;
	}
	ss.str(HASH);
	s << ss;
	s >> ss;
	msg = ss.str();
	if (!msg.compare(INVALID_FILE))
	{
		s.shut();
		return;
	}
	for (size_t i = 0; i < msg.size(); ++i)
		if (msg[i] == '1') segments.push_back(i);
	for (size_t i = 0; i < segments.size(); ++i)
	{
		seg = segments[i];
		bitmap_l.lock();
		if (bitmap[seg])
		{
			bitmap_l.unlock();
			continue;
		}
		else bitmap[seg] = 1;
		bitmap_l.unlock();
		msg = to_string(seg);
		ss.str(msg);
		s << ss;
		msg = FILENAME + ".seg." + msg;
		ofs.open(msg);
		s >> ofs;
		s >> ss;
		while (1)
		{
			ss >> idx;
			if (idx == -1) break;
			segments.push_back(idx);
			output(ip + " has segment " + to_string(idx));
		}
		ifs.open(msg);
		if (HASHES[seg].compare(SHA1::hash(ifs)))
		{
			segments.push_back(seg);
			bitmap_l.lock();
			bitmap[seg] = 0;
			bitmap_l.unlock();
		}
		else
		{
			output("Received " + msg + " from " + ip);
			bitmap_l.lock();
			bitmap[seg] = 2;
			bitmap_l.unlock();
			haves_l.lock();
			haves.push_back(seg);
			haves_l.unlock();
			if (check_done()) cv.notify_one();
		}
	}
	ss.str("-1");
	s << ss;
	s.shut();
}

int main()
{
	char buf[BUF_SIZE];
	mutex mtx;
	unique_lock<mutex> lck(mtx);
	string seg;
	ifstream ifs;
	ofstream ofs;
	cin >> IP;
	tor_download();
	peer_list();
	peer_register();
	threads_l.lock();
	for (auto it = peers.begin(); it != peers.end(); ++it)
		threads.push_back(thread(file_download, it->first));
	threads_l.unlock();
	while (!check_done()) cv.wait(lck);
	ofs.open(FILENAME);
	for (size_t i = 0; i < HASHES.size(); ++i)
	{
		seg = FILENAME + ".seg." + to_string(i);
		ifs.open(seg);
		while (ifs)
		{
			ifs.read(buf, BUF_SIZE);
			ofs.write(buf, ifs.gcount());
		}
		ifs.close();
		remove(seg.c_str());
	}
	ofs.close();
	output("Downloaded " + FILENAME);
	while (1);
}
