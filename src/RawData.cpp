//
//  RawData.cpp
//  MSRLWeb
//
//  RawData class implementation
//

#include "RaylibTypes.h"
#include "RawData.h"
#include "miniscript.h"
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace MiniScript {

// MS2 has no exceptions: intrinsics report runtime errors via the VM and return.
// This helper mirrors the old RuntimeException/IndexException(...).raise() sites.
static IntrinsicResult raiseError(Context context, const char* msg) {
	context.vm.RaiseRuntimeError(msg);
	return IntrinsicResult::Null;
}

// Macro to reduce boilerplate
#define INTRINSIC_LAMBDA [](Context context, IntrinsicResult partialResult) -> IntrinsicResult

// Check if this system is little endian
static bool IsSystemLittleEndian() {
    uint16_t test = 0x0001;
    return *((uint8_t*)&test) == 0x01;
}

static const bool kSystemIsLittleEndian = IsSystemLittleEndian();

//--------------------------------------------------------------------------------
// BinaryData implementation
//--------------------------------------------------------------------------------

BinaryData::BinaryData(int size)
    : bytes(nullptr), length(size), littleEndian(true), ownsBuffer(true) {
    if (size > 0) {
        bytes = (unsigned char*)malloc(size);
        if (bytes == nullptr) {
            throw std::bad_alloc();
        }
        memset(bytes, 0, size);
    }
}

BinaryData::BinaryData(unsigned char* buffer, int size, bool own)
    : bytes(buffer), length(size), littleEndian(true), ownsBuffer(own) {
}

BinaryData::~BinaryData() {
    if (ownsBuffer && bytes != nullptr) {
        free(bytes);
        bytes = nullptr;
    }
}

void BinaryData::Resize(int newSize) {
    if (!ownsBuffer) {
        // Defensive: the resize intrinsic guards this and reports a script-level
        // error via the VM; we simply refuse the resize here.
        return;
    }

    if (newSize == length) return;

    if (newSize == 0) {
        if (bytes != nullptr) {
            free(bytes);
            bytes = nullptr;
        }
        length = 0;
        return;
    }

    unsigned char* newBytes = (unsigned char*)realloc(bytes, newSize);
    if (newBytes == nullptr) {
        throw std::bad_alloc();
    }

    // Zero out any new bytes
    if (newSize > length) {
        memset(newBytes + length, 0, newSize - length);
    }

    bytes = newBytes;
    length = newSize;
}

void BinaryData::ReleaseOwnership() {
    ownsBuffer = false;
}

void BinaryData::TakeOwnership() {
    ownsBuffer = true;
}

uint16_t BinaryData::SwapUInt16(uint16_t value) const {
    return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
}

uint32_t BinaryData::SwapUInt32(uint32_t value) const {
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8) |
           ((value & 0x0000FF00) << 8) |
           ((value & 0x000000FF) << 24);
}

uint64_t BinaryData::SwapUInt64(uint64_t value) const {
    return ((value & 0xFF00000000000000ULL) >> 56) |
           ((value & 0x00FF000000000000ULL) >> 40) |
           ((value & 0x0000FF0000000000ULL) >> 24) |
           ((value & 0x000000FF00000000ULL) >> 8) |
           ((value & 0x00000000FF000000ULL) << 8) |
           ((value & 0x0000000000FF0000ULL) << 24) |
           ((value & 0x000000000000FF00ULL) << 40) |
           ((value & 0x00000000000000FFULL) << 56);
}

uint8_t BinaryData::GetUInt8(int offset) const {
    return bytes[offset];
}

void BinaryData::SetUInt8(int offset, uint8_t value) {
    bytes[offset] = value;
}

int8_t BinaryData::GetInt8(int offset) const {
    return (int8_t)bytes[offset];
}

void BinaryData::SetInt8(int offset, int8_t value) {
    bytes[offset] = (uint8_t)value;
}

uint16_t BinaryData::GetUInt16(int offset) const {
    uint16_t value;
    memcpy(&value, bytes + offset, sizeof(uint16_t));
    if (littleEndian != kSystemIsLittleEndian) {
        value = SwapUInt16(value);
    }
    return value;
}

