/*
MIT License

Copyright (c) 2019 Vilya Harvey

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "miniply.h"

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#ifndef _WIN32
#include <errno.h>
#endif


namespace miniply {

  //
  // Public constants
  //

  // Standard PLY element names
  const char* kPLYVertexElement = "vertex";
  const char* kPLYFaceElement = "face";


  //
  // PLY constants
  //

  static constexpr uint32_t kPLYReadBufferSize = 128 * 1024;
  static constexpr uint32_t kPLYTempBufferSize = kPLYReadBufferSize;

  static const char* kPLYFileTypes[] = { "ascii", "binary_little_endian", "binary_big_endian", nullptr };
  static const uint32_t kPLYPropertySize[]= { 1, 1, 2, 2, 4, 4, 4, 8 };

  struct PLYTypeAlias {
    const char* name;
    PLYPropertyType type;
  };

  static const PLYTypeAlias kTypeAliases[] = {
    { "char",   PLYPropertyType::Char   },
    { "uchar",  PLYPropertyType::UChar  },
    { "short",  PLYPropertyType::Short  },
    { "ushort", PLYPropertyType::UShort },
    { "int",    PLYPropertyType::Int    },
    { "uint",   PLYPropertyType::UInt   },
    { "float",  PLYPropertyType::Float  },
    { "float32",PLYPropertyType::Float  },
    { "float64",PLYPropertyType::Double  },
    { "double", PLYPropertyType::Double },

    { "uint8",  PLYPropertyType::UChar  },
    { "uint16", PLYPropertyType::UShort },
    { "uint32", PLYPropertyType::UInt   },

    { "int8",   PLYPropertyType::Char   },
    { "int16",  PLYPropertyType::Short  },
    { "int32",  PLYPropertyType::Int    },

    { nullptr,  PLYPropertyType::None   }
  };


  //
  // Constants
  //

  static constexpr double kDoubleDigits[10] = { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };

  static constexpr float kPi = 3.14159265358979323846f;


  //
  // Vec2 type
  //

  struct Vec2 {
    float x, y;
  };

  static inline Vec2 operator - (Vec2 lhs, Vec2 rhs) { return Vec2{ lhs.x - rhs.x, lhs.y - rhs.y }; }

  static inline float dot(Vec2 lhs, Vec2 rhs) { return lhs.x * rhs.x + lhs.y * rhs.y; }
  static inline float length(Vec2 v) { return std::sqrt(dot(v, v)); }
  static inline Vec2 normalize(Vec2 v) { float len = length(v); return Vec2{ v.x / len, v.y / len }; }


  //
  // Vec3 type
  //

  struct Vec3 {
    float x, y, z;
  };

  static inline Vec3 operator - (Vec3 lhs, Vec3 rhs) { return Vec3{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z }; }

  static inline float dot(Vec3 lhs, Vec3 rhs) { return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z; }
  static inline float length(Vec3 v) { return std::sqrt(dot(v, v)); }
  static inline Vec3 normalize(Vec3 v) { float len = length(v); return Vec3{ v.x / len, v.y / len, v.z / len }; }
  static inline Vec3 cross(Vec3 lhs, Vec3 rhs) { return Vec3{ lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x }; }


  //
  // Internal-only functions
  //

  static inline bool is_whitespace(char ch)
  {
    return ch == ' ' || ch == '\t' || ch == '\r';
  }


  static inline bool is_digit(char ch)
  {
    return ch >= '0' && ch <= '9';
  }


  static inline bool is_letter(char ch)
  {
    ch |= 32; // upper and lower case letters differ only at this bit.
    return ch >= 'a' && ch <= 'z';
  }


  static inline bool is_alnum(char ch)
  {
    return is_digit(ch) || is_letter(ch);
  }


  static inline bool is_keyword_start(char ch)
  {
    return is_letter(ch) || ch == '_';
  }


  static inline bool is_keyword_part(char ch)
  {
    return is_alnum(ch) || ch == '_';
  }


  static inline bool is_safe_buffer_end(char ch)
  {
    return (ch > 0 && ch <= 32) || (ch >= 127);
  }


  static int file_open(FILE** f, const char* filename, const char* mode)
  {
  #ifdef _WIN32
    return fopen_s(f, filename, mode);
  #else
    *f = fopen(filename, mode);
    return (*f != nullptr) ? 0 : errno;
  #endif
  }


  static inline int file_seek(FILE* file, int64_t offset, int origin)
  {
  #ifdef _WIN32
    return _fseeki64(file, offset, origin);
  #else
    static_assert(sizeof(off_t) == sizeof(int64_t), "off_t is not 64 bits.");
    return fseeko(file, offset, origin);
  #endif
  }


  static bool int_literal(const char* start, char const** end, int* val)
  {
    const char* pos = start;

    bool negative = false;
    if (*pos == '-') {
      negative = true;
      ++pos;
    }
    else if (*pos == '+') {
      ++pos;
    }

    bool hasLeadingZeroes = *pos == '0';
    if (hasLeadingZeroes) {
      do {
        ++pos;
      } while (*pos == '0');
    }

    int numDigits = 0;
    int localVal = 0;
    while (is_digit(*pos)) {
      // FIXME: this will overflow if we get too many digits.
      localVal = localVal * 10 + static_cast<int>(*pos - '0');
      ++numDigits;
      ++pos;
    }

    if (numDigits == 0 && hasLeadingZeroes) {
      numDigits = 1;
    }

    if (numDigits == 0 || is_letter(*pos) || *pos == '_') {
      return false;
    }
    else if (numDigits > 10) {
      // Overflow, literal value is larger than an int can hold.
      // FIXME: this won't catch *all* cases of overflow, make it exact.
      return false;
    }

    if (val != nullptr) {
      *val = negative ? -localVal : localVal;
    }
    if (end != nullptr) {
      *end = pos;
    }
    return true;
  }


  static bool double_literal(const char* start, char const** end, double* val)
  {
    const char* pos = start;

    bool negative = false;
    if (*pos == '-') {
      negative = true;
      ++pos;
    }
    else if (*pos == '+') {
      ++pos;
    }

    double localVal = 0.0;

    bool hasIntDigits = is_digit(*pos);
    if (hasIntDigits) {
      do {
        localVal = localVal * 10.0 + kDoubleDigits[*pos - '0'];
        ++pos;
      } while (is_digit(*pos));
    }
    else if (*pos != '.') {
//      set_error("Not a floating point number");
      return false;
    }

    bool hasFracDigits = false;
    if (*pos == '.') {
      ++pos;
      hasFracDigits = is_digit(*pos);
      if (hasFracDigits) {
        double scale = 0.1;
        do {
          localVal += scale * kDoubleDigits[*pos - '0'];
          scale *= 0.1;
          ++pos;
        } while (is_digit(*pos));
      }
      else if (!hasIntDigits) {
//        set_error("Floating point number has no digits before or after the decimal point");
        return false;
      }
    }

    bool hasExponent = *pos == 'e' || *pos == 'E';
    if (hasExponent) {
      ++pos;
      bool negativeExponent = false;
      if (*pos == '-') {
        negativeExponent = true;
        ++pos;
      }
      else if (*pos == '+') {
        ++pos;
      }

      if (!is_digit(*pos)) {
//        set_error("Floating point exponent has no digits");
        return false; // error: exponent part has no digits.
      }

      double exponent = 0.0;
      do {
        exponent = exponent * 10.0 + kDoubleDigits[*pos - '0'];
        ++pos;
      } while (is_digit(*pos));

      if (val != nullptr) {
        if (negativeExponent) {
          exponent = -exponent;
        }
        localVal *= std::pow(10.0, exponent);
      }
    }

    if (*pos == '.' || *pos == '_' || is_alnum(*pos)) {
//      set_error("Floating point number has trailing chars");
      return false;
    }

    if (negative) {
      localVal = -localVal;
    }

    if (val != nullptr) {
      *val = localVal;
    }
    if (end != nullptr) {
      *end = pos;
    }
    return true;
  }


  static bool float_literal(const char* start, char const** end, float* val)
  {
    double tmp = 0.0;
    bool ok = double_literal(start, end, &tmp);
    if (ok && val != nullptr) {
      *val = static_cast<float>(tmp);
    }
    return ok;
  }


  static inline void endian_swap_2(uint8_t* data)
  {
    uint16_t tmp = *reinterpret_cast<uint16_t*>(data);
    tmp = static_cast<uint16_t>((tmp >> 8) | (tmp << 8));
    *reinterpret_cast<uint16_t*>(data) = tmp;
  }


  static inline void endian_swap_4(uint8_t* data)
  {
    uint32_t tmp = *reinterpret_cast<uint32_t*>(data);
    tmp = (tmp >> 16) | (tmp << 16);
    tmp = ((tmp & 0xFF00FF00) >> 8) | ((tmp & 0x00FF00FF) << 8);
    *reinterpret_cast<uint32_t*>(data) = tmp;
  }


  static inline void endian_swap_8(uint8_t* data)
  {
    uint64_t tmp = *reinterpret_cast<uint64_t*>(data);
    tmp = (tmp >> 32) | (tmp << 32);
    tmp = ((tmp & 0xFFFF0000FFFF0000) >> 16) | ((tmp & 0x0000FFFF0000FFFF) << 16);
    tmp = ((tmp & 0xFF00FF00FF00FF00) >> 8) | ((tmp & 0x00FF00FF00FF00FF) << 8);
    *reinterpret_cast<uint64_t*>(data) = tmp;
  }


  static inline void endian_swap(uint8_t* data, PLYPropertyType type)
  {
    switch (kPLYPropertySize[uint32_t(type)]) {
    case 2: endian_swap_2(data); break;
    case 4: endian_swap_4(data); break;
    case 8: endian_swap_8(data); break;
    default: break;
    }
  }


  static inline void endian_swap_array(uint8_t* data, PLYPropertyType type, int n)
  {
    switch (kPLYPropertySize[uint32_t(type)]) {
    case 2:
      for (const uint8_t* end = data + 2 * n; data < end; data += 2) {
        endian_swap_2(data);
      }
      break;
    case 4:
      for (const uint8_t* end = data + 4 * n; data < end; data += 4) {
        endian_swap_4(data);
      }
      break;
    case 8:
      for (const uint8_t* end = data + 8 * n; data < end; data += 8) {
        endian_swap_8(data);
      }
      break;
    default:
      break;
    }
  }


  template <class T>
  static void copy_and_convert_to(T* dest, const uint8_t* src, PLYPropertyType srcType)
  {
    switch (srcType) {
    case PLYPropertyType::Char:   *dest = static_cast<T>(*reinterpret_cast<const int8_t*>(src)); break;
    case PLYPropertyType::UChar:  *dest = static_cast<T>(*reinterpret_cast<const uint8_t*>(src)); break;
    case PLYPropertyType::Short:  *dest = static_cast<T>(*reinterpret_cast<const int16_t*>(src)); break;
    case PLYPropertyType::UShort: *dest = static_cast<T>(*reinterpret_cast<const uint16_t*>(src)); break;
    case PLYPropertyType::Int:    *dest = static_cast<T>(*reinterpret_cast<const int*>(src)); break;
    case PLYPropertyType::UInt:   *dest = static_cast<T>(*reinterpret_cast<const uint32_t*>(src)); break;
    case PLYPropertyType::Float:  *dest = static_cast<T>(*reinterpret_cast<const float*>(src)); break;
    case PLYPropertyType::Double: *dest = static_cast<T>(*reinterpret_cast<const double*>(src)); break;
    case PLYPropertyType::None:   break;
    }
  }


  static void copy_and_convert(uint8_t* dest, PLYPropertyType destType, const uint8_t* src, PLYPropertyType srcType)
  {
    switch (destType) {
    case PLYPropertyType::Char:   copy_and_convert_to(reinterpret_cast<int8_t*>  (dest), src, srcType); break;
    case PLYPropertyType::UChar:  copy_and_convert_to(reinterpret_cast<uint8_t*> (dest), src, srcType); break;
    case PLYPropertyType::Short:  copy_and_convert_to(reinterpret_cast<int16_t*> (dest), src, srcType); break;
    case PLYPropertyType::UShort: copy_and_convert_to(reinterpret_cast<uint16_t*>(dest), src, srcType); break;
    case PLYPropertyType::Int:    copy_and_convert_to(reinterpret_cast<int32_t*> (dest), src, srcType); break;
    case PLYPropertyType::UInt:   copy_and_convert_to(reinterpret_cast<uint32_t*>(dest), src, srcType); break;
    case PLYPropertyType::Float:  copy_and_convert_to(reinterpret_cast<float*>   (dest), src, srcType); break;
    case PLYPropertyType::Double: copy_and_convert_to(reinterpret_cast<double*>  (dest), src, srcType); break;
    case PLYPropertyType::None:   break;
    }
  }


  static inline bool compatible_types(PLYPropertyType srcType, PLYPropertyType destType)
  {
    return (srcType == destType) ||
        (srcType < PLYPropertyType::Float && (uint32_t(srcType) ^ 0x1) == uint32_t(destType));
  }


  //
  // PLYElement methods
  //

  void PLYElement::calculate_offsets()
  {
    fixedSize = true;
    for (PLYProperty& prop : properties) {
      if (prop.countType != PLYPropertyType::None) {
        fixedSize = false;
        break;
      }
    }

    // Note that each list property gets its own separate storage. Only fixed
    // size properties go into the common data block. The `rowStride` is the
    // size of a row in the common data block.
    rowStride = 0;
    for (PLYProperty& prop : properties) {
      if (prop.countType != PLYPropertyType::None) {
        continue;
      }
      prop.offset = rowStride;
      rowStride += kPLYPropertySize[uint32_t(prop.type)];
    }
  }


  uint32_t PLYElement::find_property(const char *propName) const
  {
    for (uint32_t i = 0, endI = uint32_t(properties.size()); i < endI; i++) {
      if (strcmp(propName, properties.at(i).name.c_str()) == 0) {
        return i;
      }
    }
    return kInvalidIndex;
  }


  bool PLYElement::find_properties(uint32_t propIdxs[], uint32_t numIdxs, ...) const
  {
    va_list args;
    va_start(args, numIdxs);
    bool foundAll = find_properties_va(propIdxs, numIdxs, args);
    va_end(args);
    return foundAll;
  }


  bool PLYElement::find_properties_va(uint32_t propIdxs[], uint32_t numIdxs, va_list names) const
  {
    for (uint32_t i = 0; i < numIdxs; i++) {
      propIdxs[i] = find_property(va_arg(names, const char*));
      if (propIdxs[i] == kInvalidIndex) {
        return false;
      }
    }
    return true;
  }


  bool PLYElement::convert_list_to_fixed_size(uint32_t listPropIdx, uint32_t listSize, uint32_t newPropIdxs[])
  {
    if (fixedSize || listPropIdx >= properties.size() || properties[listPropIdx].countType == PLYPropertyType::None) {
      return false;
    }

    PLYProperty oldListProp = properties[listPropIdx];

    // If the generated names are less than 256 chars, we will use an array on
    // the stack as temporary storage. In the rare case that they're longer,
    // we'll allocate an array of sufficient size on the heap and use that
    // instead. This means we'll avoid allocating in all but the most extreme
    // cases.
    char inlineBuf[256];
    size_t nameBufSize = oldListProp.name.size() + 12; // the +12 allows space for an '_', a number up to 10 digits long and the terminating null.
    char* nameBuf = inlineBuf;
    if (nameBufSize > sizeof(inlineBuf)) {
      nameBuf = new char[nameBufSize];
    }

    // Set up a property for the list count column.
    PLYProperty& countProp = properties[listPropIdx];
    snprintf(nameBuf, nameBufSize, "%s_count", oldListProp.name.c_str());
    countProp.name = nameBuf;
    countProp.type = oldListProp.countType;
    countProp.countType = PLYPropertyType::None;
    countProp.stride = kPLYPropertySize[uint32_t(oldListProp.countType)];

    if (listSize > 0) {
      // Set up additional properties for the list entries, 1 per entry.
      if (listPropIdx + 1 == properties.size()) {
        properties.resize(properties.size() + listSize);
      }
      else {
        properties.insert(properties.begin() + listPropIdx + 1, listSize, PLYProperty());
      }

      for (uint32_t i = 0; i < listSize; i++) {
        uint32_t propIdx = listPropIdx + 1 + i;

        PLYProperty& itemProp = properties[propIdx];
        snprintf(nameBuf, nameBufSize, "%s_%u", oldListProp.name.c_str(), i);
        itemProp.name = nameBuf;
        itemProp.type = oldListProp.type;
        itemProp.countType = PLYPropertyType::None;
        itemProp.stride = kPLYPropertySize[uint32_t(oldListProp.type)];

        newPropIdxs[i] = propIdx;
      }
    }

    if (nameBuf != inlineBuf) {
      delete[] nameBuf;
    }

    calculate_offsets();
    return true;
  }


  //
  // PLYReader methods
  //

  PLYReader::PLYReader(const char* filename)
  {
    m_buf = new char[kPLYReadBufferSize + 1];
    m_buf[kPLYReadBufferSize] = '\0';

    m_tmpBuf = new char[kPLYTempBufferSize + 1];
    m_tmpBuf[kPLYTempBufferSize] = '\0';

    m_bufEnd = m_buf + kPLYReadBufferSize;
    m_pos = m_bufEnd;
    m_end = m_bufEnd;

    if (file_open(&m_f, filename, "rb") != 0) {
      m_f = nullptr;
      m_valid = false;
      return;
    }
    m_valid = true;

    refill_buffer();

    m_valid = keyword("ply") && next_line() &&
              keyword("format") && advance() &&
              typed_which(kPLYFileTypes, &m_fileType) && advance() &&
              int_literal(&m_majorVersion) && advance() &&
              match(".") && advance() &&
              int_literal(&m_minorVersion) && next_line() &&
              parse_elements() &&
              keyword("end_header") && advance() && match("\n") && accept();
    if (!m_valid) {
      return;
    }
    m_inDataSection = true;
    if (m_fileType == PLYFileType::ASCII) {
      advance();
    }

    for (PLYElement& elem : m_elements) {
      elem.calculate_offsets();
    }
  }


  PLYReader::~PLYReader()
  {
    if (m_f != nullptr) {
      fclose(m_f);
    }
    delete[] m_buf;
    delete[] m_tmpBuf;
  }


  bool PLYReader::valid() const
  {
    return m_valid;
  }


  bool PLYReader::has_element() const
  {
    return m_valid && m_currentElement < m_elements.size();
  }


  const PLYElement* PLYReader::element() const
  {
    assert(has_element());
    return &m_elements[m_currentElement];
  }


  bool PLYReader::load_element()
  {
    assert(has_element());
    if (m_elementLoaded) {
      return true;
    }

    PLYElement& elem = m_elements[m_currentElement];
    return elem.fixedSize ? load_fixed_size_element(elem) : load_variable_size_element(elem);
  }


  void PLYReader::next_element()
  {
    if (!has_element()) {
      return;
    }

    // If the element was loaded, the read buffer should already be positioned at
    // the start of the next element.
    PLYElement& elem = m_elements[m_currentElement];
    m_currentElement++;

    if (m_elementLoaded) {
      // Clear any temporary storage used for list properties in the current element.
      for (PLYProperty& prop : elem.properties) {
        if (prop.countType == PLYPropertyType::None) {
          continue;
        }
        prop.listData.clear();
        prop.listData.shrink_to_fit();
        prop.rowCount.clear();
        prop.rowCount.shrink_to_fit();
      }

      // Clear temporary storage for the non-list properties in the current element.
      m_elementData.clear();
      m_elementLoaded = false;
      return;
    }

    // If the element wasn't loaded, we have to move the file pointer past its
    // contents. How we do that depends on whether this is an ASCII or binary
    // file and, if it's a binary, whether the element is fixed or variable
    // size.
    if (m_fileType == PLYFileType::ASCII) {
      for (uint32_t row = 0; row < elem.count; row++) {
        next_line();
      }
    }
    else if (elem.fixedSize) {
      int64_t elementStart = static_cast<int64_t>(m_pos - m_buf);
      int64_t elementSize = elem.rowStride * elem.count;
      int64_t elementEnd = elementStart + elementSize;
      if (elementEnd >= kPLYReadBufferSize) {
        m_bufOffset += elementEnd;
        file_seek(m_f, m_bufOffset, SEEK_SET);
        m_bufEnd = m_buf + kPLYReadBufferSize;
        m_pos = m_bufEnd;
        m_end = m_bufEnd;
        refill_buffer();
      }
      else {
        m_pos = m_buf + elementEnd;
        m_end = m_pos;
      }
    }
    else if (m_fileType == PLYFileType::Binary) {
      for (uint32_t row = 0; row < elem.count; row++) {
        for (const PLYProperty& prop : elem.properties) {
          if (prop.countType == PLYPropertyType::None) {
            uint32_t numBytes = kPLYPropertySize[uint32_t(prop.type)];
            if (m_pos + numBytes > m_bufEnd) {
              if (!refill_buffer() || m_pos + numBytes > m_bufEnd) {
                m_valid = false;
                return;
              }
            }
            m_pos += numBytes;
            m_end = m_pos;
            continue;
          }

          uint32_t numBytes = kPLYPropertySize[uint32_t(prop.countType)];
          if (m_pos + numBytes > m_bufEnd) {
            if (!refill_buffer() || m_pos + numBytes > m_bufEnd) {
              m_valid = false;
              return;
            }
          }

          int count = 0;
          copy_and_convert_to(&count, reinterpret_cast<const uint8_t*>(m_pos), prop.countType);

          if (count < 0) {
            m_valid = false;
            return;
          }

          numBytes += uint32_t(count) * kPLYPropertySize[uint32_t(prop.type)];
          if (m_pos + numBytes > m_bufEnd) {
            if (!refill_buffer() || m_pos + numBytes > m_bufEnd) {
              m_valid = false;
              return;
            }
          }
          m_pos += numBytes;
          m_end = m_pos;
        }
      }
    }
    else { // PLYFileType::BinaryBigEndian
      for (uint32_t row = 0; row < elem.count; row++) {
        for (const PLYProperty& prop : elem.properties) {
          if (prop.countType == PLYPropertyType::None) {
            uint32_t numBytes = kPLYPropertySize[uint32_t(prop.type)];
            if (m_pos + numBytes > m_bufEnd) {
              if (!refill_buffer() || m_pos + numBytes > m_bufEnd) {
                m_valid = false;
                return;
              }
            }
            m_pos += numBytes;
            m_end = m_pos;
            continue;
          }

          uint32_t numBytes = kPLYPropertySize[uint32_t(prop.countType)];
          if (m_pos + numBytes > m_bufEnd) {
            if (!refill_buffer() || m_pos + numBytes > m_bufEnd) {
              m_valid = false;
              return;
            }
          }

          int count = 0;
          uint8_t tmp[8];
          memcpy(tmp, m_pos, numBytes);
          endian_swap(tmp, prop.countType);
          copy_and_convert_to(&count, tmp, prop.countType);

          if (count < 0) {
            m_valid = false;
            return;
          }

          numBytes += uint32_t(count) * kPLYPropertySize[uint32_t(prop.type)];
          if (m_pos + numBytes > m_bufEnd) {
            if (!refill_buffer() || m_pos + numBytes > m_bufEnd) {
              m_valid = false;
              return;
            }
          }

          m_pos += numBytes;
          m_end = m_pos;
        }
      }
    }
  }


  PLYFileType PLYReader::file_type() const
  {
    return m_fileType;
  }


  int PLYReader::version_major() const
  {
    return m_majorVersion;
  }


  int PLYReader::version_minor() const
  {
    return m_minorVersion;
  }


  uint32_t PLYReader::num_elements() const
  {
    return m_valid ? static_cast<uint32_t>(m_elements.size()) : 0;
  }


  uint32_t PLYReader::find_element(const char* name) const
  {
    for (uint32_t i = 0, endI = num_elements(); i < endI; i++) {
      const PLYElement& elem = m_elements[i];
      if (strcmp(elem.name.c_str(), name) == 0) {
        return i;
      }
    }
    return kInvalidIndex;
  }


  PLYElement* PLYReader::get_element(uint32_t idx)
  {
    return (idx < num_elements()) ? &m_elements[idx] : nullptr;
  }


  bool PLYReader::element_is(const char* name) const
  {
    return has_element() && strcmp(element()->name.c_str(), name) == 0;
  }


  uint32_t PLYReader::num_rows() const
  {
    return has_element() ? element()->count : 0;
  }


  uint32_t PLYReader::find_property(const char* name) const
  {
    return has_element() ? element()->find_property(name) : kInvalidIndex;
  }


  bool PLYReader::find_properties(uint32_t propIdxs[], uint32_t numIdxs, ...) const
  {
    if (!has_element()) {
      return false;
    }
    va_list args;
    va_start(args, numIdxs);
    bool foundAll = element()->find_properties_va(propIdxs, numIdxs, args);
    va_end(args);
    return foundAll;
  }


  bool PLYReader::extract_properties(const uint32_t propIdxs[], uint32_t numProps, PLYPropertyType destType, void *dest) const
  {
    if (numProps == 0) {
      return false;
    }

    const PLYElement* elem = element();

    // Make sure all property indexes are valid and that none of the properties
    // are lists (this function only extracts non-list data).
    for (uint32_t i = 0; i < numProps; i++) {
      if (propIdxs[i] >= elem->properties.size()) {
        return false;
      }
    }

    // Find out whether we have contiguous columns. If so, we may be able to
    // use a more efficient data extraction technique.
    bool contiguousCols = true;
    uint32_t expectedOffset = elem->properties[propIdxs[0]].offset;
    for (uint32_t i = 0; i < numProps; i++) {
      uint32_t propIdx = propIdxs[i];
      const PLYProperty& prop = elem->properties[propIdx];
      if (prop.offset != expectedOffset) {
        contiguousCols = false;
        break;
      }
      expectedOffset = prop.offset + kPLYPropertySize[uint32_t(prop.type)];
    }

    // If the row we're extracting is contiguous in memory (i.e. there are no
    // gaps anywhere in a row - start, end or middle), we can use an even MORE
    // efficient data extraction technique.
    bool contiguousRows = contiguousCols &&
                          (elem->properties[propIdxs[0]].offset == 0) &&
                          (expectedOffset == elem->rowStride);

    // If no data conversion is required, we can memcpy chunks of data
    // directly over to `dest`. How big those chunks will be depends on whether
    // the columns and/or rows are contiguous, as determined above.
    bool conversionRequired = false;
    for (uint32_t i = 0; i < numProps; i++) {
      uint32_t propIdx = propIdxs[i];
      const PLYProperty& prop = elem->properties[propIdx];
      if (!compatible_types(prop.type, destType)) {
        conversionRequired = true;
        break;
      }
    }

    uint8_t* to = reinterpret_cast<uint8_t*>(dest);
    if (!conversionRequired) {
      // If no data conversion is required, we can just use memcpy to get
      // values into dest.
      if (contiguousRows) {
        // Most efficient case is when the rows are contiguous. It means we're
        // simply copying the entire data block for this element, which we can
        // do with a single memcpy.
        std::memcpy(to, m_elementData.data(), m_elementData.size());
      }
      else if (contiguousCols) {
        // If the rows aren't contiguous, but the columns we're extracting
        // within each row are, then we can do a single memcpy per row.
        const uint8_t* from = m_elementData.data() + elem->properties[propIdxs[0]].offset;
        const uint8_t* end = m_elementData.data() + m_elementData.size();
        const size_t numBytes = expectedOffset - elem->properties[propIdxs[0]].offset;
        while (from < end) {
          std::memcpy(to, from, numBytes);
          from += elem->rowStride;
          to += numBytes;
        }
      }
      else {
        // If the columns aren't contiguous, we must memcpy each one separately.
        const uint8_t* row = m_elementData.data();
        const uint8_t* end = m_elementData.data() + m_elementData.size();
        uint8_t* to = reinterpret_cast<uint8_t*>(dest);
        size_t colBytes = kPLYPropertySize[uint32_t(destType)]; // size of an output column in bytes.
        while (row < end) {
          for (uint32_t i = 0; i < numProps; i++) {
            uint32_t propIdx = propIdxs[i];
            const PLYProperty& prop = elem->properties[propIdx];
            std::memcpy(to, row + prop.offset, colBytes);
            to += colBytes;
          }
          row += elem->rowStride;
        }
      }
    }
    else {
      // We will have to do data type conversions on the column values here. We
      // cannot simply use memcpy in this case, every column has to be
      // processed separately.
      const uint8_t* row = m_elementData.data();
      const uint8_t* end = m_elementData.data() + m_elementData.size();
      uint8_t* to = reinterpret_cast<uint8_t*>(dest);
      size_t colBytes = kPLYPropertySize[uint32_t(destType)]; // size of an output column in bytes.
      while (row < end) {
        for (uint32_t i = 0; i < numProps; i++) {
          uint32_t propIdx = propIdxs[i];
          const PLYProperty& prop = elem->properties[propIdx];
          copy_and_convert(to, destType, row + prop.offset, prop.type);
          to += colBytes;
        }
        row += elem->rowStride;
      }
    }

    return true;
  }


  bool PLYReader::extract_properties_with_stride(const uint32_t propIdxs[], uint32_t numProps, PLYPropertyType destType, void *dest, uint32_t destStride) const
  {
    if (numProps == 0) {
      return false;
    }

    // The destination stride must be greater than or equal to the combined
    // size of all properties we're extracting. Zero is treated as a special
    // value meaning packed with no spacing.
    const uint32_t minDestStride = numProps * kPLYPropertySize[uint32_t(destType)];
    if (destStride == 0 || destStride == minDestStride) {
      return extract_properties(propIdxs, numProps, destType, dest);
    }
    else if (destStride < minDestStride) {
      return false;
    }

    const PLYElement* elem = element();

    // Make sure all property indexes are valid and that none of the properties
    // are lists (this function only extracts non-list data).
    for (uint32_t i = 0; i < numProps; i++) {
      if (propIdxs[i] >= elem->properties.size()) {
        return false;
      }
    }

    // Find out whether we have contiguous columns. If so, we may be able to
    // use a more efficient data extraction technique.
    bool contiguousCols = true;
    uint32_t expectedOffset = elem->properties[propIdxs[0]].offset;
    for (uint32_t i = 0; i < numProps; i++) {
      uint32_t propIdx = propIdxs[i];
      const PLYProperty& prop = elem->properties[propIdx];
      if (prop.offset != expectedOffset) {
        contiguousCols = false;
        break;
      }
      expectedOffset = prop.offset + kPLYPropertySize[uint32_t(prop.type)];
    }

    // If no data conversion is required, we can memcpy chunks of data
    // directly over to `dest`. How big those chunks will be depends on whether
    // the columns and/or rows are contiguous, as determined above.
    bool conversionRequired = false;
    for (uint32_t i = 0; i < numProps; i++) {
      uint32_t propIdx = propIdxs[i];
      const PLYProperty& prop = elem->properties[propIdx];
      if (!compatible_types(prop.type, destType)) {
        conversionRequired = true;
        break;
      }
    }

    uint8_t* to = reinterpret_cast<uint8_t*>(dest);
    if (!conversionRequired) {
      // If no data conversion is required, we can just use memcpy to get
      // values into dest. When the destination requires some padding between
      // rows, the best we can do is a memcpy per row.
      if (contiguousCols) {
        // If the rows aren't contiguous, but the columns we're extracting
        // within each row are, then we can do a single memcpy per row.
        const uint8_t* from = m_elementData.data() + elem->properties[propIdxs[0]].offset;
        const uint8_t* end = m_elementData.data() + m_elementData.size();
        const size_t numBytes = expectedOffset - elem->properties[propIdxs[0]].offset;
        while (from < end) {
          std::memcpy(to, from, numBytes);
          from += elem->rowStride;
          to += destStride;
        }
      }
      else {
        // If the columns aren't contiguous, we must memcpy each one separately.
        const uint8_t* row = m_elementData.data();
        const uint8_t* end = m_elementData.data() + m_elementData.size();
        uint8_t* to = reinterpret_cast<uint8_t*>(dest);
        const size_t colBytes = kPLYPropertySize[uint32_t(destType)]; // size of an output column in bytes.
        const size_t colPadding = destStride - minDestStride;
        while (row < end) {
          for (uint32_t i = 0; i < numProps; i++) {
            uint32_t propIdx = propIdxs[i];
            const PLYProperty& prop = elem->properties[propIdx];
            std::memcpy(to, row + prop.offset, colBytes);
            to += colBytes;
          }
          row += elem->rowStride;
          to += colPadding;
        }
      }
    }
    else {
      // We will have to do data type conversions on the column values here. We
      // cannot simply use memcpy in this case, every column has to be
      // processed separately.
      const uint8_t* row = m_elementData.data();
      const uint8_t* end = m_elementData.data() + m_elementData.size();
      uint8_t* to = reinterpret_cast<uint8_t*>(dest);
      size_t colBytes = kPLYPropertySize[uint32_t(destType)]; // size of an output column in bytes.
      size_t colPadding = destStride - minDestStride;
      while (row < end) {
        for (uint32_t i = 0; i < numProps; i++) {
          uint32_t propIdx = propIdxs[i];
          const PLYProperty& prop = elem->properties[propIdx];
          copy_and_convert(to, destType, row + prop.offset, prop.type);
          to += colBytes;
        }
        row += elem->rowStride;
        to += colPadding;
      }
    }

    return true;
  }


  const uint32_t* PLYReader::get_list_counts(uint32_t propIdx) const
  {
    if (!has_element() || propIdx >= element()->properties.size() || element()->properties[propIdx].countType == PLYPropertyType::None) {
      return nullptr;
    }
    return element()->properties[propIdx].rowCount.data();
  }


  uint32_t PLYReader::sum_of_list_counts(uint32_t propIdx) const
  {
    if (!has_element() || propIdx >= element()->properties.size() || element()->properties[propIdx].countType == PLYPropertyType::None) {
      return 0;
    }
    const PLYProperty& prop = element()->properties[propIdx];
    return (uint32_t)prop.listData.size() / kPLYPropertySize[uint32_t(prop.type)];
  }


  const uint8_t* PLYReader::get_list_data(uint32_t propIdx) const
  {
    if (!has_element() || propIdx >= element()->properties.size() || element()->properties[propIdx].countType == PLYPropertyType::None) {
      return nullptr;
    }
    return element()->properties[propIdx].listData.data();
  }


  bool PLYReader::extract_list_property(uint32_t propIdx, PLYPropertyType destType, void *dest) const
  {
    if (!has_element() || propIdx >= element()->properties.size() || element()->properties[propIdx].countType == PLYPropertyType::None) {
      return false;
    }

    const PLYProperty& prop = element()->properties[propIdx];
    if (compatible_types(prop.type, destType)) {
      // If no type conversion is required, we can just copy the list data
      // directly over with a single memcpy.
      std::memcpy(dest, prop.listData.data(), prop.listData.size());
    }
    else {
      // If type conversion is required we'll have to process each list value separately.
      const uint8_t* from = prop.listData.data();
      const uint8_t* end = prop.listData.data() + prop.listData.size();
      uint8_t* to = reinterpret_cast<uint8_t*>(dest);
      const size_t toBytes = kPLYPropertySize[uint32_t(destType)];
      const size_t fromBytes = kPLYPropertySize[uint32_t(prop.type)];
      while (from < end) {
        copy_and_convert(to, destType, from, prop.type);
        to += toBytes;
        from += fromBytes;
      }
    }

    return true;
  }


  uint32_t PLYReader::num_triangles(uint32_t propIdx) const
  {
    const uint32_t* counts = get_list_counts(propIdx);
    if (counts == nullptr) {
      return 0;
    }

    const uint32_t numRows = element()->count;
    uint32_t num = 0;
    for (uint32_t i = 0; i < numRows; i++) {
      if (counts[i] >= 3) {
        num += counts[i] - 2;
      }
    }
    return num;
  }


  bool PLYReader::requires_triangulation(uint32_t propIdx) const
  {
    const uint32_t* counts = get_list_counts(propIdx);
    if (counts == nullptr) {
      return false;
    }

    const uint32_t numRows = element()->count;
    for (uint32_t i = 0; i < numRows; i++) {
      if (counts[i] != 3) {
        return true;
      }
    }
    return false;
  }


  bool PLYReader::extract_triangles(uint32_t propIdx, const float pos[], uint32_t numVerts, PLYPropertyType destType, void *dest) const
  {
    if (!requires_triangulation(propIdx)) {
      return extract_list_property(propIdx, destType, dest);
    }

    const PLYElement* elem = element();
    const PLYProperty& prop = elem->properties[propIdx];

    const uint32_t* counts = prop.rowCount.data();
    const uint8_t*  data   = prop.listData.data();

    uint8_t* to = reinterpret_cast<uint8_t*>(dest);

    bool convertSrc = !compatible_types(elem->properties[propIdx].type, PLYPropertyType::Int);
    bool convertDst = !compatible_types(PLYPropertyType::Int, destType);

    size_t srcValBytes  = kPLYPropertySize[uint32_t(prop.type)];
    size_t destValBytes = kPLYPropertySize[uint32_t(destType)];

    if (convertSrc && convertDst) {
      std::vector<int> faceIndices, triIndices;
      faceIndices.reserve(32);
      triIndices.reserve(64);
      const uint8_t* face = data;
      for (uint32_t faceIdx = 0; faceIdx < elem->count; faceIdx++) {
        const uint8_t* faceEnd = face + srcValBytes * counts[faceIdx];
        faceIndices.clear();
        faceIndices.reserve(counts[faceIdx]);
        for (; face < faceEnd; face += srcValBytes) {
          int idx = -1;
          copy_and_convert_to(&idx, face, prop.type);
          faceIndices.push_back(idx);
        }

        triIndices.resize((counts[faceIdx] - 2) * 3);
        triangulate_polygon(counts[faceIdx], pos, numVerts, faceIndices.data(), triIndices.data());
        for (int idx : triIndices) {
          copy_and_convert(to, destType, reinterpret_cast<const uint8_t*>(&idx), PLYPropertyType::Int);
          to += destValBytes;
        }
      }
    }
    else if (convertSrc) {
      std::vector<int> faceIndices;
      faceIndices.reserve(32);
      const uint8_t* face = data;
      for (uint32_t faceIdx = 0; faceIdx < elem->count; faceIdx++) {
        const uint8_t* faceEnd = face + srcValBytes * counts[faceIdx];
        faceIndices.clear();
        faceIndices.reserve(counts[faceIdx]);
        for (; face < faceEnd; face += srcValBytes) {
          int idx = -1;
          copy_and_convert_to(&idx, face, prop.type);
          faceIndices.push_back(idx);
        }

        uint32_t numTris = triangulate_polygon(counts[faceIdx], pos, numVerts, faceIndices.data(), reinterpret_cast<int*>(to));
        to += numTris * 3 * destValBytes;
      }
    }
    else if (convertDst) {
      std::vector<int> triIndices;
      triIndices.reserve(64);
      const uint8_t* face = data;
      for (uint32_t faceIdx = 0; faceIdx < elem->count; faceIdx++) {
        triIndices.resize((counts[faceIdx] - 2) * 3);
        triangulate_polygon(counts[faceIdx], pos, numVerts, reinterpret_cast<const int*>(face), triIndices.data());
        for (int idx : triIndices) {
          copy_and_convert(to, destType, reinterpret_cast<const uint8_t*>(&idx), PLYPropertyType::Int);
          to += destValBytes;
        }
        face += srcValBytes * counts[faceIdx];
      }
    }
    else {
      const uint8_t* face = data;
      for (uint32_t faceIdx = 0; faceIdx < elem->count; faceIdx++) {
        uint32_t numTris = triangulate_polygon(counts[faceIdx], pos, numVerts, reinterpret_cast<const int*>(face), reinterpret_cast<int*>(to));
        face += counts[faceIdx] * srcValBytes;
        to += numTris * 3 * destValBytes;
      }
    }

    return true;
  }


  bool PLYReader::find_pos(uint32_t propIdxs[3]) const
  {
    return find_properties(propIdxs, 3, "x", "y", "z");
  }


  bool PLYReader::find_normal(uint32_t propIdxs[3]) const
  {
    return find_properties(propIdxs, 3, "nx", "ny", "nz");
  }


  bool PLYReader::find_texcoord(uint32_t propIdxs[2]) const
  {
    return find_properties(propIdxs, 2, "u", "v") ||
           find_properties(propIdxs, 2, "s", "t") ||
           find_properties(propIdxs, 2, "texture_u", "texture_v") ||
           find_properties(propIdxs, 2, "texture_s", "texture_t");
  }


  bool PLYReader::find_color(uint32_t propIdxs[3]) const
  {
    return find_properties(propIdxs, 3, "r", "g", "b") ||
           find_properties(propIdxs, 3, "red", "green", "blue"); 
  }


  bool PLYReader::find_indices(uint32_t propIdxs[1]) const
  {
    return find_properties(propIdxs, 1, "vertex_indices") || 
           find_properties(propIdxs, 1, "vertex_index");
  }


  //
  // PLYReader private methods
  //

  bool PLYReader::refill_buffer()
  {
    if (m_f == nullptr || m_atEOF) {
      // Nothing left to read.
      return false;
    }

    if (m_pos == m_buf && m_end == m_bufEnd) {
      // Can't make any more room in the buffer!
      return false;
    }

    // Move everything from the start of the current token onwards, to the
    // start of the read buffer.
    int64_t bufSize = static_cast<int64_t>(m_bufEnd - m_buf);
    if (bufSize < kPLYReadBufferSize) {
      m_buf[bufSize] = m_buf[kPLYReadBufferSize];
      m_buf[kPLYReadBufferSize] = '\0';
      m_bufEnd = m_buf + kPLYReadBufferSize;
    }
    size_t keep = static_cast<size_t>(m_bufEnd - m_pos);
    if (keep > 0 && m_pos > m_buf) {
      std::memmove(m_buf, m_pos, sizeof(char) * keep);
      m_bufOffset += static_cast<int64_t>(m_pos - m_buf);
    }
    m_end = m_buf + (m_end - m_pos);
    m_pos = m_buf;

    // Fill the remaining space in the buffer with data from the file.
    size_t fetched = fread(m_buf + keep, sizeof(char), kPLYReadBufferSize - keep, m_f) + keep;
    m_atEOF = fetched < kPLYReadBufferSize;
    m_bufEnd = m_buf + fetched;

    if (!m_inDataSection || m_fileType == PLYFileType::ASCII) {
      return rewind_to_safe_char();
    }
    return true;
  }


  bool PLYReader::rewind_to_safe_char()
  {
    // If it looks like a token might run past the end of this buffer, move
    // the buffer end pointer back before it & rewind the file. This way the
    // next refill will pick up the whole of the token.
    if (!m_atEOF && (m_bufEnd[-1] == '\n' || !is_safe_buffer_end(m_bufEnd[-1]))) {
      const char* safe = m_bufEnd - 2;
      // If '\n' is the last char in the buffer, then a call to `next_line()`
      // will move `m_pos` to point at the null terminator but won't refresh
      // the buffer. It would be clearer to fix this in `next_line()` but I
      // believe it'll be more performant to simply treat `\n` as an unsafe
      // character here.
      while (safe >= m_end && (*safe == '\n' || !is_safe_buffer_end(*safe))) {
        --safe;
      }
      if (safe < m_end) {
        // No safe places to rewind to in the whole buffer!
        return false;
      }
      ++safe;
      m_buf[kPLYReadBufferSize] = *safe;
      m_bufEnd = safe;
    }
    m_buf[m_bufEnd - m_buf] = '\0';

    return true;
  }


  bool PLYReader::accept()
  {
    m_pos = m_end;
    return true;
  }


  // Advances to end of line or to next non-whitespace char.
  bool PLYReader::advance()
  {
    m_pos = m_end;
    while (true) {
      while (is_whitespace(*m_pos)) {
        ++m_pos;
      }
      if (m_pos == m_bufEnd) {
        m_end = m_pos;
        if (refill_buffer()) {
          continue;
        }
        return false;
      }
      break;
    }
    m_end = m_pos;
    return true;
  }


  bool PLYReader::next_line()
  {
    m_pos = m_end;
    do {
      while (*m_pos != '\n') {
        if (m_pos == m_bufEnd) {
          m_end = m_pos;
          if (refill_buffer()) {
            continue;
          }
          return false;
        }
        ++m_pos;
      }
      ++m_pos; // move past the newline char
      m_end = m_pos;
    } while (match("comment") || match("obj_info"));

    return true;
  }


  bool PLYReader::match(const char* str)
  {
    m_end = m_pos;
    while (m_end < m_bufEnd && *str != '\0' && *m_end == *str) {
      ++m_end;
      ++str;
    }
    if (*str != '\0') {
      return false;
    }
    return true;
  }


  bool PLYReader::which(const char* values[], uint32_t* index)
  {
    for (uint32_t i = 0; values[i] != nullptr; i++) {
      if (keyword(values[i])) {
        *index = i;
        return true;
      }
    }
    return false;
  }


  bool PLYReader::which_property_type(PLYPropertyType* type)
  {
    for (uint32_t i = 0; kTypeAliases[i].name != nullptr; i++) {
      if (keyword(kTypeAliases[i].name)) {
        *type = kTypeAliases[i].type;
        return true;
      }
    }
    return false;
  }


  bool PLYReader::keyword(const char* kw)
  {
    return match(kw) && !is_keyword_part(*m_end);
  }


  bool PLYReader::identifier(char* dest, size_t destLen)
  {
    m_end = m_pos;
    if (!is_keyword_start(*m_end) || destLen == 0) {
      return false;
    }
    do {
      ++m_end;
    } while (is_keyword_part(*m_end));

    size_t len = static_cast<size_t>(m_end - m_pos);
    if (len >= destLen) {
      return false; // identifier too large for dest!
    }
    std::memcpy(dest, m_pos, sizeof(char) * len);
    dest[len] = '\0';
    return true;
  }


  bool PLYReader::int_literal(int* value)
  {
    return miniply::int_literal(m_pos, &m_end, value);
  }


  bool PLYReader::float_literal(float* value)
  {
    return miniply::float_literal(m_pos, &m_end, value);
  }


  bool PLYReader::double_literal(double* value)
  {
    return miniply::double_literal(m_pos, &m_end, value);
  }


  bool PLYReader::parse_elements()
  {
    m_elements.reserve(4);
    while (m_valid && keyword("element")) {
      parse_element();
    }
    return true;
  }


  bool PLYReader::parse_element()
  {
    int count = 0;

    m_valid = keyword("element") && advance() &&
              identifier(m_tmpBuf, kPLYTempBufferSize) && advance() &&
              int_literal(&count) && next_line();
    if (!m_valid || count < 0) {
      return false;
    }

    m_elements.push_back(PLYElement());
    PLYElement& elem = m_elements.back();
    elem.name = m_tmpBuf;
    elem.count = static_cast<uint32_t>(count);
    elem.properties.reserve(10);

    while (m_valid && keyword("property")) {
      parse_property(elem.properties);
    }

    return true;
  }


  bool PLYReader::parse_property(std::vector<PLYProperty>& properties)
  {
    PLYPropertyType type      = PLYPropertyType::None;
    PLYPropertyType countType = PLYPropertyType::None;

    m_valid = keyword("property") && advance();
    if (!m_valid) {
      return false;
    }

    if (keyword("list")) {
      // This is a list property
      m_valid = advance() && which_property_type(&countType) && advance();
      if (!m_valid) {
        return false;
      }
    }

    m_valid = which_property_type(&type) && advance() &&
              identifier(m_tmpBuf, kPLYTempBufferSize) && next_line();
    if (!m_valid) {
      return false;
    }

    properties.push_back(PLYProperty());
    PLYProperty& prop = properties.back();
    prop.name = m_tmpBuf;
    prop.type = type;
    prop.countType = countType;

    return true;
  }


  bool PLYReader::load_fixed_size_element(PLYElement& elem)
  {
    size_t numBytes = static_cast<size_t>(elem.count) * elem.rowStride;

    m_elementData.resize(numBytes);

    if (m_fileType == PLYFileType::ASCII) {
      size_t back = 0;

      for (uint32_t row = 0; row < elem.count; row++) {
        for (PLYProperty& prop : elem.properties) {
          if (!load_ascii_scalar_property(prop, back)) {
            m_valid = false;
            return false;
          }
        }
        next_line();
      }
    }
    else {
      uint8_t* dst = m_elementData.data();
      uint8_t* dstEnd = dst + numBytes;
      while (dst < dstEnd) {
        size_t bytesAvailable = static_cast<size_t>(m_bufEnd - m_pos);
        if (dst + bytesAvailable > dstEnd) {
          bytesAvailable = static_cast<size_t>(dstEnd - dst);
        }
        std::memcpy(dst, m_pos, bytesAvailable);
        m_pos += bytesAvailable;
        m_end = m_pos;
        dst += bytesAvailable;
        if (!refill_buffer()) {
          break;
        }
      }
      if (dst < dstEnd) {
        m_valid = false;
        return false;
      }

      // We assume the CPU is little endian, so if the file is big-endian we
      // need to do an endianness swap on every data item in the block.
      if (m_fileType == PLYFileType::BinaryBigEndian) {
        uint8_t* data = m_elementData.data();
        for (uint32_t row = 0; row < elem.count; row++) {
          for (PLYProperty& prop : elem.properties) {
            size_t numBytes = kPLYPropertySize[uint32_t(prop.type)];
            switch (numBytes) {
            case 2:
              endian_swap_2(data);
              break;
            case 4:
              endian_swap_4(data);
              break;
            case 8:
              endian_swap_8(data);
              break;
            default:
              break;
            }
            data += numBytes;
          }
        }
      }
    }

    m_elementLoaded = true;
    return true;
  }


  bool PLYReader::load_variable_size_element(PLYElement& elem)
  {
    m_elementData.resize(static_cast<size_t>(elem.count) * elem.rowStride);

    // Preallocate enough space for each row in the property to contain three
    // items. This is based on the assumptions that (a) the most common use for
    // list properties is vertex indices; and (b) most faces are triangles.
    // This gives a performance boost because we won't have to grow the
    // listData vector as many times during loading.
    for (PLYProperty& prop : elem.properties) {
      if (prop.countType != PLYPropertyType::None) {
        prop.listData.reserve(elem.count * kPLYPropertySize[uint32_t(prop.type)] * 3);
      }
    }

    if (m_fileType == PLYFileType::Binary) {
      size_t back = 0;
      for (uint32_t row = 0; row < elem.count; row++) {
        for (PLYProperty& prop : elem.properties) {
          if (prop.countType == PLYPropertyType::None) {
            m_valid = load_binary_scalar_property(prop, back);
          }
          else {
            load_binary_list_property(prop);
          }
        }
      }
    }
    else if (m_fileType == PLYFileType::ASCII) {
      size_t back = 0;
      for (uint32_t row = 0; row < elem.count; row++) {
        for (PLYProperty& prop : elem.properties) {
          if (prop.countType == PLYPropertyType::None) {
            m_valid = load_ascii_scalar_property(prop, back);
          }
          else {
            load_ascii_list_property(prop);
          }
        }
        next_line();
      }
    }
    else { // m_fileType == PLYFileType::BinaryBigEndian
      size_t back = 0;
      for (uint32_t row = 0; row < elem.count; row++) {
        for (PLYProperty& prop : elem.properties) {
          if (prop.countType == PLYPropertyType::None) {
            m_valid = load_binary_scalar_property_big_endian(prop, back);
          }
          else {
            load_binary_list_property_big_endian(prop);
          }
        }
      }
    }

    m_elementLoaded = true;
    return true;
  }


  bool PLYReader::load_ascii_scalar_property(PLYProperty& prop, size_t& destIndex)
  {
    uint8_t value[8];
    if (!ascii_value(prop.type, value)) {
      return false;
    }

    size_t numBytes = kPLYPropertySize[uint32_t(prop.type)];
    std::memcpy(m_elementData.data() + destIndex, value, numBytes);
    destIndex += numBytes;
    return true;
  }


  bool PLYReader::load_ascii_list_property(PLYProperty& prop)
  {
    int count = 0;
    m_valid = (prop.countType < PLYPropertyType::Float) && int_literal(&count) && advance() && (count >= 0);
    if (!m_valid) {
      return false;
    }

    const size_t numBytes = kPLYPropertySize[uint32_t(prop.type)];

    size_t back = prop.listData.size();
    prop.rowCount.push_back(static_cast<uint32_t>(count));
    prop.listData.resize(back + numBytes * size_t(count));

    for (uint32_t i = 0; i < uint32_t(count); i++) {
      if (!ascii_value(prop.type, prop.listData.data() + back)) {
        m_valid = false;
        return false;
      }
      back += numBytes;
    }

    return true;
  }


  bool PLYReader::load_binary_scalar_property(PLYProperty& prop, size_t& destIndex)
  {
    size_t numBytes = kPLYPropertySize[uint32_t(prop.type)];
    if (m_pos + numBytes > m_bufEnd) {
      if (!refill_buffer() || m_pos + numBytes > m_bufEnd) {
        m_valid = false;
        return false;
      }
    }
    std::memcpy(m_elementData.data() + destIndex, m_pos, numBytes);
    m_pos += numBytes;
    m_end = m_pos;
    destIndex += numBytes;
    return true;
  }


  bool PLYReader::load_binary_list_property(PLYProperty& prop)
  {
    size_t countBytes = kPLYPropertySize[uint32_t(prop.countType)];
    if (m_pos + countBytes > m_bufEnd) {
      if (!refill_buffer() || m_pos + countBytes > m_bufEnd) {
        m_valid = false;
        return false;
      }
    }

    int count = 0;
    copy_and_convert_to(&count, reinterpret_cast<const uint8_t*>(m_pos), prop.countType);

    if (count < 0) {
      m_valid = false;
      return false;
    }

    m_pos += countBytes;
    m_end = m_pos;

    const size_t listBytes = kPLYPropertySize[uint32_t(prop.type)] * uint32_t(count);
    if (m_pos + listBytes > m_bufEnd) {
      if (!refill_buffer() || m_pos + listBytes > m_bufEnd) {
        m_valid = false;
        return false;
      }
    }
    size_t back = prop.listData.size();
    prop.rowCount.push_back(static_cast<uint32_t>(count));
    prop.listData.resize(back + listBytes);
    std::memcpy(prop.listData.data() + back, m_pos, listBytes);

    m_pos += listBytes;
    m_end = m_pos;
    return true;
  }


  bool PLYReader::load_binary_scalar_property_big_endian(PLYProperty &prop, size_t &destIndex)
  {
    size_t startIndex = destIndex;
    if (load_binary_scalar_property(prop, destIndex)) {
      endian_swap(m_elementData.data() + startIndex, prop.type);
      return true;
    }
    else {
      return false;
    }
  }


  bool PLYReader::load_binary_list_property_big_endian(PLYProperty &prop)
  {
    size_t countBytes = kPLYPropertySize[uint32_t(prop.countType)];
    if (m_pos + countBytes > m_bufEnd) {
      if (!refill_buffer() || m_pos + countBytes > m_bufEnd) {
        m_valid = false;
        return false;
      }
    }

    int count = 0;
    uint8_t tmp[8];
    std::memcpy(tmp, m_pos, countBytes);
    endian_swap(tmp, prop.countType);
    copy_and_convert_to(&count, tmp, prop.countType);

    if (count < 0) {
      m_valid = false;
      return false;
    }

    m_pos += countBytes;
    m_end = m_pos;

    const size_t typeBytes = kPLYPropertySize[uint32_t(prop.type)];
    const size_t listBytes = typeBytes * uint32_t(count);
    if (m_pos + listBytes > m_bufEnd) {
      if (!refill_buffer() || m_pos + listBytes > m_bufEnd) {
        m_valid = false;
        return false;
      }
    }
    size_t back = prop.listData.size();
    prop.rowCount.push_back(static_cast<uint32_t>(count));
    prop.listData.resize(back + listBytes);

    uint8_t* list = prop.listData.data() + back;
    std::memcpy(list, m_pos, listBytes);
    endian_swap_array(list, prop.type, count);

    m_pos += listBytes;
    m_end = m_pos;
    return true;
  }


  bool PLYReader::ascii_value(PLYPropertyType propType, uint8_t value[8])
  {
    int tmpInt = 0;

    switch (propType) {
    case PLYPropertyType::Char:
    case PLYPropertyType::UChar:
    case PLYPropertyType::Short:
    case PLYPropertyType::UShort:
      m_valid = int_literal(&tmpInt);
      break;
    case PLYPropertyType::Int:
    case PLYPropertyType::UInt:
      m_valid = int_literal(reinterpret_cast<int*>(value));
      break;
    case PLYPropertyType::Float:
      m_valid = float_literal(reinterpret_cast<float*>(value));
      break;
    case PLYPropertyType::Double:
    default:
      m_valid = double_literal(reinterpret_cast<double*>(value));
      break;
    }

    if (!m_valid) {
      return false;
    }
    advance();

    switch (propType) {
    case PLYPropertyType::Char:
      reinterpret_cast<int8_t*>(value)[0] = static_cast<int8_t>(tmpInt);
      break;
    case PLYPropertyType::UChar:
      value[0] = static_cast<uint8_t>(tmpInt);
      break;
    case PLYPropertyType::Short:
      reinterpret_cast<int16_t*>(value)[0] = static_cast<int16_t>(tmpInt);
      break;
    case PLYPropertyType::UShort:
      reinterpret_cast<uint16_t*>(value)[0] = static_cast<uint16_t>(tmpInt);
      break;
    default:
      break;
    }
    return true;
  }


  //
  // Polygon triangulation
  //

  static float angle_at_vert(uint32_t idx,
                             const std::vector<Vec2>& points2D,
                             const std::vector<uint32_t>& prev,
                             const std::vector<uint32_t>& next)
  {
    Vec2 xaxis = normalize(points2D[next[idx]] - points2D[idx]);
    Vec2 yaxis = Vec2{-xaxis.y, xaxis.x};
    Vec2 p2p0 = points2D[prev[idx]] - points2D[idx];
    float angle = std::atan2(dot(p2p0, yaxis), dot(p2p0, xaxis));
    if (angle <= 0.0f || angle >= kPi) {
      angle = 10000.0f;
    }
    return angle;
  }


  uint32_t triangulate_polygon(uint32_t n, const float pos[], uint32_t numVerts, const int indices[], int dst[])
  {
    if (n < 3) {
      return 0;
    }
    else if (n == 3) {
      dst[0] = indices[0];
      dst[1] = indices[1];
      dst[2] = indices[2];
      return 1;
    }
    else if (n == 4) {
      dst[0] = indices[0];
      dst[1] = indices[1];
      dst[2] = indices[3];

      dst[3] = indices[2];
      dst[4] = indices[3];
      dst[5] = indices[1];
      return 2;
    }

    // Check that all indices for this face are in the valid range before we
    // try to dereference them.
    for (uint32_t i = 0; i < n; i++) {
      if (indices[i] < 0 || uint32_t(indices[i]) >= numVerts) {
        return 0;
      }
    }

    const Vec3* vpos = reinterpret_cast<const Vec3*>(pos);

    // Calculate the geometric normal of the face
    Vec3 origin = vpos[indices[0]];
    Vec3 faceU = normalize(vpos[indices[1]] - origin);
    Vec3 faceNormal = normalize(cross(faceU, normalize(vpos[indices[n - 1]] - origin)));
    Vec3 faceV = normalize(cross(faceNormal, faceU));

    // Project the faces points onto the plane perpendicular to the normal.
    std::vector<Vec2> points2D(n, Vec2{0.0f, 0.0f});
    for (uint32_t i = 1; i < n; i++) {
      Vec3 p = vpos[indices[i]] - origin;
      points2D[i] = Vec2{dot(p, faceU), dot(p, faceV)};
    }

    std::vector<uint32_t> next(n, 0u);
    std::vector<uint32_t> prev(n, 0u);
    uint32_t first = 0;
    for (uint32_t i = 0, j = n - 1; i < n; i++) {
      next[j] = i;
      prev[i] = j;
      j = i;
    }

    // Do ear clipping.
    while (n > 3) {
      // Find the (remaining) vertex with the sharpest angle.
      uint32_t bestI = first;
      float bestAngle = angle_at_vert(first, points2D, prev, next);
      for (uint32_t i = next[first]; i != first; i = next[i]) {
        float angle = angle_at_vert(i, points2D, prev, next);
        if (angle < bestAngle) {
          bestI = i;
          bestAngle = angle;
        }
      }

      // Clip the triangle at bestI.
      uint32_t nextI = next[bestI];
      uint32_t prevI = prev[bestI];

      dst[0] = indices[bestI];
      dst[1] = indices[nextI];
      dst[2] = indices[prevI];
      dst += 3;

      if (bestI == first) {
        first = nextI;
      }
      next[prevI] = nextI;
      prev[nextI] = prevI;
      --n;
    }

    // Add the final triangle.
    dst[0] = indices[first];
    dst[1] = indices[next[first]];
    dst[2] = indices[prev[first]];

    return n - 2;
  }

} // namespace miniply
