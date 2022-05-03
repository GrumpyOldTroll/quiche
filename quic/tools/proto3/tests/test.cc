#include <fstream>
#include <iostream>
#include <string>
#include "../../quic_server_push_interface.h"
using namespace std;

#define CATCH_CONFIG_MAIN
#include "catch.hpp"


TEST_CASE("WriteDelimitedTo and ReadDelimitedFrom") {
    ServerPushInterface::Top output_cmd;

    output_cmd.mutable_activate_pool()->set_pool_id(42);

    char buffer[256];

    google::protobuf::io::ArrayOutputStream output_stream(buffer, 256);

    bool write_success = ServerPushInterface::writeDelimitedTo(output_cmd, &output_stream);

    REQUIRE(write_success);

    ServerPushInterface::Top input_cmd;

    google::protobuf::io::ArrayInputStream input_stream(buffer, 256);

    ssize_t consumed;

    bool read_success = ServerPushInterface::readDelimitedFrom(&input_stream, &input_cmd, &consumed);

    REQUIRE(read_success);
    REQUIRE(consumed == output_cmd.ByteSizeLong() + 1);
    REQUIRE(consumed == input_cmd.ByteSizeLong() + 1);
    REQUIRE(input_cmd.activate_pool().pool_id() == 42);
}

TEST_CASE("ReadDelimitedFrom with message cutoff") {
    ServerPushInterface::Top output_cmd;

    output_cmd.mutable_activate_pool()->set_pool_id(42);

    char buffer[256];

    google::protobuf::io::ArrayOutputStream output_stream(buffer, 256);

    bool write_success = ServerPushInterface::writeDelimitedTo(output_cmd, &output_stream);

    REQUIRE(write_success);

    ServerPushInterface::Top input_cmd;

    // Simulate a cutoff in the message at the end - 2

    int cutoff = google::protobuf::io::CodedOutputStream::VarintSize32(output_cmd.ByteSizeLong()) + output_cmd.ByteSizeLong() - 2;

    google::protobuf::io::ArrayInputStream input_stream(buffer, cutoff);

    ssize_t consumed;

    bool read_success = ServerPushInterface::readDelimitedFrom(&input_stream, &input_cmd, &consumed);

    REQUIRE(consumed == -2);

    REQUIRE_FALSE(read_success);
}

