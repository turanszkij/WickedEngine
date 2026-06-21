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

#ifndef MINIPLY_H
#define MINIPLY_H

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>


/// miniply - A simple and fast parser for PLY files
/// ================================================
///
/// For details about the PLY format see:
/// * http://paulbourke.net/dataformats/ply/
/// * https://en.wikipedia.org/wiki/PLY_(file_format)

namespace miniply {

  //
  // Constants
  //

  static constexpr uint32_t kInvalidIndex = 0xFFFFFFFFu;

  // Standard PLY element names
  extern const char* kPLYVertexElement; // "vertex"
  extern const char* kPLYFaceElement;   // "face"


  //
  // PLY Parsing types
  //

  enum class PLYFileType {
    ASCII,
    Binary,
    BinaryBigEndian,
  };


  enum class PLYPropertyType {
    Char,
    UChar,
    Short,
    UShort,
    Int,
    UInt,
    Float,
    Double,

    None, //!< Special value used in Element::listCountType to indicate a non-list property.
  };


  struct PLYProperty {
    std::string name;
    PLYPropertyType type      = PLYPropertyType::None; //!< Type of the data. Must be set to a value other than None.
    PLYPropertyType countType = PLYPropertyType::None; //!< None indicates this is not a list type, otherwise it's the type for the list count.
    uint32_t offset           = 0;                  //!< Byte offset from the start of the row.
    uint32_t stride           = 0;

    std::vector<uint8_t> listData;
    std::vector<uint32_t> rowCount; // Entry `i` is the number of items (*not* the number of bytes) in row `i`.
  };


  struct PLYElement {
    std::string              name;              //!< Name of this element.
    std::vector<PLYProperty> properties;
    uint32_t                 count      = 0;    //!< The number of items in this element (e.g. the number of vertices if this is the vertex element).
    bool                     fixedSize  = true; //!< `true` if there are only fixed-size properties in this element, i.e. no list properties.
    uint32_t                 rowStride  = 0;    //!< The number of bytes from the start of one row to the start of the next, for this element.

    void calculate_offsets();

    /// Returns the index for the named property in this element, or `kInvalidIndex`
    /// if it can't be found.
    uint32_t find_property(const char* propName) const;

    /// Return the indices for several properties in one go. Use it like this:
    /// ```
    /// uint32_t indexes[3];
    /// if (elem.find_properties(indexes, 3, "foo", "bar", "baz")) { ... }
    /// ```
    /// `propIdxs` is where the property indexes will be stored. `numIdxs` is
    /// the number of properties we will look up. There must be exactly
    /// `numIdxs` parameters after `numIdxs`; each of the is a c-style string
    /// giving the name of a property.
    ///
    /// The return value will be true if all properties were found. If it was
    /// not true, you should not use any values from propIdxs.
    bool find_properties(uint32_t propIdxs[], uint32_t numIdxs, ...) const;

    /// Same as `find_properties`, for when you already have a `va_list`. This
    /// is called internally by both `PLYElement::find_properties` and
    /// `PLYReader::find_properties`.
    bool find_properties_va(uint32_t propIdxs[], uint32_t numIdxs, va_list names) const;

    /// Call this on the element at some point before you load its data, when
    /// you know that every row's list will have the same length. It will
    /// replace the single variable-size property with a set of new fixed-size
    /// properties: one for the list count, followed by one for each of the
    /// list values. This will allow miniply to load and extract the property
    /// data a lot more efficiently, giving a big performance increase.
    ///
    /// After you've called this, you must use PLYReader's `extract_columns`
    /// method to get the data, rather than `extract_list_column`.
    ///
    /// The `newPropIdxs` parameter must be an array with at least `listSize`
    /// entries. If the function returns true, this will have been populated
    /// with the indices of the new properties that represent the list values
    /// (i.e. not including the list count property, which will have the same
    /// index as the old list property).
    ///
    /// The function returns false if the property index is invalid, or the
    /// property it refers to is not a list property. In these cases it will
    /// not modify anything. Otherwise it will return true.
    bool convert_list_to_fixed_size(uint32_t listPropIdx, uint32_t listSize, uint32_t newPropIdxs[]);
  };


  class PLYReader {
  public:
    PLYReader(const char* filename);
    ~PLYReader();

    bool valid() const;
    bool has_element() const;
    const PLYElement* element() const;
    bool load_element();
    void next_element();

    PLYFileType file_type() const;
    int version_major() const;
    int version_minor() const;
    uint32_t num_elements() const;
    uint32_t find_element(const char* name) const;
    PLYElement* get_element(uint32_t idx);

    /// Check whether the current element has the given name.
    bool element_is(const char* name) const;

    /// Number of rows in the current element.
    uint32_t num_rows() const;

    /// Returns the index for the named property in the current element, or
    /// `kInvalidIndex` if it can't be found.
    uint32_t find_property(const char* name) const;

    /// Equivalent to calling `find_properties` on the current element.
    bool find_properties(uint32_t propIdxs[], uint32_t numIdxs, ...) const;

    /// Copy the data for the specified properties into `dest`, which must be
    /// an array with at least enough space to hold all of the extracted column
    /// data. `propIdxs` is an array containing the indexes of the properties
    /// to copy; it has `numProps` elements.
    ///
    /// `destType` specifies the data type for values stored in `dest`. All
    /// property values will be converted to this type if necessary.
    ///
    /// This function does some checks up front to pick the most efficient code
    /// path for extracting the data. It considers:
    /// (a) whether any data conversion is required.
    /// (b) whether all property values to be extracted are in contiguous
    ///     memory locations for any given item.
    /// (c) whether the data for all rows is contiguous in memory.
    /// In the best case it reduces to a single memcpy call. In the worst case
    /// we must iterate over all values to be copied, applying type conversions
    /// as we go.
    ///
    /// Note that this function does not handle list-valued properties. Use
    /// `extract_list_column()` for those instead.
    bool extract_properties(const uint32_t propIdxs[], uint32_t numProps, PLYPropertyType destType, void* dest) const;

