

proto:
	protoc --js_out=import_style=commonjs,binary:. ./../../../quic/core/proto/push_*.proto -I./../../..

certs:
	PWD=/home/user42/mc/quiche-work/src bash certs.sh

start_server:
	cd ./../../../../../../..
	./out/Default/epoll_quic_server --quic_ietf_draft   --quic_response_cache_dir=/tmp/quic-data/www.example.org   --certificate_file=net/tools/quic/certs/out/leaf_cert.pem   --key_file=net/tools/quic/certs/out/leaf_cert.pkcs8 --quic_multicast_upstream=root:/tmp/blah