void BinaryData::SetUInt16(int offset, uint16_t value) {
    if (littleEndian != kSystemIsLittleEndian) {
        value = SwapUInt16(value);
    }
    memcpy(bytes + offset, &value, sizeof(uint16_t));
}

int16_t BinaryData::GetInt16(int offset) const {
    return (int16_t)GetUInt16(offset);
}

void BinaryData::SetInt16(int offset, int16_t value) {
    SetUInt16(offset, (uint16_t)value);
}

uint32_t BinaryData::GetUInt32(int offset) const {
    uint32_t value;
    memcpy(&value, bytes + offset, sizeof(uint32_t));
    if (littleEndian != kSystemIsLittleEndian) {
        value = SwapUInt32(value);
    }
    return value;
}

void BinaryData::SetUInt32(int offset, uint32_t value) {
    if (littleEndian != kSystemIsLittleEndian) {
        value = SwapUInt32(value);
    }
    memcpy(bytes + offset, &value, sizeof(uint32_t));
}

int32_t BinaryData::GetInt32(int offset) const {
    return (int32_t)GetUInt32(offset);
}

void BinaryData::SetInt32(int offset, int32_t value) {
    SetUInt32(offset, (uint32_t)value);
}

float BinaryData::GetFloat(int offset) const {
    uint32_t bits = GetUInt32(offset);
    float value;
    memcpy(&value, &bits, sizeof(float));
    return value;
}

void BinaryData::SetFloat(int offset, float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(float));
    SetUInt32(offset, bits);
}

double BinaryData::GetDouble(int offset) const {
    uint64_t bits;
    memcpy(&bits, bytes + offset, sizeof(uint64_t));
    if (littleEndian != kSystemIsLittleEndian) {
        bits = SwapUInt64(bits);
    }
    double value;
    memcpy(&value, &bits, sizeof(double));
    return value;
}

void BinaryData::SetDouble(int offset, double value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof(double));
    if (littleEndian != kSystemIsLittleEndian) {
        bits = SwapUInt64(bits);
    }
    memcpy(bytes + offset, &bits, sizeof(uint64_t));
}

String BinaryData::GetUTF8(int offset, int byteCount) const {
    if (byteCount <= 0) return String();

    // Find null terminator if present
    int actualCount = 0;
    for (int i = 0; i < byteCount && offset + i < length; i++) {
        if (bytes[offset + i] == 0) break;
        actualCount++;
    }

    if (actualCount == 0) return String();

    // Create string from UTF-8 bytes
    return String((const char*)(bytes + offset), actualCount);
}

int BinaryData::SetUTF8(int offset, const String& value) {
    if (value.empty()) return 0;

    int bytesToCopy = value.LengthB();
    if (offset + bytesToCopy > length) {
        bytesToCopy = length - offset;
    }

    if (bytesToCopy > 0) {
        memcpy(bytes + offset, value.c_str(), bytesToCopy);
    }

    return bytesToCopy;
}

//--------------------------------------------------------------------------------
// MiniScript RawData class
//--------------------------------------------------------------------------------

static const Value& kHandle() { static Value v("_handle"); return v; }
static const Value& kLittleEndian() { static Value v("littleEndian"); return v; }

// Helper: get BinaryData from a RawData object
static BinaryData* GetBinaryData(Context context) {
    Value self = context.GetVar(String("self"));
    if (self.Type() != ValueType::Map) {
        { context.vm.RaiseRuntimeError("RawData required for self parameter"); return nullptr; }
    }

    ValueDict map = self.GetDict();
    Value handleVal = map.Lookup(kHandle(), Value::Null);

    if (handleVal.Type() != ValueType::Number) {
        return nullptr;
    }

    BinaryData* data = (BinaryData*)ValueToPointer(handleVal);

    // Update littleEndian from the map
    Value leVal = map.Lookup(kLittleEndian(), Value::one);
    if (data != nullptr) {
        data->littleEndian = leVal.BoolValue();
    }

    return data;
}