    /// The same as `extract_properties`, but does not require rows in the
    /// destination to be contiguous: `destStride` is the number of bytes
    /// between the start of one row and the start of the next row in the
    /// destination memory.
    ///
    /// This is useful for when your destination is an array of structs where
    /// you cannot extract all of the properties with a single
    /// `extract_properties` call, e.g. when not all of the struct members
    /// have the same type, or when the data you're extracting is only a
    /// subset of the columns in each destination row.
    ///
    /// This is a tiny bit slower than `extract_properties`. Wherever possible
    /// you should use `extract_properties` in preference to this method.
    bool extract_properties_with_stride(const uint32_t propIdxs[], uint32_t numProps, PLYPropertyType destType, void* dest, uint32_t destStride) const;

    /// Get the array of item counts for a list property. Entry `i` in this
    /// array is the number of items in the `i`th list.
    const uint32_t* get_list_counts(uint32_t propIdx) const;

    /// Get the sum of all item counts for a list property. This can be useful
    /// to determine how big a destination array you'll need for a call to
    /// `extract_list_property`. It's equivalent to summing up all the values
    /// in the array returned by `get_list_counts`, but faster.
    uint32_t sum_of_list_counts(uint32_t propIdx) const;

    const uint8_t* get_list_data(uint32_t propIdx) const;
    bool extract_list_property(uint32_t propIdx, PLYPropertyType destType, void* dest) const;

    uint32_t num_triangles(uint32_t propIdx) const;
    bool requires_triangulation(uint32_t propIdx) const;
    bool extract_triangles(uint32_t propIdx, const float pos[], uint32_t numVerts, PLYPropertyType destType, void* dest) const;

    bool find_pos(uint32_t propIdxs[3]) const;
    bool find_normal(uint32_t propIdxs[3]) const;
    bool find_texcoord(uint32_t propIdxs[2]) const;
    bool find_color(uint32_t propIdxs[3]) const;
    bool find_indices(uint32_t propIdxs[1]) const;

  private:
    bool refill_buffer();
    bool rewind_to_safe_char();
    bool accept();
    bool advance();
    bool next_line();
    bool match(const char* str);
    bool which(const char* values[], uint32_t* index);
    bool which_property_type(PLYPropertyType* type);
    bool keyword(const char* kw);
    bool identifier(char* dest, size_t destLen);

    template <class T> // T must be a type compatible with uint32_t.
    bool typed_which(const char* values[], T* index) {
      return which(values, reinterpret_cast<uint32_t*>(index));
    }

    bool int_literal(int* value);
    bool float_literal(float* value);
    bool double_literal(double* value);

    bool parse_elements();
    bool parse_element();
    bool parse_property(std::vector<PLYProperty>& properties);

    bool load_fixed_size_element(PLYElement& elem);
    bool load_variable_size_element(PLYElement& elem);

    bool load_ascii_scalar_property(PLYProperty& prop, size_t& destIndex);
    bool load_ascii_list_property(PLYProperty& prop);
    bool load_binary_scalar_property(PLYProperty& prop, size_t& destIndex);
    bool load_binary_list_property(PLYProperty& prop);
    bool load_binary_scalar_property_big_endian(PLYProperty& prop, size_t& destIndex);
    bool load_binary_list_property_big_endian(PLYProperty& prop);

    bool ascii_value(PLYPropertyType propType, uint8_t value[8]);

  private:
    FILE* m_f             = nullptr;
    char* m_buf           = nullptr;
    const char* m_bufEnd  = nullptr;
    const char* m_pos     = nullptr;
    const char* m_end     = nullptr;
    bool m_inDataSection  = false;
    bool m_atEOF          = false;
    int64_t m_bufOffset   = 0;

    bool m_valid          = false;

    PLYFileType m_fileType = PLYFileType::ASCII; //!< Whether the file was ascii, binary little-endian, or binary big-endian.
    int m_majorVersion     = 0;
    int m_minorVersion     = 0;
    std::vector<PLYElement> m_elements;         //!< Element descriptors for this file.

    size_t m_currentElement = 0;
    bool m_elementLoaded    = false;
    std::vector<uint8_t> m_elementData;

    char* m_tmpBuf = nullptr;
  };


  /// Given a polygon with `n` vertices, where `n` > 3, triangulate it and
  /// store the indices for the resulting triangles in `dst`. The `pos`
  /// parameter is the array of all vertex positions for the mesh; `indices` is
  /// the list of `n` indices for the polygon we're triangulating; and `dst` is
  /// where we write the new indices to.
  ///
  /// The triangulation will always produce `n - 2` triangles, so `dst` must
  /// have enough space for `3 * (n - 2)` indices.
  ///
  /// If `n == 3`, we simply copy the input indices to `dst`. If `n < 3`,
  /// nothing gets written to dst.
  ///
  /// The return value is the number of triangles.
  uint32_t triangulate_polygon(uint32_t n, const float pos[], uint32_t numVerts, const int indices[], int dst[]);

} // namespace miniply

#endif // MINIPLY_H
