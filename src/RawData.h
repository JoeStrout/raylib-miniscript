//
//  RawData.h
//  MSRLWeb
//
//  RawData class for MiniScript - provides access to raw binary buffers
//  Compatible with raylib APIs that work with malloc/free
//

#ifndef RAWDATA_H
#define RAWDATA_H

#include "MiniscriptTypes.h"

namespace MiniScript {

// BinaryData: manages a raw buffer with typed read/write access
class BinaryData {
public:
    unsigned char* bytes;
    int length;
    bool littleEndian;
    bool ownsBuffer;  // If true, we free the buffer on destruction

    // Constructor: creates a new buffer of the given size
    BinaryData(int size);

    // Constructor: wraps an existing buffer (takes ownership if own=true)
    BinaryData(unsigned char* buffer, int size, bool own);

    ~BinaryData();

    // Resize the buffer (only if we own it)
    void Resize(int newSize);

    // Transfer ownership of the buffer to external code
    // After this, we no longer free the buffer
    void ReleaseOwnership();

    // Take ownership of the buffer (we will free it)
    void TakeOwnership();

    // Typed read/write methods
    uint8_t GetUInt8(int offset) const;
    void SetUInt8(int offset, uint8_t value);

    int8_t GetInt8(int offset) const;
    void SetInt8(int offset, int8_t value);

    uint16_t GetUInt16(int offset) const;
    void SetUInt16(int offset, uint16_t value);

    int16_t GetInt16(int offset) const;
    void SetInt16(int offset, int16_t value);

    uint32_t GetUInt32(int offset) const;
    void SetUInt32(int offset, uint32_t value);

    int32_t GetInt32(int offset) const;
    void SetInt32(int offset, int32_t value);

    float GetFloat(int offset) const;
    void SetFloat(int offset, float value);

    double GetDouble(int offset) const;
    void SetDouble(int offset, double value);

    String GetUTF8(int offset, int byteCount) const;
    int SetUTF8(int offset, const String& value);

private:
    // Helper for byte swapping (when endianness doesn't match)
    uint16_t SwapUInt16(uint16_t value) const;
    uint32_t SwapUInt32(uint32_t value) const;
    uint64_t SwapUInt64(uint64_t value) const;
};

// Get the RawData class (MiniScript intrinsic class)
ValueDict RawDataClass();

// Convert between MiniScript Value and BinaryData
Value RawDataToValue(BinaryData* data);
BinaryData* ValueToRawData(Value value);

} // namespace MiniScript

#endif // RAWDATA_H
