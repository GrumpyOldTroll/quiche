NAME=tester

test: tester
	./tester
	
$(NAME): *.cc ../../quic_server_push_interface.cc ../build/*.cc
	clang++ *.cc ../../quic_server_push_interface.cc ../build/*.cc -lprotobuf -o tester