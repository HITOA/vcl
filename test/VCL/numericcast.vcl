in float32 f32;
in float64 f64;

out float64 fce;
out float32 fct;
out int64 fts;
out uint64 ftu;

int Main() {
    fce = (float64)f32;
    fct = (float32)f64;
    fts = (int64)f32;
    ftu = (uint64)f32;
}