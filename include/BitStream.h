#ifndef _DUMMY_BITSTREAM_H
#define _DUMMY_BITSTREAM_H

#include <stddef.h>

namespace SLNet {
    class BitStream {
    public:
        BitStream() {}
        // The missing signature matching packet->data, packet->length, false
        BitStream(unsigned char* data, unsigned int length, bool copyData) {}
        
        void Write(const char* str) {}
        void Read(char* str) {}
        void IgnoreBits(int numberOfBits) {}
        void IgnoreBytes(int numberOfBytes) {}
        
        template <typename T>
        void Write(T data) {}
        
        template <typename T>
        void Read(T &data) {}
    };
}

#endif
