#pragma once

class QuicEpollUDSClient
{
    public:
        typedef std::vector<uint8_t> BufferType;
        typedef int FDType;
        typedef std::size_t size_t;
        
    public:
        BufferType buffer;
        FDType fd = 0;
        size_t filled_len = 0;

    public:
        QuicEpollUDSClient() = default;
        ~QuicEpollUDSClient() = default;
        QuicEpollUDSClient(const QuicEpollUDSClient&) = default;
        QuicEpollUDSClient& operator=(const QuicEpollUDSClient&) = default;
};