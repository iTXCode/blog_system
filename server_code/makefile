FLAG = -std=c++11 -L/usr/lib64/mysql -lmysqlclient -lpthread  -ljsoncpp 

.PHONY:all
all:db_test server

db_test:db_test.cc db.hpp
	g++ db_test.cc -o db_test $(FLAG)

server:blog_server.cc httplib.h
	g++ blog_server.cc -o server $(FLAG)

.PHONY:clean 

clean:
	rm db_test server
