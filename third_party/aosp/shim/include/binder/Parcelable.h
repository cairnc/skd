// Parcelable / Parcel shim — layerviewer never IPCs. Every read is a no-op
// that fills defaults; every write is a no-op returning OK. Lets upstream
// writeToParcel / readFromParcel bodies compile and link byte-identical.
#pragma once
#include <utils/Errors.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <utils/StrongPointer.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace android {

class IBinder;
class Parcel;
class Parcelable;

class Parcelable {
public:
  virtual ~Parcelable() = default;
  virtual status_t writeToParcel(Parcel * /*parcel*/) const { return OK; }
  virtual status_t readFromParcel(const Parcel * /*parcel*/) { return OK; }
};

class Parcel {
public:
  Parcel() = default;

  // writes — all no-ops
  status_t writeBool(bool) { return OK; }
  status_t writeInt32(int32_t) { return OK; }
  status_t writeUint32(uint32_t) { return OK; }
  status_t writeInt64(int64_t) { return OK; }
  status_t writeUint64(uint64_t) { return OK; }
  status_t writeFloat(float) { return OK; }
  status_t writeDouble(double) { return OK; }
  status_t writeByte(int8_t) { return OK; }
  status_t writeString8(const String8 &) { return OK; }
  status_t writeString8(const char *, size_t = 0) { return OK; }
  status_t writeString16(const String16 &) { return OK; }
  status_t writeStrongBinder(const sp<IBinder> &) { return OK; }
  status_t writeStrongBinder(const IBinder *) { return OK; }
  status_t writeUtf8AsUtf16(const std::string &) { return OK; }
  status_t writeUtf8AsUtf16(const char *) { return OK; }
  status_t writeByteVector(const std::vector<uint8_t> &) { return OK; }
  status_t writeByteVector(const std::vector<int8_t> &) { return OK; }
  status_t writeParcelable(const Parcelable &) { return OK; }
  status_t writeNullableParcelable(const Parcelable *) { return OK; }
  status_t writeBlob(size_t, bool, void *) { return OK; }
  status_t write(const void *, size_t) { return OK; }
  // Flattenable / user-type overload — FE passes Rect, etc.
  template <typename T,
            std::enable_if_t<!std::is_convertible_v<const T &, const void *>,
                             int> = 0>
  status_t write(const T &) {
    return OK;
  }

  template <typename T> status_t writeParcelableVector(const std::vector<T> &) {
    return OK;
  }
  template <typename T> status_t writeInt32Vector(const std::vector<T> &) {
    return OK;
  }
  template <typename T> status_t writeVector(const std::vector<T> &) {
    return OK;
  }

  // reads — fill defaults
  status_t readBool(bool *out) const {
    if (out)
      *out = false;
    return OK;
  }
  status_t readInt32(int32_t *out) const {
    if (out)
      *out = 0;
    return OK;
  }
  int32_t readInt32() const { return 0; }
  status_t readUint32(uint32_t *out) const {
    if (out)
      *out = 0;
    return OK;
  }
  uint32_t readUint32() const { return 0; }
  status_t readInt64(int64_t *out) const {
    if (out)
      *out = 0;
    return OK;
  }
  int64_t readInt64() const { return 0; }
  status_t readUint64(uint64_t *out) const {
    if (out)
      *out = 0;
    return OK;
  }
  uint64_t readUint64() const { return 0; }
  status_t readFloat(float *out) const {
    if (out)
      *out = 0;
    return OK;
  }
  float readFloat() const { return 0; }
  status_t readDouble(double *out) const {
    if (out)
      *out = 0;
    return OK;
  }
  status_t readByte(int8_t *out) const {
    if (out)
      *out = 0;
    return OK;
  }
  status_t readString8(String8 *) const { return OK; }
  status_t readString16(String16 *) const { return OK; }
  String16 readString16() const { return String16(); }
  status_t readStrongBinder(sp<IBinder> *) const { return OK; }
  sp<IBinder> readStrongBinder() const { return nullptr; }
  status_t readNullableStrongBinder(sp<IBinder> *) const { return OK; }
  status_t readUtf8FromUtf16(std::string *) const { return OK; }
  status_t readByteVector(std::vector<uint8_t> *) const { return OK; }
  status_t readByteVector(std::vector<int8_t> *) const { return OK; }
  status_t readParcelable(Parcelable *) const { return OK; }
  status_t read(void *, size_t) const { return OK; }
  const void *readInplace(size_t) const { return nullptr; }
  template <typename T,
            std::enable_if_t<!std::is_convertible_v<T &, void *>, int> = 0>
  status_t read(T &) const {
    return OK;
  }

  template <typename T> status_t readParcelableVector(std::vector<T> *) const {
    return OK;
  }
  template <typename T> status_t readInt32Vector(std::vector<T> *) const {
    return OK;
  }
  template <typename T> status_t readVector(std::vector<T> *) const {
    return OK;
  }
};

} // namespace android
