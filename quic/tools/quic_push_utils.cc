#include "quic_push_utils.h"

namespace quic {

namespace PushUtils {

bool writeDelimitedTo(const google::protobuf::MessageLite& message,
                      google::protobuf::io::ZeroCopyOutputStream* rawOutput) {
  google::protobuf::io::CodedOutputStream output(rawOutput);

  // Write the size.
  const int size = message.ByteSizeLong();
  output.WriteVarint32(size);

  uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
  if (buffer != NULL) {
    // Optimization:  The message fits in one buffer, so use the faster
    // direct-to-array serialization path.
    message.SerializeWithCachedSizesToArray(buffer);
  } else {
    // Slightly-slower path when the message is multiple buffers.
    message.SerializeWithCachedSizes(&output);
    if (output.HadError())
      return false;
  }

  return true;
}

bool readDelimitedFrom(google::protobuf::io::ZeroCopyInputStream* rawInput,
                       google::protobuf::MessageLite* message,
                       ssize_t* consumed) {
  google::protobuf::io::CodedInputStream input(rawInput);

  // Read the size.
  uint32_t size;
  if (!input.ReadVarint32(&size)) {
    if (consumed)
      *consumed = -1;
    return false;
  }

  // Tell the stream not to read beyond that size.
  auto limit = input.PushLimit(size);

  // Parse the message.
  //if (!message->MergePartialFromCodedStream(&input) ||
  //    !input.ConsumedEntireMessage()) {
  if (!message->MergeFromCodedStream(&input)) {
    if (consumed)
      *consumed = -((ssize_t)size - (ssize_t)rawInput->ByteCount() + 1);
    return false;
  }

  // Release the limit.
  input.PopLimit(limit);

  if (consumed)
    *consumed =
        google::protobuf::io::CodedOutputStream::VarintSize32(size) + size;

  return true;
}

}  // namespace QuicPushUtils
}  // namespace quic
