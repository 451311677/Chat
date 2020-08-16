svr_bin=./ChatServer
cli_bin=./ChatClient

LDFLAGS=-lpthread -ljson_linux-gcc-4.8.5_libmt

.PHONY:all
all:$(svr_bin) $(cli_bin)
$(svr_bin):ChatServer.cpp
	g++ -std=c++11 $^ -o $@ -g $(LDFLAGS)
$(cli_bin):ChatClient.cpp
	g++ -std=c++11 $^ -o $@ -g $(LDFLAGS) -lncurses

.PHONY:clean
clean:
	rm -f $(svr_bin) $(cli_bin)


