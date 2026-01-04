// Test control flow statements: if, while, for, break, continue

// Inputs
in int32 condition_value;
in int32 loop_count;

// ============================================================================
// If Statement Outputs
// ============================================================================

out int32 o_if_simple;           // Simple if
out int32 o_if_else;             // If-else
out int32 o_if_elseif_else;      // If-else if-else chain
out int32 o_if_nested;           // Nested if statements

// ============================================================================
// While Loop Outputs
// ============================================================================

out int32 o_while_basic;         // Basic while loop counter
out int32 o_while_break;         // While with break
out int32 o_while_continue;      // While with continue (sum of even numbers)

// ============================================================================
// For Loop Outputs
// ============================================================================

out int32 o_for_basic;           // Basic for loop counter
out int32 o_for_break;           // For with break
out int32 o_for_continue;        // For with continue (sum of odd numbers)
out int32 o_for_nested;          // Nested for loops

void Main() {
    // ========================================================================
    // If Statement Tests
    // ========================================================================

    // Simple if
    o_if_simple = 0;
    if (condition_value > 0) {
        o_if_simple = 1;
    }

    // If-else
    o_if_else = 0;
    if (condition_value > 10) {
        o_if_else = 1;
    } else {
        o_if_else = 2;
    }

    // If-else if-else chain
    o_if_elseif_else = 0;
    if (condition_value < 0) {
        o_if_elseif_else = 1;  // negative
    } else if (condition_value == 0) {
        o_if_elseif_else = 2;  // zero
    } else if (condition_value < 10) {
        o_if_elseif_else = 3;  // small positive (1-9)
    } else {
        o_if_elseif_else = 4;  // large positive (>=10)
    }

    // Nested if statements
    o_if_nested = 0;
    if (condition_value >= 0) {
        if (condition_value < 5) {
            o_if_nested = 1;  // 0-4
        } else {
            if (condition_value < 10) {
                o_if_nested = 2;  // 5-9
            } else {
                o_if_nested = 3;  // >=10
            }
        }
    } else {
        o_if_nested = 4;  // negative
    }

    // ========================================================================
    // While Loop Tests
    // ========================================================================

    // Basic while loop - count iterations
    o_while_basic = 0;
    int32 while_counter = 0;
    while (while_counter < loop_count) {
        o_while_basic = o_while_basic + 1;
        while_counter = while_counter + 1;
    }

    // While with break - stop at half
    o_while_break = 0;
    int32 break_counter = 0;
    while (break_counter < loop_count) {
        if (break_counter >= loop_count / 2) {
            break;
        }
        o_while_break = o_while_break + 1;
        break_counter = break_counter + 1;
    }

    // While with continue - sum only even numbers
    o_while_continue = 0;
    int32 continue_counter = 0;
    while (continue_counter < loop_count) {
        int32 current = continue_counter;
        continue_counter = continue_counter + 1;

        // Skip odd numbers
        if ((current % 2) != 0) {
            continue;
        }

        o_while_continue = o_while_continue + current;
    }

    // ========================================================================
    // For Loop Tests
    // ========================================================================

    // Basic for loop - count iterations
    o_for_basic = 0;
    for (int32 i = 0; i < loop_count; i = i + 1) {
        o_for_basic = o_for_basic + 1;
    }

    // For with break - stop at half
    o_for_break = 0;
    for (int32 i = 0; i < loop_count; i = i + 1) {
        if (i >= loop_count / 2) {
            break;
        }
        o_for_break = o_for_break + 1;
    }

    // For with continue - sum only odd numbers
    o_for_continue = 0;
    for (int32 i = 0; i < loop_count; i = i + 1) {
        // Skip even numbers
        if ((i % 2) == 0) {
            continue;
        }

        o_for_continue = o_for_continue + i;
    }

    // Nested for loops - count total iterations
    o_for_nested = 0;
    for (int32 i = 0; i < 3; i = i + 1) {
        for (int32 j = 0; j < 4; j = j + 1) {
            o_for_nested = o_for_nested + 1;
        }
    }
}
