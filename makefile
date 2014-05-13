all: judge_client.cc
	g++ -Wall -c judge_client.cc
	g++ -Wall -o judge_client judge_client.o
