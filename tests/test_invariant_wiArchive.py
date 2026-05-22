import pytest
import struct
import io


# Simulated archive reader that mimics wiArchive.cpp deserialization behavior
class ArchiveReader:
    """
    Simulates the WickedEngine archive deserialization logic.
    Reads a length-prefixed buffer and enforces that reads never exceed declared length.
    """

    def __init__(self, data: bytes):
        self._data = data
        self._pos = 0
        self._reads = []

    def read_uint32(self):
        if self._pos + 4 > len(self._data):
            raise ValueError(f"Buffer underflow: need 4 bytes at pos {self._pos}, have {len(self._data) - self._pos}")
        val = struct.unpack_from('<I', self._data, self._pos)[0]
        self._pos += 4
        return val

    def read_uint64(self):
        if self._pos + 8 > len(self._data):
            raise ValueError(f"Buffer underflow: need 8 bytes at pos {self._pos}, have {len(self._data) - self._pos}")
        val = struct.unpack_from('<Q', self._data, self._pos)[0]
        self._pos += 8
        return val

    def read_bytes(self, length: int) -> bytes:
        """
        Security invariant: reads must never exceed declared length.
        If declared length exceeds available data, raise instead of over-reading.
        """
        if length < 0:
            raise ValueError(f"Negative length declared: {length}")
        if length > 1024 * 1024 * 64:  # 64MB sanity cap
            raise ValueError(f"Declared length {length} exceeds sanity cap")
        available = len(self._data) - self._pos
        if length > available:
            raise ValueError(
                f"Declared length {length} exceeds available bytes {available} at pos {self._pos}"
            )
        chunk = self._data[self._pos:self._pos + length]
        assert len(chunk) == length, (
            f"Read {len(chunk)} bytes but declared {length} — buffer over-read detected!"
        )
        self._pos += length
        self._reads.append((length, len(chunk)))
        return chunk

    def deserialize_string(self) -> str:
        """Deserialize a length-prefixed string."""
        length = self.read_uint32()
        raw = self.read_bytes(length)
        return raw.decode('utf-8', errors='replace')

    def deserialize_blob(self) -> bytes:
        """Deserialize a length-prefixed binary blob."""
        length = self.read_uint64()
        return self.read_bytes(length)

    def deserialize_scene_component(self) -> dict:
        """Simulate scene component deserialization."""
        name = self.deserialize_string()
        data = self.deserialize_blob()
        return {"name": name, "data": data}


def make_length_prefixed_string_u32(s: bytes) -> bytes:
    return struct.pack('<I', len(s)) + s

def make_length_prefixed_blob_u64(b: bytes) -> bytes:
    return struct.pack('<Q', len(b)) + b

def make_scene_component(name: bytes, data: bytes) -> bytes:
    return make_length_prefixed_string_u32(name) + make_length_prefixed_blob_u64(data)

def make_oversized_length_u32(declared_length: int, actual_data: bytes) -> bytes:
    """Declare a larger length than actual data — simulates attacker-controlled length field."""
    return struct.pack('<I', declared_length) + actual_data

def make_oversized_length_u64(declared_length: int, actual_data: bytes) -> bytes:
    return struct.pack('<Q', declared_length) + actual_data


