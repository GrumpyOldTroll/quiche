pushd net/tools/quic/certs
./generate-certs.sh
popd
mkdir -p /tmp/quic-data
pushd /tmp/quic-data
wget -p --save-headers https://www.example.org

# manually edit index.html
# (as in https://www.chromium.org/quic/playing-with-quic/):
#   Remove (if it exists):
#     "Transfer-Encoding: chunked"
#     "Alternate-Protocol: ..."
#   Add:
#     X-Original-Url: https://www.example.org/
# note: it's very touchy about the url in X-Original-Url

popd