# MACROS
CXX = g++
CXXFLAGS = -O3 -Wall -std=c++11
LIBS = -pthread -lssl -lcrypto
OBJS = server/server.o upload/upload.o tracker/tracker.o download/download.o
EXES = server/server upload/upload tracker/tracker download/download

# TARGETS
all: $(EXES)
	rm -rf */*.torrent */peer_list *.seg.*

server/server: server/server.o
	$(CXX) -o server/server server/server.o $(CXXFLAGS) $(LIBS)

upload/upload: upload/upload.o
	$(CXX) -o upload/upload upload/upload.o $(CXXFLAGS) $(LIBS)

tracker/tracker: tracker/tracker.o
	$(CXX) -o tracker/tracker tracker/tracker.o $(CXXFLAGS) $(LIBS)

download/download: download/download.o
	$(CXX) -o download/download download/download.o $(CXXFLAGS) $(LIBS)

server/server.o: server/server.cc util.h
	$(CXX) -o server/server.o -c server/server.cc $(CXXFLAGS) $(LIBS)

upload/upload.o: upload/upload.cc util.h
	$(CXX) -o upload/upload.o -c upload/upload.cc $(CXXFLAGS) $(LIBS)

tracker/tracker.o: tracker/tracker.cc util.h
	$(CXX) -o tracker/tracker.o -c tracker/tracker.cc $(CXXFLAGS) $(LIBS)

download/download.o: download/download.cc util.h
	$(CXX) -o download/download.o -c download/download.cc $(CXXFLAGS) $(LIBS)

# REMOVAL
clean:
	rm -rf $(OBJS) $(EXES) */*.torrent */peer_list */*.seg.*
