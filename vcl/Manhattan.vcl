/**
*   Minimal VCL example
*/

struct Vec2 {
    float32 x;
    float32 y;
};

float32 ManhattanDistance(Vec2 a, Vec2 b) {
    return abs((b.x - a.x) + (b.y - a.y));
}