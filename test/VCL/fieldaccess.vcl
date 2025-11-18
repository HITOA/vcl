// Test field access expressions
in float32 input_x;
in float32 input_y;
in int32 input_count;

// Define test structures
struct Point {
    float32 x;
    float32 y;
}

struct Rectangle {
    Point topLeft;
    Point bottomRight;
    int32 id;
}

struct Complex {
    Rectangle bounds;
    float32 scale;
    int32 flags;
}

// Output values for field access tests
out float32 o_point_x;       // Direct field access
out float32 o_point_y;       // Direct field access
out float32 o_rect_x;        // Nested field access
out float32 o_rect_y;        // Nested field access
out int32 o_rect_id;         // Integer field access
out float32 o_complex_x;     // Deep nested field access
out float32 o_scale;         // Complex struct field access
out int32 o_flags;           // Complex struct field access

// Modified field values
out float32 o_modified_x;    // After field modification
out float32 o_modified_y;    // After field modification

void Main() {
    // Create and initialize structures
    Point p;
    p.x = input_x;
    p.y = input_y;
    
    Rectangle rect;
    rect.topLeft.x = input_x;
    rect.topLeft.y = input_y;
    rect.bottomRight.x = input_x + 10.0;
    rect.bottomRight.y = input_y + 10.0;
    rect.id = input_count;
    
    Complex complex;
    complex.bounds = rect;
    complex.scale = input_x * 2.0;
    complex.flags = input_count * 2;
    
    // Test direct field access
    o_point_x = p.x;
    o_point_y = p.y;
    
    // Test nested field access
    o_rect_x = rect.topLeft.x;
    o_rect_y = rect.topLeft.y;
    o_rect_id = rect.id;
    
    // Test deep nested field access
    o_complex_x = complex.bounds.topLeft.x;
    o_scale = complex.scale;
    o_flags = complex.flags;
    
    // Test field modification
    p.x = p.x * 3.0;
    p.y = p.y + 5.0;
    o_modified_x = p.x;
    o_modified_y = p.y;
}