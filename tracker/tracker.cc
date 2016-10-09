#include "../util.h"

mutex threads_l, file_l;
vector<thread> threads;

void register_run(Socket s)
{
	string msg;
	stringstream ss;
	ofstream ofs;
	s >> ss;
	msg = s.ip() + " " + ss.str();
	file_l.lock();
	ofs.open("peer_list", ofs.app);
	ofs << msg << endl;
	ofs.close();
	file_l.unlock();
	s.shut();
	output(msg);
}

void register_port()
{
	Socket s, c;
	s.list(REGISTER_PORT);
	output("Listening uploaders at port " + to_string(REGISTER_PORT));
	while (1)
	{
		c = s.acpt();
		if (c.sock() >= 0)
		{
			threads_l.lock();
			threads.push_back(thread(register_run, c));
			threads_l.unlock();
		}
	}
}

void list_run(Socket s)
{
	size_t idx;
	string msg, hash, line;
	stringstream ss;
	ifstream ifs;
	s >> ss;
	hash = ss.str();
	file_l.lock();
	ifs.open("peer_list");
	while (ifs)
	{
		getline(ifs, line);
		idx = line.rfind(' ');
		if (!hash.compare(line.substr(idx + 1)))
			msg.append(line.substr(0, idx) + "\n");
	}
	ifs.close();
	file_l.unlock();
	ss.str(msg);
	s << ss;
	s.shut();
	output("Sent peer_list with hash " + hash);
}

void list_port()
{
	Socket s, c;
	s.list(LIST_PORT);
	output("Listening downloaders at port " + to_string(LIST_PORT));
	while (1)
	{
		c = s.acpt();
		if (c.sock() >= 0)
		{
			threads_l.lock();
			threads.push_back(thread(list_run, c));
			threads_l.unlock();
		}
	}
}

int main()
{
	threads_l.lock();
	threads.push_back(thread(register_port));
	threads.push_back(thread(list_port));
	threads_l.unlock();
	while (1);
}