ValueDict& RawDataClass() {
    static ValueDict rawDataClass;

    if (rawDataClass.Count() > 0) return rawDataClass;

    rawDataClass.SetValue(kHandle(), Value::Null);
    rawDataClass.SetValue(kLittleEndian(), Value::one);

    Intrinsic f;

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.set_Code(INTRINSIC_LAMBDA {
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return IntrinsicResult(Value::zero);
        return IntrinsicResult(Value(data->length));
    });
    // get the size of this RawData object in bytes
    rawDataClass.SetValue(String("len"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("bytes", 32);
    f.set_Code(INTRINSIC_LAMBDA {
        int newSize = context.GetVar(String("bytes")).IntValue();
        if (newSize < 0) {
            return raiseError(context, "bytes parameter must be >= 0");
        }

        Value self = context.GetVar(String("self"));
        ValueDict map = self.GetDict();
        BinaryData* oldData = GetBinaryData(context);

        if (oldData != nullptr && oldData->length == newSize) {
            return IntrinsicResult::Null;
        }

        if (newSize == 0) {
            // Delete old data
            if (oldData != nullptr) delete oldData;
            map.SetValue(kHandle(), Value::Null);
            return IntrinsicResult::Null;
        }

        // Create new data or resize existing
        if (oldData == nullptr) {
            BinaryData* newData = new BinaryData(newSize);
            map.SetValue(kHandle(), Value((double)(intptr_t)newData));
        } else {
            if (!oldData->ownsBuffer) return raiseError(context, "Cannot resize RawData buffer that we don't own");
            oldData->Resize(newSize);
        }

        return IntrinsicResult::Null;
    });
    // resize this RawData object to the given number of bytes
    rawDataClass.SetValue(String("resize"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset >= data->length) return raiseError(context, "index out of range");

        return IntrinsicResult(data->GetUInt8(offset));
    });
    // get unsigned byte (0-255) at the given byte offset
    rawDataClass.SetValue(String("byte"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.AddParam("value", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        int value = context.GetVar(String("value")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset >= data->length) return raiseError(context, "index out of range");

        data->SetUInt8(offset, (uint8_t)value);
        return IntrinsicResult::Null;
    });
    // set unsigned byte at the given byte offset
    rawDataClass.SetValue(String("setByte"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset >= data->length) return raiseError(context, "index out of range");

        return IntrinsicResult(data->GetInt8(offset));
    });
    // get signed byte (-128 to 127) at the given byte offset
    rawDataClass.SetValue(String("sbyte"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.AddParam("value", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        int value = context.GetVar(String("value")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset >= data->length) return raiseError(context, "index out of range");

        data->SetInt8(offset, (int8_t)value);
        return IntrinsicResult::Null;
    });
    // set signed byte at the given byte offset
    rawDataClass.SetValue(String("setSbyte"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset + 2 > data->length) return raiseError(context, "index out of range");

        return IntrinsicResult(data->GetUInt16(offset));
    });
    // get unsigned 16-bit integer (0-65535) at the given byte offset
    rawDataClass.SetValue(String("ushort"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.AddParam("value", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        int value = context.GetVar(String("value")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset + 2 > data->length) return raiseError(context, "index out of range");

        data->SetUInt16(offset, (uint16_t)value);
        return IntrinsicResult::Null;
    });
    // set unsigned 16-bit integer at the given byte offset
    rawDataClass.SetValue(String("setUshort"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset + 2 > data->length) return raiseError(context, "index out of range");

        return IntrinsicResult(data->GetInt16(offset));
    });
    // get signed 16-bit integer (-32768 to 32767) at the given byte offset
    rawDataClass.SetValue(String("short"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.AddParam("value", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        int value = context.GetVar(String("value")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset + 2 > data->length) return raiseError(context, "index out of range");

        data->SetInt16(offset, (int16_t)value);
        return IntrinsicResult::Null;
    });
    // set signed 16-bit integer at the given byte offset
    rawDataClass.SetValue(String("setShort"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset + 4 > data->length) return raiseError(context, "index out of range");

        return IntrinsicResult((double)data->GetUInt32(offset));
    });
    // get unsigned 32-bit integer at the given byte offset
    rawDataClass.SetValue(String("uint"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.AddParam("value", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        double value = context.GetVar(String("value")).DoubleValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset + 4 > data->length) return raiseError(context, "index out of range");

        data->SetUInt32(offset, (uint32_t)value);
        return IntrinsicResult::Null;
    });
    // set unsigned 32-bit integer at the given byte offset
    rawDataClass.SetValue(String("setUint"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset + 4 > data->length) return raiseError(context, "index out of range");

        return IntrinsicResult(data->GetInt32(offset));
    });
    // get signed 32-bit integer at the given byte offset
    rawDataClass.SetValue(String("int"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.AddParam("value", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        int value = context.GetVar(String("value")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset + 4 > data->length) return raiseError(context, "index out of range");

        data->SetInt32(offset, value);
        return IntrinsicResult::Null;
    });
    // set signed 32-bit integer at the given byte offset
    rawDataClass.SetValue(String("setInt"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset + 4 > data->length) return raiseError(context, "index out of range");

        return IntrinsicResult(data->GetFloat(offset));
    });
    // get 32-bit float at the given byte offset
    rawDataClass.SetValue(String("float"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.AddParam("value", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        float value = context.GetVar(String("value")).FloatValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset + 4 > data->length) return raiseError(context, "index out of range");

        data->SetFloat(offset, value);
        return IntrinsicResult::Null;
    });
    // set 32-bit float at the given byte offset
    rawDataClass.SetValue(String("setFloat"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset + 8 > data->length) return raiseError(context, "index out of range");

        return IntrinsicResult(data->GetDouble(offset));
    });
    // get 64-bit double at the given byte offset
    rawDataClass.SetValue(String("double"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.AddParam("value", 0);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        double value = context.GetVar(String("value")).DoubleValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset + 8 > data->length) return raiseError(context, "index out of range");

        data->SetDouble(offset, value);
        return IntrinsicResult::Null;
    });
    // set 64-bit double at the given byte offset
    rawDataClass.SetValue(String("setDouble"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.AddParam("bytes", -1);
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        int byteCount = context.GetVar(String("bytes")).IntValue();
        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (byteCount < 0) byteCount = data->length - offset;
        if (offset < 0 || byteCount < 0) return raiseError(context, "index out of range");

        return IntrinsicResult(data->GetUTF8(offset, byteCount));
    });
    // get UTF-8 string from the given byte offset (bytes=-1 reads to end)
    rawDataClass.SetValue(String("utf8"), f.GetFunc());

    f = Intrinsic::Create("");
    f.AddParam("self");
    f.AddParam("offset", 0);
    f.AddParam("value", "");
    f.set_Code(INTRINSIC_LAMBDA {
        int offset = context.GetVar(String("offset")).IntValue();
        String value = context.GetVar(String("value")).ToString();
        if (value.empty()) return IntrinsicResult(Value::zero);

        BinaryData* data = GetBinaryData(context);
        if (data == nullptr) return raiseError(context, "index out of range");

        if (offset < 0) offset += data->length;
        if (offset < 0 || offset >= data->length) return raiseError(context, "index out of range");

        return IntrinsicResult(data->SetUTF8(offset, value));
    });
    // write a UTF-8 string at the given byte offset; returns bytes written
    rawDataClass.SetValue(String("setUtf8"), f.GetFunc());

    return rawDataClass;
}

Value RawDataToValue(BinaryData* data) {
    if (data == nullptr) return Value::Null;

    ValueDict map = RawDataClass();
    map.SetValue(kHandle(), Value((double)(intptr_t)data));
    map.SetValue(kLittleEndian(), Value(data->littleEndian ? 1.0 : 0.0));
    return DynamicMap(map);
}

BinaryData* ValueToRawData(Value value) {
    if (value.Type() != ValueType::Map) return nullptr;

    ValueDict map = value.GetDict();
    Value handleVal = map.Lookup(kHandle(), Value::Null);

    if (handleVal.Type() != ValueType::Number) return nullptr;

    BinaryData* data = (BinaryData*)ValueToPointer(handleVal);

    // Update littleEndian setting
    if (data != nullptr) {
        Value leVal = map.Lookup(kLittleEndian(), Value::one);
        data->littleEndian = leVal.BoolValue();
    }

    return data;
}

} // namespace MiniScript
