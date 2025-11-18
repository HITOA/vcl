// Input values for all numeric types
in float32 if32;
in float64 if64;
in int8 ii8;
in int16 ii16;
in int32 ii32;
in int64 ii64;
in uint8 iu8;
in uint16 iu16;
in uint32 iu32;
in uint64 iu64;

// Output values for floating point casts
out float64 o_fce_f32;      // FloatingCastExt: float32 -> float64
out float32 o_fct_f64;      // FloatingCastTrunc: float64 -> float32

// Output values for floating to integer casts
out int8 o_fts_f32_i8;      // FloatingToSigned: float32 -> int8
out int16 o_fts_f32_i16;    // FloatingToSigned: float32 -> int16
out int32 o_fts_f32_i32;    // FloatingToSigned: float32 -> int32
out int64 o_fts_f32_i64;    // FloatingToSigned: float32 -> int64
out int64 o_fts_f64_i64;    // FloatingToSigned: float64 -> int64

out uint8 o_ftu_f32_u8;     // FloatingToUnsigned: float32 -> uint8
out uint16 o_ftu_f32_u16;   // FloatingToUnsigned: float32 -> uint16
out uint32 o_ftu_f32_u32;   // FloatingToUnsigned: float32 -> uint32
out uint64 o_ftu_f32_u64;   // FloatingToUnsigned: float32 -> uint64
out uint64 o_ftu_f64_u64;   // FloatingToUnsigned: float64 -> uint64

// Output values for signed integer casts
out int16 o_sce_i8_i16;     // SignedCastExt: int8 -> int16
out int32 o_sce_i8_i32;     // SignedCastExt: int8 -> int32
out int64 o_sce_i8_i64;     // SignedCastExt: int8 -> int64
out int32 o_sce_i16_i32;    // SignedCastExt: int16 -> int32
out int64 o_sce_i16_i64;    // SignedCastExt: int16 -> int64
out int64 o_sce_i32_i64;    // SignedCastExt: int32 -> int64

out int8 o_sct_i16_i8;      // SignedCastTrunc: int16 -> int8
out int8 o_sct_i32_i8;      // SignedCastTrunc: int32 -> int8
out int8 o_sct_i64_i8;      // SignedCastTrunc: int64 -> int8
out int16 o_sct_i32_i16;    // SignedCastTrunc: int32 -> int16
out int16 o_sct_i64_i16;    // SignedCastTrunc: int64 -> int16
out int32 o_sct_i64_i32;    // SignedCastTrunc: int64 -> int32

// Output values for signed to floating casts
out float32 o_stf_i8_f32;   // SignedToFloating: int8 -> float32
out float32 o_stf_i16_f32;  // SignedToFloating: int16 -> float32
out float32 o_stf_i32_f32;  // SignedToFloating: int32 -> float32
out float64 o_stf_i64_f64;  // SignedToFloating: int64 -> float64

// Output values for signed to unsigned casts
out uint8 o_stu_i8_u8;      // SignedToUnsigned: int8 -> uint8
out uint16 o_stu_i16_u16;   // SignedToUnsigned: int16 -> uint16
out uint32 o_stu_i32_u32;   // SignedToUnsigned: int32 -> uint32
out uint64 o_stu_i64_u64;   // SignedToUnsigned: int64 -> uint64

// Output values for unsigned integer casts
out uint16 o_uce_u8_u16;    // UnsignedCastExt: uint8 -> uint16
out uint32 o_uce_u8_u32;    // UnsignedCastExt: uint8 -> uint32
out uint64 o_uce_u8_u64;    // UnsignedCastExt: uint8 -> uint64
out uint32 o_uce_u16_u32;   // UnsignedCastExt: uint16 -> uint32
out uint64 o_uce_u16_u64;   // UnsignedCastExt: uint16 -> uint64
out uint64 o_uce_u32_u64;   // UnsignedCastExt: uint32 -> uint64

out uint8 o_uct_u16_u8;     // UnsignedCastTrunc: uint16 -> uint8
out uint8 o_uct_u32_u8;     // UnsignedCastTrunc: uint32 -> uint8
out uint8 o_uct_u64_u8;     // UnsignedCastTrunc: uint64 -> uint8
out uint16 o_uct_u32_u16;   // UnsignedCastTrunc: uint32 -> uint16
out uint16 o_uct_u64_u16;   // UnsignedCastTrunc: uint64 -> uint16
out uint32 o_uct_u64_u32;   // UnsignedCastTrunc: uint64 -> uint32

// Output values for unsigned to floating casts
out float32 o_utf_u8_f32;   // UnsignedToFloating: uint8 -> float32
out float32 o_utf_u16_f32;  // UnsignedToFloating: uint16 -> float32
out float32 o_utf_u32_f32;  // UnsignedToFloating: uint32 -> float32
out float64 o_utf_u64_f64;  // UnsignedToFloating: uint64 -> float64

