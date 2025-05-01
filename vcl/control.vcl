/**
*   Script testing control flow statement
*/

in float v1;
in float v2;
in float v3;

out float r;

void Main() {
    if (v1 > v2 && v1 > v3) r = v1;
    else if (v2 > v3) r = v2;
    else r = v3;

    while(1) {
        r = r + 1.0;
        if (r > v1 + v3)
            break;
    }

    for (int i = 0; i < 50; i = i + 1)
        r = r + i;
}