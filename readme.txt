This project consists of 4 components, server, tracker, downloader, and
uploader.

The operating system we use is Ubuntu 14.04 LTS, and the compiler is g++ 4.8.4.

To calculate the sha1 hash, we included OpenSSL into our project.

To compile the codes:
	make

To run the server:
	cd server/
	./server

To run the tracker:
	cd tracker/
	./tracker

To run the uploader:
	cd upload/
	./upload < [configure file]

	The configure file has the following format (See configure.upload):
	Path to the file to be uploaded
	Server's IP

To run the downloader:
	cd download/
	./download < [configure file]

	The configure file has the following format (See configure.download):
	Server's IP
	Torrent name

Notes:
	In this project, we assume the server and the tracker run in the same IP.