# Attack payloads: (description, raw_bytes, expected_to_raise)
PAYLOADS = [
    # --- Oversized string: declared 2x actual size ---
    (
        "string_2x_oversize",
        make_oversized_length_u32(20, b"A" * 10),
        True,
    ),
    # --- Oversized string: declared 10x actual size ---
    (
        "string_10x_oversize",
        make_oversized_length_u32(100, b"B" * 10),
        True,
    ),
    # --- Oversized blob: declared 2x actual size ---
    (
        "blob_2x_oversize",
        make_length_prefixed_string_u32(b"valid_name") + make_oversized_length_u64(200, b"C" * 100),
        True,
    ),
    # --- Oversized blob: declared 10x actual size ---
    (
        "blob_10x_oversize",
        make_length_prefixed_string_u32(b"valid_name") + make_oversized_length_u64(1000, b"D" * 100),
        True,
    ),
    # --- Max uint32 declared length (classic integer overflow attack) ---
    (
        "string_max_uint32",
        make_oversized_length_u32(0xFFFFFFFF, b"E" * 16),
        True,
    ),
    # --- Max uint64 declared length ---
    (
        "blob_max_uint64",
        make_length_prefixed_string_u32(b"x") + make_oversized_length_u64(0xFFFFFFFFFFFFFFFF, b"F" * 8),
        True,
    ),
    # --- Empty data with nonzero declared length ---
    (
        "string_nonzero_length_empty_data",
        struct.pack('<I', 42),  # declares 42 bytes, provides 0
        True,
    ),
    # --- Negative-like length (large uint32 that wraps) ---
    (
        "string_wrap_length",
        make_oversized_length_u32(0x80000001, b"G" * 4),
        True,
    ),
    # --- Valid small payload (should NOT raise) ---
    (
        "valid_small_string",
        make_length_prefixed_string_u32(b"hello"),
        False,
    ),
    # --- Valid scene component (should NOT raise) ---
    (
        "valid_scene_component",
        make_scene_component(b"mesh_component", b"\x00\x01\x02\x03" * 8),
        False,
    ),
    # --- Truncated header: only 2 bytes for a uint32 length ---
    (
        "truncated_length_header",
        b"\x10\x00",  # incomplete uint32
        True,
    ),
    # --- Zero-length string (edge case, valid) ---
    (
        "zero_length_string",
        make_length_prefixed_string_u32(b""),
        False,
    ),
    # --- Oversized by exactly 1 byte ---
    (
        "string_oversize_by_one",
        make_oversized_length_u32(11, b"H" * 10),
        True,
    ),
    # --- Payload with embedded null bytes and oversized declaration ---
    (
        "null_bytes_oversized",
        make_oversized_length_u32(50, b"\x00" * 10),
        True,
    ),
    # --- Scene component with oversized name ---
    (
        "scene_component_oversized_name",
        make_oversized_length_u32(9999, b"I" * 5) + make_length_prefixed_blob_u64(b"data"),
        True,
    ),
    # --- Deeply nested: valid name, oversized blob by 10x ---
    (
        "scene_component_oversized_blob_10x",
        make_length_prefixed_string_u32(b"component") + make_oversized_length_u64(10000, b"J" * 1000),
        True,
    ),
    # --- All zeros (length=0 for string, then tries to read blob with no data) ---
    (
        "all_zeros_truncated",
        b"\x00\x00\x00\x00",  # string length=0, then no blob data
        True,  # blob read will fail
    ),
    # --- Sanity cap exceeded ---
    (
        "sanity_cap_exceeded",
        make_length_prefixed_string_u32(b"x") + struct.pack('<Q', 64 * 1024 * 1024 + 1) + b"K" * 100,
        True,
    ),
]


@pytest.mark.parametrize("description,payload,should_raise", PAYLOADS)
def test_buffer_reads_never_exceed_declared_length(description, payload, should_raise):
    """
    Invariant: Buffer reads must NEVER exceed the declared length field.
    Any attempt to read more bytes than declared (or more than available)
    must be rejected — either truncated or raise an exception.
    No silent over-read of memory is permitted.
    """
    reader = ArchiveReader(payload)

    if should_raise:
        with pytest.raises((ValueError, struct.error), match=r".+"):
            # Attempt full deserialization — must raise before any over-read
            try:
                reader.deserialize_scene_component()
            except (ValueError, struct.error):
                raise
            except Exception as e:
                pytest.fail(f"[{description}] Unexpected exception type {type(e)}: {e}")

        # Verify no read ever returned more bytes than declared
        for declared, actual in reader._reads:
            assert actual <= declared, (
                f"[{description}] SECURITY VIOLATION: read {actual} bytes "
                f"but declared length was {declared} — buffer over-read!"
            )
    else:
        # Valid payload: must succeed without error
        try:
            result = reader.deserialize_scene_component()
        except Exception as e:
            pytest.fail(f"[{description}] Valid payload raised unexpected exception: {e}")

        # Verify all reads respected declared lengths
        for declared, actual in reader._reads:
            assert actual == declared, (
                f"[{description}] Read {actual} bytes but declared {declared} — mismatch!"
            )
        assert result is not None, f"[{description}] Expected non-None result for valid payload"


@pytest.mark.parametrize("multiplier", [2, 5, 10, 100, 1000])
def test_oversized_string_by_multiplier(multiplier):
    """
    Invariant: Strings with declared length N*actual_size must always be rejected,
    never silently over-read.
    """
    actual_data = b"X" * 32
    declared_length = len(actual_data) * multiplier
    payload = make_oversized_length_u32(declared_length, actual_data)

    reader = ArchiveReader(payload)
    with pytest.raises(ValueError):
        reader.deserialize_string()

    for declared, actual in reader._reads:
        assert actual <= declared, (
            f"SECURITY VIOLATION at {multiplier}x: read {actual} bytes "
            f"but declared {declared} — buffer over-read!"
        )


@pytest.mark.parametrize("declared,actual_size", [
    (0xFFFFFFFF, 4),
    (0xFFFFFFFE, 4),
    (0x7FFFFFFF, 8),
    (65536, 16),
    (4096, 1),
])
def test_integer_overflow_length_fields(declared, actual_size):
    """
    Invariant: Large or wrap-around length values must never cause
    the reader to access memory beyond the actual buffer boundary.
    """
    payload = struct.pack('<I', declared) + b"Y" * actual_size
    reader = ArchiveReader(payload)

    with pytest.raises(ValueError):
        reader.deserialize_string()

    # Critical: no read should have returned more bytes than declared
    for d, a in reader._reads:
        assert a <= d, (
            f"SECURITY VIOLATION: declared={declared}, actual_size={actual_size}: "
            f"read {a} bytes but declared {d}"
        )