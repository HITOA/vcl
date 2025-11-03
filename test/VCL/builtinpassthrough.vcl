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
in bool ib;

out float32 of32;
out float64 of64;
out int8 oi8;
out int16 oi16;
out int32 oi32;
out int64 oi64;
out uint8 ou8;
out uint16 ou16;
out uint32 ou32;
out uint64 ou64;
out bool ob;

void Main() {
    of32 = if32;
    of64 = if64;
    oi8 = ii8;
    oi16 = ii16;
    oi32 = ii32;
    oi64 = ii64;
    ou8 = iu8;
    ou16 = iu16;
    ou32 = iu32;
    ou64 = iu64;
    ob = ib;
}