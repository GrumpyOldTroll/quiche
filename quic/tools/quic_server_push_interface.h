#include "proto3/build/Top.pb.h"

namespace ServerPushInterface {

bool writeDelimitedTo(const google::protobuf::MessageLite& message,
                      google::protobuf::io::ZeroCopyOutputStream* rawOutput);

bool readDelimitedFrom(google::protobuf::io::ZeroCopyInputStream* rawInput,
                       google::protobuf::MessageLite* message,
                       ssize_t* consumed = nullptr);

}