// Output values for unsigned to signed casts
out int8 o_uts_u8_i8;       // UnsignedToSigned: uint8 -> int8
out int16 o_uts_u16_i16;    // UnsignedToSigned: uint16 -> int16
out int32 o_uts_u32_i32;    // UnsignedToSigned: uint32 -> int32
out int64 o_uts_u64_i64;    // UnsignedToSigned: uint64 -> int64

void Main() {
    // Floating point extension and truncation
    o_fce_f32 = (float64)if32;     // FloatingCastExt
    o_fct_f64 = (float32)if64;     // FloatingCastTrunc
    
    // Floating to signed integer casts
    o_fts_f32_i8 = (int8)if32;     // FloatingToSigned
    o_fts_f32_i16 = (int16)if32;   // FloatingToSigned
    o_fts_f32_i32 = (int32)if32;   // FloatingToSigned
    o_fts_f32_i64 = (int64)if32;   // FloatingToSigned
    o_fts_f64_i64 = (int64)if64;   // FloatingToSigned
    
    // Floating to unsigned integer casts
    o_ftu_f32_u8 = (uint8)if32;    // FloatingToUnsigned
    o_ftu_f32_u16 = (uint16)if32;  // FloatingToUnsigned
    o_ftu_f32_u32 = (uint32)if32;  // FloatingToUnsigned
    o_ftu_f32_u64 = (uint64)if32;  // FloatingToUnsigned
    o_ftu_f64_u64 = (uint64)if64;  // FloatingToUnsigned
    
    // Signed integer extension casts
    o_sce_i8_i16 = (int16)ii8;     // SignedCastExt
    o_sce_i8_i32 = (int32)ii8;     // SignedCastExt
    o_sce_i8_i64 = (int64)ii8;     // SignedCastExt
    o_sce_i16_i32 = (int32)ii16;   // SignedCastExt
    o_sce_i16_i64 = (int64)ii16;   // SignedCastExt
    o_sce_i32_i64 = (int64)ii32;   // SignedCastExt
    
    // Signed integer truncation casts
    o_sct_i16_i8 = (int8)ii16;     // SignedCastTrunc
    o_sct_i32_i8 = (int8)ii32;     // SignedCastTrunc
    o_sct_i64_i8 = (int8)ii64;     // SignedCastTrunc
    o_sct_i32_i16 = (int16)ii32;   // SignedCastTrunc
    o_sct_i64_i16 = (int16)ii64;   // SignedCastTrunc
    o_sct_i64_i32 = (int32)ii64;   // SignedCastTrunc
    
    // Signed to floating casts
    o_stf_i8_f32 = (float32)ii8;   // SignedToFloating
    o_stf_i16_f32 = (float32)ii16; // SignedToFloating
    o_stf_i32_f32 = (float32)ii32; // SignedToFloating
    o_stf_i64_f64 = (float64)ii64; // SignedToFloating
    
    // Signed to unsigned casts
    o_stu_i8_u8 = (uint8)ii8;      // SignedToUnsigned
    o_stu_i16_u16 = (uint16)ii16;  // SignedToUnsigned
    o_stu_i32_u32 = (uint32)ii32;  // SignedToUnsigned
    o_stu_i64_u64 = (uint64)ii64;  // SignedToUnsigned
    
    // Unsigned integer extension casts
    o_uce_u8_u16 = (uint16)iu8;    // UnsignedCastExt
    o_uce_u8_u32 = (uint32)iu8;    // UnsignedCastExt
    o_uce_u8_u64 = (uint64)iu8;    // UnsignedCastExt
    o_uce_u16_u32 = (uint32)iu16;  // UnsignedCastExt
    o_uce_u16_u64 = (uint64)iu16;  // UnsignedCastExt
    o_uce_u32_u64 = (uint64)iu32;  // UnsignedCastExt
    
    // Unsigned integer truncation casts
    o_uct_u16_u8 = (uint8)iu16;    // UnsignedCastTrunc
    o_uct_u32_u8 = (uint8)iu32;    // UnsignedCastTrunc
    o_uct_u64_u8 = (uint8)iu64;    // UnsignedCastTrunc
    o_uct_u32_u16 = (uint16)iu32;  // UnsignedCastTrunc
    o_uct_u64_u16 = (uint16)iu64;  // UnsignedCastTrunc
    o_uct_u64_u32 = (uint32)iu64;  // UnsignedCastTrunc
    
    // Unsigned to floating casts
    o_utf_u8_f32 = (float32)iu8;   // UnsignedToFloating
    o_utf_u16_f32 = (float32)iu16; // UnsignedToFloating
    o_utf_u32_f32 = (float32)iu32; // UnsignedToFloating
    o_utf_u64_f64 = (float64)iu64; // UnsignedToFloating
    
    // Unsigned to signed casts
    o_uts_u8_i8 = (int8)iu8;       // UnsignedToSigned
    o_uts_u16_i16 = (int16)iu16;   // UnsignedToSigned
    o_uts_u32_i32 = (int32)iu32;   // UnsignedToSigned
    o_uts_u64_i64 = (int64)iu64;   // UnsignedToSigned
}