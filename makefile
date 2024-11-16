mycmd:main.cpp
	g++ -o $@ $^ -std=c++11 -ljsoncpp -lmysqlcppconn

.PHONY:clean
clean:
	rm -rf mycmd testdir

.PHONEY:retry
retry:
	make clean
	make 
	./mycmd