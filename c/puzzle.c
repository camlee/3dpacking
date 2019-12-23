#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#define VERIFY
// #define DEBUG
// #define VERBOSE

#ifdef DEBUG
    #include <unistd.h>

    #define SLOW_DOWN() usleep(500000)
    #define VERIFY
    // #define VERBOSE
#else
    #define SLOW_DOWN() // Do nothing
#endif

#define RESET   "\x1b[0m"

#define SPACE_WIDTH 5
#define SPACE_HEIGHT 5
#define SPACE_DEPTH 5
#define SPACE_SIZE SPACE_WIDTH * SPACE_HEIGHT * SPACE_DEPTH

#define COMMON_PIECE_SIZE 5
// Note: rotate_piece() only works with cubes right now.
#define SPACE_WILL_BE_FULL

#define STOP_AT_FIRST_SOLUTION
// #define TRACK_PROGRESS
// #define DEBUG_SOLUTION



/*

Attempt at drawing a 3 dimensional view of the axis:

      y
     ^
    /
   /
  /
 o ---------> x
 |
 |
 |
 |
 v
 z

*/


#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2

#ifdef VERIFY
    #define ASSERT_WITHIN_BOUNDS(x, y, z) if ((x) >= SPACE_WIDTH || (y) >= SPACE_HEIGHT || (z) >= SPACE_DEPTH) \
        {printf("\nx, y, or z out of bounds (%u, %u, %u)\n\n(File %s, Line %d, in %s().\n\nTerminating!\n\n", x, y, z, __FILE__, __LINE__, __func__); exit(1);}
#else
    #define ASSERT_WITHIN_BOUNDS(x, y, z) // Do nothing
#endif

#define PIECE_ORIENTATIONS_LIMIT 1000

typedef unsigned __int128 geom;
// Use one of the following, as per the needed bits:
// uint_fast8_t
// uint_fast16_t
// uint_fast32_t
// uint_fast64_t
// uint128_t
// unsigned __int128

typedef unsigned int uint;

// Bitmask to space mapping:
//
// x is for width
// y is for height
// z is for depth
// The values shown within are the bit position, starting at the right
//
//           z=0 z=1 z=2
//           0   1   2      y=0
//   x=0     3   4   5      y=1
//           6   7   8      y=2
//
//           9   10  11     y=0
//   x=1     12  13  14     y=1
//           15  16  17     y=2
//
//           18  19  20     y=0
//   x=2     21  22  23     y=1
//           24  25  26     y=2
//
//
// Where the values show above are the bit that's set.
// So for x=2, y=2, z=2, that's bit 26 set, aka binary: 100000000000000000000000000, base 10: 33554432.
//
// aka counting z = 0, 1, 2 (keeping x and y at 0) gives: 0, 1, 2
// and counting y = 0, 1, 2 (keeping x and z at 0) gives: 0, 3, 6
// and counting x = 0, 1, 2 (keeping y and z at 0) gives: 0, 9, 18

geom l2b(uint x, uint y, uint z){
    /*
    Converts the provided location in x, y, and z to the corresponding bit in the space.
    */
    // return 1 << z;
    ASSERT_WITHIN_BOUNDS(x, y, z);
    return (((geom)1) << (z + (SPACE_DEPTH * y) + (SPACE_DEPTH * SPACE_HEIGHT * x)));
}

void print_piece(geom piece){
    for (uint x=0; x<SPACE_WIDTH; ++x){
        for (uint y=0; y<SPACE_HEIGHT; ++y){
            for (uint z=0; z<SPACE_DEPTH; ++z){
                if (piece & l2b(x, y, z)){
                    printf("(%i, %i, %i)\n", x, y, z);
                }
            }
        }
    }
};

void _print_binary(geom number){
    if (number){
        _print_binary(number >> 1);
        putc((number & 1) ? '1' : '0', stdout);
    }
}

void print_binary(geom piece){
    _print_binary(piece);
    printf("\n");
}

void print_space_fill(geom space, uint fill){
    /*
    Prints a flattened visual representation of what spots in the space are filled.
    Example:

    1 1 1
    1 0 0
    1 0 0

    1 0 0
    0 0 0
    0 0 0

    1 0 0
    0 0 0
    0 0 0

    This is all the spots touching the axis for a 3 x 3 x 3 cube. Aka:
    (0, 0, 0)
    (0, 1, 0)
    (0, 2, 0)
    (0, 0, 1)
    (0, 0, 2)
    (0, 1, 0)
    (0, 2, 0)

    Annotated to show the meaning:

    x = 2 ------|
    x = 1 ----| |
    x = 0 --| | |
            | | |
            v v v

            1 1 1    z = 0
    y = 0   1 0 0    z = 1
            1 0 0    z = 2

            1 0 0
    y = 1   0 0 0
            0 0 0

            1 0 0
    y = 2   0 0 0
            0 0 0

    */
    for (uint y=0; y<SPACE_HEIGHT; ++y){
        for (uint z=0; z<SPACE_DEPTH; ++z){
            for (uint x=0; x<SPACE_WIDTH; ++x){
                if (space & l2b(x, y, z)){
                    printf("%u ", fill);
                } else{
                    printf("0 ");
                }
            }
            printf("\n");
        }
        printf("\n");
    }
};

void print_space_simple(geom space){
    print_space_fill(space, 1);
}

void print_space(geom space){

    for (uint z=0; z<SPACE_DEPTH; ++z){
        printf(" ┌");
        for (uint x=0; x<SPACE_WIDTH; ++x){
            printf("───");
        }
        printf("┐ ");
    }
    printf("\n");


    for (uint z=0; z<SPACE_DEPTH; ++z){
        for (uint y=0; y<SPACE_HEIGHT; ++y){
            printf(" │");
            for (uint x=0; x<SPACE_WIDTH; ++x){
                if (space & l2b(x, y, z)){
                    printf(" ■ ");
                } else{
                    printf("   ");
                }
            }
            printf("│ ");
        }
        printf("\n");
    }
    for (uint z=0; z<SPACE_DEPTH; ++z){
        printf(" └");
        for (uint x=0; x<SPACE_WIDTH; ++x){
            printf("───");
        }
        printf("┘ ");
    }
    printf("\n");
}


void print_bits(geom space){
    unsigned char *b = (unsigned char*) &space;
    unsigned char byte;
    int i, j;

    for (i=sizeof(space)-1;i>=0;i--){
        for (j=7;j>=0;j--){
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}

bool piece_in_array(geom *orientations, uint orientation_count, geom piece){
    for (uint i=0; i<orientation_count; ++i){
        if (orientations[i] == piece){
            return true;
        }
    }
    return false;
}

geom rotate_piece(geom piece, uint axis, uint count){
    /*
    Rotates the part count times around the specified axis.

    */
    geom output = 0;

    int old_x;
    int old_y;
    int old_z;

    int new_x;
    int new_y;
    int new_z;

    if (count == 0){
        return piece;
    }

    if (axis == X_AXIS){
        for (uint x=0; x<SPACE_WIDTH; ++x){
            for (uint y=0; y<SPACE_HEIGHT; ++y){
                for (uint z=0; z<SPACE_DEPTH; ++z){
                    if (piece & l2b(x, y, z)){

                        new_x = (int)x;
                        old_y = (int)y;
                        old_z = (int)z;

                        for (uint r=0; r<count; ++r){
                            new_y = old_z;
                            new_z = -old_y + SPACE_DEPTH - 1;

                            old_y = new_y;
                            old_z = new_z;
                        }

                        if (new_x < 0 || new_x >= SPACE_WIDTH || new_y < 0 || new_y >= SPACE_HEIGHT || new_z < 0 || new_z >= SPACE_DEPTH){
                            printf("Failure rotating piece: goes out of bounds.\n");
                            return piece;
                        } else {
                            output |= l2b((uint)new_x, (uint)new_y, (uint)new_z);
                        }
                    }
                }
            }
        }
    } else if (axis == Y_AXIS){
        for (uint x=0; x<SPACE_WIDTH; ++x){
            for (uint y=0; y<SPACE_HEIGHT; ++y){
                for (uint z=0; z<SPACE_DEPTH; ++z){
                    if (piece & l2b(x, y, z)){

                        old_x = (int)x;
                        new_y = (int)y;
                        old_z = (int)z;

                        for (uint r=0; r<count; ++r){
                            new_x = old_z;
                            new_z = -old_x + SPACE_DEPTH - 1;

                            old_x = new_x;
                            old_z = new_z;
                        }

                        if (new_x < 0 || new_x >= SPACE_WIDTH || new_y < 0 || new_y >= SPACE_HEIGHT || new_z < 0 || new_z >= SPACE_DEPTH){
                            printf("Failure rotating piece: goes out of bounds.\n");
                            return piece;
                        } else {
                            output |= l2b((uint)new_x, (uint)new_y, (uint)new_z);
                        }
                    }
                }
            }
        }
    } else if (axis == Z_AXIS){ // z axis
        for (uint x=0; x<SPACE_WIDTH; ++x){
            for (uint y=0; y<SPACE_HEIGHT; ++y){
                for (uint z=0; z<SPACE_DEPTH; ++z){
                    if (piece & l2b(x, y, z)){

                        old_x = (int)x;
                        old_y = (int)y;
                        new_z = (int)z;

                        for (uint r=0; r<count; ++r){
                            new_x = old_y;
                            new_y = -old_x + SPACE_WIDTH - 1;

                            old_x = new_x;
                            old_y = new_y;
                        }

                        if (new_x < 0 || new_x >= SPACE_WIDTH || new_y < 0 || new_y >= SPACE_HEIGHT || new_z < 0 || new_z >= SPACE_DEPTH){
                            printf("Failure rotating piece: goes out of bounds.\n");
                            return piece;
                        } else {
                            output |= l2b((uint)new_x, (uint)new_y, (uint)new_z);
                        }
                    }
                }
            }
        }
    } else {
        printf("Invalid axis value provided: %u.\n", axis);
    }

    return output;
}

geom shift_piece(geom piece, int x_shift, int y_shift, int z_shift){
    geom output = 0;
    int new_x;
    int new_y;
    int new_z;
    for (uint x=0; x<SPACE_WIDTH; ++x){
        for (uint y=0; y<SPACE_HEIGHT; ++y){
            for (uint z=0; z<SPACE_DEPTH; ++z){
                // printf("Checking x=%u, y=%u, z=%u\n", x, y, z);
                if (piece & l2b(x, y, z)){
                    new_x = (int)x + x_shift;
                    new_y = (int)y + y_shift;
                    new_z = (int)z + z_shift;
                    // printf("new x=%u, y=%u, z=%u\n", new_x, new_y, new_z);
                    if (new_x < 0 || new_x >= SPACE_WIDTH || new_y < 0 || new_y >= SPACE_HEIGHT || new_z < 0 || new_z >= SPACE_DEPTH){
                        // printf("Failure shifting piece: goes out of bounds.\n");
                        return piece;
                    }

                    // printf("Adding new x, y, z to output.\n");

                    output |= l2b((uint)new_x, (uint)new_y, (uint)new_z);
                }
            }
        }
    }

    return output;
}

uint populate_orientations(geom *orientations, geom piece){
    geom new_piece;
    // uint orientation_attempts = 0;
    uint orientation_count = 0;
    for (uint axis=0; axis<3; ++axis){
        for (uint rotation=0; rotation<4; ++rotation){
            for (int x_shift=-(SPACE_WIDTH-1); x_shift<(SPACE_WIDTH); ++x_shift){
                for (int y_shift=-(SPACE_HEIGHT-1); y_shift<(SPACE_HEIGHT); ++y_shift){
                    for (int z_shift=-(SPACE_DEPTH-1); z_shift<(SPACE_DEPTH); ++z_shift){
                        // printf("z_shift=%i, y_shift=%i, x_shift=%i, rotation=%u, axis=%u:\n", z_shift, y_shift, x_shift, rotation, axis);

                        // printf("\nz_shift=%i, y_shift=%i, x_shift=%i, rotation=%u, axis=%u:\n", z_shift, y_shift, x_shift, rotation, axis);
                        // printf("Making piece %u.\n", ++orientation_attempts);

                        // SLOW_DOWN();

                        new_piece = shift_piece(rotate_piece(piece, axis, rotation), x_shift, y_shift, z_shift);
                        if (piece_in_array(orientations, orientation_count, new_piece)){
                            // printf("Same piece. \n");
                            // getchar();
                            continue;
                        } else {
                            // printf("z_shift=%i, y_shift=%i, x_shift=%i, rotation=%u, axis=%u:", z_shift, y_shift, x_shift, rotation, axis);

                            // printf("\nNew piece:\n");
                            // print_space(new_piece);
                            // getchar();
                            orientations[orientation_count] = new_piece;
                            ++orientation_count;
                            if (orientation_count >= PIECE_ORIENTATIONS_LIMIT){
                                return orientation_count;
                            }
                        }
                    }
                }

            }
        }
    }
    return orientation_count;
}

volatile sig_atomic_t keep_running = 1;
volatile sig_atomic_t print_status = 0;

static void sig_handler(int signum)
{
    switch (signum){
        case SIGUSR1:
            print_status = 1;
            break;
        default:
            keep_running = 0;
    }
}

#ifdef COMMON_PIECE_SIZE
bool are_empty_spaces_factors(geom space){
    uint num_connected_holes;
    geom connected_holes = 0;

    uint holes_to_check_index;
    uint holes_to_check[SPACE_SIZE][3] = {{0}};

    geom part;

    uint current_x;
    uint current_y;
    uint current_z;

    uint alt_x;
    uint alt_y;
    uint alt_z;

    space = ~space; // Because we want to find holes


    for (uint x=0; x<SPACE_WIDTH; ++x){
        for (uint y=0; y<SPACE_HEIGHT; ++y){
            for (uint z=0; z<SPACE_DEPTH; ++z){
                part = l2b(x, y, z);
                if ((space & part) && !(connected_holes & part)){ // If it's a hole in the space and we haven't already found this hole
                    num_connected_holes = 0;

                    holes_to_check_index = 0;

                    ++num_connected_holes;
                    connected_holes |= part;

                    holes_to_check[holes_to_check_index][0] = x;
                    holes_to_check[holes_to_check_index][1] = y;
                    holes_to_check[holes_to_check_index][2] = z;
                    ++holes_to_check_index;

                    while (holes_to_check_index > 0){

                        --holes_to_check_index;
                        current_x = holes_to_check[holes_to_check_index][0];
                        current_y = holes_to_check[holes_to_check_index][1];
                        current_z = holes_to_check[holes_to_check_index][2];

                        if (current_x > 0){
                            alt_x = current_x - 1;
                            part = l2b(alt_x, current_y, current_z);
                            if ((space & part) && !(connected_holes & part)){ // If it's a hole in the space and we haven't already found this hole
                                ++num_connected_holes;
                                connected_holes |= part;

                                holes_to_check[holes_to_check_index][0] = alt_x;
                                holes_to_check[holes_to_check_index][1] = current_y;
                                holes_to_check[holes_to_check_index][2] = current_z;
                                ++holes_to_check_index;
                            }
                        }
                        alt_x = current_x + 1;
                        if (alt_x < SPACE_WIDTH){
                            part = l2b(alt_x, current_y, current_z);
                            if ((space & part) && !(connected_holes & part)){ // If it's a hole in the space and we haven't already found this hole
                                ++num_connected_holes;
                                connected_holes |= part;

                                holes_to_check[holes_to_check_index][0] = alt_x;
                                holes_to_check[holes_to_check_index][1] = current_y;
                                holes_to_check[holes_to_check_index][2] = current_z;
                                ++holes_to_check_index;
                            }
                        }

                        if (current_y > 0){
                            alt_y = current_y-1;
                            part = l2b(current_x, alt_y, current_z);
                            if ((space & part) && !(connected_holes & part)){ // If it's a hole in the space and we haven't already found this hole
                                ++num_connected_holes;
                                connected_holes |= part;

                                holes_to_check[holes_to_check_index][0] = current_x;
                                holes_to_check[holes_to_check_index][1] = alt_y;
                                holes_to_check[holes_to_check_index][2] = current_z;
                                ++holes_to_check_index;
                            }
                        }
                        alt_y = current_y + 1;
                        if (alt_y < SPACE_HEIGHT){
                            part = l2b(current_x, alt_y, current_z);
                            if ((space & part) && !(connected_holes & part)){ // If it's a hole in the space and we haven't already found this hole
                                ++num_connected_holes;
                                connected_holes |= part;

                                holes_to_check[holes_to_check_index][0] = current_x;
                                holes_to_check[holes_to_check_index][1] = alt_y;
                                holes_to_check[holes_to_check_index][2] = current_z;
                                ++holes_to_check_index;
                            }
                        }

                        if (current_z > 0){
                            alt_z = current_z-1;
                            part = l2b(current_x, current_y, alt_z);
                            if ((space & part) && !(connected_holes & part)){ // If it's a hole in the space and we haven't already found this hole
                                ++num_connected_holes;
                                connected_holes |= part;

                                holes_to_check[holes_to_check_index][0] = current_x;
                                holes_to_check[holes_to_check_index][1] = current_y;
                                holes_to_check[holes_to_check_index][2] = alt_z;
                                ++holes_to_check_index;
                            }
                        }
                        alt_z = current_z + 1;
                        if (alt_z < SPACE_DEPTH){
                            part = l2b(current_x, current_y, alt_z);
                            if ((space & part) && !(connected_holes & part)){ // If it's a hole in the space and we haven't already found this hole
                                ++num_connected_holes;
                                connected_holes |= part;

                                holes_to_check[holes_to_check_index][0] = current_x;
                                holes_to_check[holes_to_check_index][1] = current_y;
                                holes_to_check[holes_to_check_index][2] = alt_z;
                                ++holes_to_check_index;
                            }
                        }
                    }


                    if (num_connected_holes % COMMON_PIECE_SIZE != 0){
                        return false;
                    }
                }
            }
        }
    }
    return true;
}
#endif


#define assertGeomEqual(value1, value2, message) do { if (!((value1) == (value2))){ printf("\nfailed: %u != %u    %s", value1, value2, message); ++failures;}} while (0)
#define assertFalse(value, message) do { if (value){ printf("\nfailed: %u is true    %s", value, message); ++failures;}} while (0)
#define assertTrue(value, message) do { if (!(value)){ printf("\nfailed: %u is false    %s", value, message); ++failures;}} while (0)
#define assertGeomIn(value, array, length, message) do {bool match = false; for (uint _assertGeomIn_i=0; _assertGeomIn_i<length; ++_assertGeomIn_i){if ((value) == (array[_assertGeomIn_i])){match = true; break;}}; if (!match){ printf("\nfailed: %u is not in the array    %s", value, message);}} while (0)

uint test(){
    uint failures = 0;

    if (SPACE_WIDTH > 0 && SPACE_HEIGHT > 0 && SPACE_DEPTH > 0){

        assertGeomEqual(l2b(0, 0, 0), 1, "l2b 0");
        assertGeomEqual(l2b(0, 0, 1), 2, "l2b one z");

        assertGeomEqual(l2b(0, 0, 0), 0b1, "l2b 0 binary");
        assertGeomEqual(l2b(0, 0, 1), 0b10, "l2b one z binary");

        assertGeomEqual(l2b(0, 0, 0), 1 << 0, "l2b 0 bit shift");
        assertGeomEqual(l2b(0, 0, 1), 1 << 1, "l2b one z bit shift");

        if (SPACE_DEPTH == 3){
            assertGeomEqual(l2b(0, 1, 0), 0b1000, "l2b one y");
        }
        if (SPACE_DEPTH == 3 && SPACE_HEIGHT == 3){
            assertGeomEqual(l2b(1, 0, 0), 0b1000000000, "l2b one x");
            assertGeomEqual(l2b(1, 1, 1), 0b10000000000000, "l2b one x, y, and z");
        }

        uint array_len = 3;
        geom array[3] = {0b001, 0b010, 0b011};

        assertFalse(piece_in_array(array, array_len, 0b100), "piece not in array");
        assertTrue(piece_in_array(array, array_len, 0b010), "piece in array");

        assertGeomEqual(shift_piece(l2b(0, 0, 0), 0, 0, 0), l2b(0, 0, 0), "No shifting.");
        assertGeomEqual(shift_piece(l2b(0, 0, 0), 1, 0, 0), l2b(1, 0, 0), "Shift by one.");
        assertGeomEqual(shift_piece(l2b(0, 0, 0), 2, 0, 0), l2b(2, 0, 0), "Shift by two.");
        assertGeomEqual(shift_piece(l2b(2, 0, 0), -1, 0, 0), l2b(1, 0, 0), "Shift by minus one.");
        assertGeomEqual(shift_piece(l2b(2, 0, 0), -2, 0, 0), l2b(0, 0, 0), "Shift by minus two.");

        assertGeomEqual(shift_piece(l2b(0, 0, 0), 0, 1, 0), l2b(0, 1, 0), "Shift by one y.");
        assertGeomEqual(shift_piece(l2b(0, 0, 0), 0, 0, 1), l2b(0, 0, 1), "Shift by one z.");

        assertGeomEqual(shift_piece(l2b(0, 0, 0), 1, 1, 1), l2b(1, 1, 1), "Shift by one x, y, and z.");

        assertGeomEqual(shift_piece(l2b(1, 0, 0) | l2b(1, 0, 1), -1, 1, 0), l2b(0, 1, 0) | l2b(0, 1, 1), "Shift multiple locations.");

        assertGeomEqual(shift_piece(l2b(1, 0, 0), SPACE_WIDTH, 0, 0), l2b(1, 0, 0),
            "Shift past edge of space in positive direction.");
        assertGeomEqual(shift_piece(l2b(SPACE_WIDTH-1, 0, 0), 1, 1, 0), l2b(SPACE_WIDTH-1, 0, 0),
            "Shift past edge of space in positive direction (should return same geom).");
        assertGeomEqual(shift_piece(l2b(0, 0, 0), -1, 0, 0), l2b(0, 0, 0),
            "Shift past edge of space in negative direction.");

        if (SPACE_DEPTH == 3 && SPACE_HEIGHT == 3 && SPACE_DEPTH == 3){
            assertGeomEqual(rotate_piece(l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 0, 1) | l2b(0, 0, 2), Y_AXIS, 1),
                l2b(0, 0, 0) | l2b(1, 0, 2) | l2b(2, 0, 2) | l2b(0, 0, 1) | l2b(0, 0, 2), "Rotate by one y.");

            assertGeomEqual(rotate_piece(l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 0, 1) | l2b(0, 0, 2), Y_AXIS, 2),
                l2b(2, 0, 2) | l2b(1, 0, 2) | l2b(2, 0, 0) | l2b(2, 0, 1) | l2b(0, 0, 2), "Rotate by two y.");

            assertGeomEqual(rotate_piece(l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 0, 1) | l2b(0, 0, 2), X_AXIS, 1),
                l2b(0, 0, 2) | l2b(1, 0, 2) | l2b(2, 0, 2) | l2b(0, 1, 2) | l2b(0, 2, 2), "Rotate by one x.");

            assertGeomEqual(rotate_piece(l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 0, 1) | l2b(0, 0, 2), X_AXIS, 2),
                l2b(0, 2, 0) | l2b(1, 2, 2) | l2b(2, 2, 2) | l2b(0, 2, 1) | l2b(0, 2, 2), "Rotate by two x.");


            // Testing populate_orientations:
            geom test_piece = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 0, 1) | l2b(0, 0, 2);

            geom test_orientations[PIECE_ORIENTATIONS_LIMIT] = {0};
            assertTrue(populate_orientations(test_orientations, test_piece) == 24, "24 unique orientations should have been found.");

            assertGeomIn(test_piece, test_orientations, PIECE_ORIENTATIONS_LIMIT, "The original piece should be included as one of the orientations.");
            assertGeomIn(l2b(0, 0, 0) | l2b(1, 0, 2) | l2b(2, 0, 2) | l2b(0, 0, 1) | l2b(0, 0, 2), test_orientations, PIECE_ORIENTATIONS_LIMIT, "A single rotation around y should be included as one of the orientations.");
            assertGeomIn(l2b(0, 1, 0) | l2b(1, 1, 2) | l2b(2, 1, 2) | l2b(0, 1, 1) | l2b(0, 1, 2), test_orientations, PIECE_ORIENTATIONS_LIMIT, "A single rotation around y plus a shift in positive y should be included as one of the orientations.");

        }
    }

    return failures;
}


int main(){
    struct sigaction action;
    action.sa_handler = sig_handler;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGUSR1, &action, NULL);

    printf("\nRunning tests...\n");
    uint failures = test();
    if (failures == 0){
        printf("passed!\n");
    } else {
        printf("\nThere were %u test failures. Exiting.\n", failures);
        return 1;
    }



    printf("\nStarting...\n");

    // Problem 1:
    // Space: 5 x 3 x 2
    // #define NUM_PIECES 6
    // geom piece1 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(3, 0, 0) | l2b(4, 0, 0);
    // geom piece2 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0);
    // geom piece3 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0);
    // geom piece4 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0);
    // geom piece5 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0);
    // geom piece6 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0);



    // Problem 4:
    // Space: 3 x 3 x 3
    // #define NUM_PIECES 6
    // geom piece1 = l2b(0, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0) | l2b(2, 1, 0);
    // geom piece2 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(1, 1, 0) | l2b(2, 0, 0);
    // geom piece3 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(1, 1, 0) | l2b(2, 0, 1);
    // geom piece4 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(0, 1, 0) | l2b(0, 1, 1);
    // geom piece5 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(1, 1, 0) | l2b(1, 1, 1);
    // geom piece6 = l2b(0, 0, 0) | l2b(0, 1, 0) | l2b(0, 1, 1) | l2b(1, 1, 0) | l2b(1, 2, 0);



    // Problem 5:
    // Space: 3 x 3 x 3
    // #define NUM_PIECES 6
    // geom piece1 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(1, 1, 0);
    // geom piece2 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0);
    // geom piece3 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0);
    // geom piece4 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(2, 1, 0);
    // geom piece5 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0) | l2b(2, 1, 0);
    // geom piece6 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0) | l2b(2, 1, 0);


    // geom piece1 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0);
    // geom piece2 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0);
    // geom piece3 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0);
    // geom piece4 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0) | l2b(2, 1, 0);
    // geom piece5 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0) | l2b(2, 1, 0);
    // geom piece6 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0) | l2b(2, 1, 0);


    // Coding Challenge:
    // Space: 3 x 3 x 3
    // #define NUM_PIECES 7
    // geom piece1 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(1, 1, 0);
    // geom piece2 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(2, 1, 0);
    // geom piece3 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(1, 1, 0);
    // geom piece4 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(1, 1, 0) | l2b(2, 1, 0);
    // geom piece5 = l2b(0, 0, 1) | l2b(1, 0, 1) | l2b(1, 1, 1) | l2b(1, 0, 0);
    // geom piece6 = l2b(0, 0, 1) | l2b(1, 0, 1) | l2b(0, 1, 1) | l2b(1, 0, 0);
    // geom piece7 = l2b(0, 0, 1) | l2b(1, 0, 1) | l2b(1, 0, 0) | l2b(1, 1, 0);

    // geom pieces[NUM_PIECES] = {piece1, piece2, piece3, piece4, piece5, piece6, piece7};
    // char *piece_colors[NUM_PIECES] = {
    //     "\e[38;2;255;0;0m",     // Red
    //     "\e[38;2;0;128;128m",   // Teal
    //     "\e[38;2;0;100;0m",     // Dark Green
    //     "\e[38;2;154;255;154m", // Light Green
    //     "\e[38;2;255;180;0m",   // Light Orange
    //     "\e[38;2;0;20;205m",    // Blue
    //     "\e[38;2;170;255;154m", // Light Green
    // };

    // #ifdef DEBUG_SOLUTION
    // geom solution[NUM_PIECES] = {
    //     l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(1, 1, 0),
    //     l2b(0, 2, 2) | l2b(1, 2, 2) | l2b(2, 1, 2) | l2b(2, 2, 2),
    //     l2b(2, 0, 0) | l2b(2, 0, 1) | l2b(2, 0, 2) | l2b(2, 1, 1),
    //     l2b(0, 0, 1) | l2b(0, 0, 2) | l2b(0, 1, 0) | l2b(0, 1, 1),
    //     l2b(1, 2, 0) | l2b(2, 1, 0) | l2b(2, 2, 0) | l2b(2, 2, 1),
    //     l2b(0, 2, 0) | l2b(0, 2, 1) | l2b(1, 1, 1) | l2b(1, 2, 1),
    //     l2b(0, 1, 2) | l2b(1, 0, 1) | l2b(1, 0, 2) | l2b(1, 1, 2),
    // };
    // #endif

    // Small physical wooden puzzle:
    // Space: 3 x 3 x 3
    // #define NUM_PIECES 6
    // geom piece1 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0);
    // geom piece2 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 0, 1);
    // geom piece3 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(1, 0, 1);
    // geom piece4 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(1, 1, 0) | l2b(1, 0, 1) | l2b(2, 0, 1);
    // geom piece5 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(1, 0, 1) | l2b(1, 1, 1);
    // geom piece6 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(1, 0, 1) | l2b(1, 1, 1);

    // geom pieces[NUM_PIECES] = {piece1, piece2, piece3, piece4, piece5, piece6};



    // Real problem:
    // Space: 5 x 5 x 5
    #define NUM_PIECES 25
    geom piece1  = l2b(0,0,0) | l2b(1,0,0) | l2b(2,0,0) | l2b(2,1,0) | l2b(3,1,0); // color=[238, 238, 0]), # Yellow
    geom piece2  = l2b(0,0,0) | l2b(1,0,0) | l2b(0,1,0) | l2b(0,2,0) | l2b(1,2,0); // color=[245, 238, 0]), # Yellow "U"
    geom piece3  = l2b(0,0,0) | l2b(1,0,0) | l2b(2,0,0) | l2b(0,1,0) | l2b(0,2,0); // color=[255, 165, 0]), # Light Orange "Symetric L"
    geom piece4  = l2b(0,0,0) | l2b(0,0,1) | l2b(0,0,2) | l2b(0,0,3) | l2b(0,0,4); // color=[255, 180, 0]), # Light Orange "Chocolate Bar"
    geom piece5  = l2b(0,0,0) | l2b(1,0,0) | l2b(1,1,0) | l2b(1,0,1) | l2b(2,0,1); // color=[238, 154, 0]), # Dark Orange "Y-ish"
    geom piece6  = l2b(0,0,0) | l2b(1,0,0) | l2b(2,0,0) | l2b(2,1,0) | l2b(2,1,1); // color=[238, 145, 0]), # Dark Orange "L with hook off short end"
    geom piece7  = l2b(0,0,0) | l2b(0,0,1) | l2b(1,0,0) | l2b(2,0,0) | l2b(2,1,0); // color=[238, 154, 0]), # Dark Orange "L with hook off long end"
    geom piece8  = l2b(0,0,0) | l2b(1,0,0) | l2b(2,0,0) | l2b(1,1,0) | l2b(1,2,0); // color=[255, 0, 0]), # Red "T"
    geom piece9  = l2b(0,0,0) | l2b(1,0,0) | l2b(1,1,0) | l2b(2,1,0) | l2b(2,2,0); // color=[255, 0, 20]), # Red "W"
    geom piece10 = l2b(0,0,0) | l2b(1,0,0) | l2b(2,0,0) | l2b(2,1,0) | l2b(2,0,1); // color=[200, 0, 0]), # Dark Red "L" with hook off corner"
    geom piece11 = l2b(0,0,0) | l2b(0,1,0) | l2b(1,0,0) | l2b(2,0,0) | l2b(2,0,1); // color=[200, 20, 0]), # Dark Red "L with hook off long end"
    geom piece12 = l2b(0,0,0) | l2b(1,0,0) | l2b(2,0,0) | l2b(3,0,0) | l2b(3,1,0); // color=[142, 56, 142]), # Purple "L"
    geom piece13 = l2b(0,1,0) | l2b(1,1,0) | l2b(2,1,0) | l2b(1,0,0) | l2b(1,2,0); // color=[142, 40, 142]), # Purple "Cross"
    geom piece14 = l2b(0,0,0) | l2b(0,0,1) | l2b(1,0,0) | l2b(1,1,0) | l2b(1,1,1); // color=[0, 0, 205]), # Blue "Two towers"
    geom piece15 = l2b(0,0,0) | l2b(1,0,0) | l2b(2,0,0) | l2b(1,1,0) | l2b(2,0,1); // color=[0, 20, 205]), # Blue "L with hook off middle of long end"
    geom piece16 = l2b(0,0,0) | l2b(1,0,0) | l2b(1,1,0) | l2b(2,0,0) | l2b(2,1,0); // color=[0, 128, 128]), # Teal "Foam finger"
    geom piece17 = l2b(0,0,0) | l2b(0,1,0) | l2b(1,1,0) | l2b(2,1,0) | l2b(2,2,0); // color=[20, 128, 128]), # Teal "Z"
    geom piece18 = l2b(0,0,0) | l2b(1,0,0) | l2b(1,0,1) | l2b(2,0,1) | l2b(2,1,1); // color=[173, 255, 47]), # Yellow-Green "Left-handed"
    geom piece19 = l2b(0,0,0) | l2b(0,0,1) | l2b(1,0,0) | l2b(1,1,0) | l2b(2,1,0); // color=[173, 234, 47]), # Yellow-Green "Right-handed"
    geom piece20 = l2b(0,0,0) | l2b(1,0,0) | l2b(1,1,0) | l2b(1,0,1) | l2b(2,0,0); // color=[154, 255, 154]), # Light Green "Bent Cross"
    geom piece21 = l2b(0,0,0) | l2b(1,0,0) | l2b(2,0,0) | l2b(1,1,0) | l2b(2,0,1); // color=[170, 255, 154]), # Light Green "L with hook off side of long end"
    geom piece22 = l2b(0,0,0) | l2b(1,0,0) | l2b(2,0,0) | l2b(3,0,0) | l2b(2,1,0); // color=[162, 205, 90]), # Olive Green "Rifle"
    geom piece23 = l2b(0,0,0) | l2b(1,0,0) | l2b(1,1,0) | l2b(1,2,0) | l2b(2,1,0); // color=[150, 205, 90]), # Olive Green "Y-ish"
    geom piece24 = l2b(0,0,0) | l2b(1,0,0) | l2b(0,1,0) | l2b(1,1,0) | l2b(1,1,1); // color=[0, 100, 0]), # Dark Green "Base and tower"
    geom piece25 = l2b(0,0,0) | l2b(1,0,0) | l2b(1,0,1) | l2b(1,1,0) | l2b(2,1,0); // color=[20, 100, 0]), # Dark Green "Y-ish"

    geom pieces[NUM_PIECES] = {
        piece4,
        piece8,
        piece1,
        piece22,
        piece6,
        piece24,
        piece2,
        piece19,
        piece11,
        piece3,
        piece20,
        piece16,
        piece15,
        piece23,
        piece17,
        piece5,
        piece13,
        piece21,
        piece7,
        piece9,
        piece18,
        piece10,
        piece12,
        piece14,
        piece25
    };

    //
    char *piece_colors[NUM_PIECES] = {
        "\e[38;2;255;180;0m",   // piece4: Light Orange "Chocolate Bar"
        "\e[38;2;255;0;0m",     // piece8: Red "T"
        "\e[38;2;238;238;0m",   // piece1: Yellow
        "\e[38;2;162;205;90m",  // piece22: Olive Green "Rifle"
        "\e[38;2;238;145;0m",   // piece6: Dark Orange "L with hook off short end"
        "\e[38;2;0;100;0m",     // piece24: Dark Green "Base and tower"
        "\e[38;2;245;238;0m",   // piece2: Yellow "U"
        "\e[38;2;173;234;47m",  // piece19: Yellow-Green "Right-handed"
        "\e[38;2;200;20;0m",    // piece11: Dark Red "L with hook off long end"
        "\e[38;2;255;165;0m",   // piece3: Light Orange "Symetric L"
        "\e[38;2;154;255;154m", // piece20: Light Green "Bent Cross"
        "\e[38;2;0;128;128m",   // piece16: Teal "Foam finger"
        "\e[38;2;0;20;205m",    // piece15: Blue "L with hook off middle of long end"
        "\e[38;2;150;205;90m",  // piece23: Olive Green "Y-ish"
        "\e[38;2;20;128;128m",  // piece17: Teal "Z"
        "\e[38;2;238;154;0m",   // piece5: Dark Orange "Y-ish"
        "\e[38;2;142;40;142m",  // piece13: Purple "Cross"
        "\e[38;2;170;255;154m", // piece21: Light Green "L with hook off side of long end"
        "\e[38;2;238;154;0m",   // piece7: Dark Orange "L with hook off long end"
        "\e[38;2;255;0;20m",    // piece9: Red "W"
        "\e[38;2;173;255;47m",  // piece18: Yellow-Green "Left-handed"
        "\e[38;2;200;0;0m",     // piece10: Dark Red "L" with hook off corner"
        "\e[38;2;142;56;142m",  // piece12: Purple "L"
        "\e[38;2;0;0;205m",     // piece14: Blue "Two towers"
        "\e[38;2;20;100;0m"     // piece25: Dark Green "Y-ish"
    };

    void print_colored_pieces_in_space(
        geom orientations_history[NUM_PIECES][NUM_PIECES][PIECE_ORIENTATIONS_LIMIT],
        uint orientation_history[NUM_PIECES],
        uint piece_placing_history[NUM_PIECES],
        uint to_piece){

        for (uint z=0; z<SPACE_DEPTH; ++z){
            printf(" ┌");
            for (uint x=0; x<SPACE_WIDTH; ++x){
                printf("───");
            }
            printf("┐ ");
        }
        printf("\n");


        for (uint z=0; z<SPACE_DEPTH; ++z){
            for (uint y=0; y<SPACE_HEIGHT; ++y){
                printf(" │");
                for (uint x=0; x<SPACE_WIDTH; ++x){
                    bool found_match = false;
                    uint matching_piece = 0;

                    for (uint i=0; i<to_piece; ++i){
                        uint piece_index = piece_placing_history[i];
                        if (orientations_history[i][piece_index][orientation_history[i]] & l2b(x, y, z)){
                            matching_piece = i;
                            found_match = true;
                            break;
                        }
                    }
                    if (found_match){
                        printf(piece_colors[piece_placing_history[matching_piece]]);
                        #if NUM_PIECES > 10
                        printf(" ■ "); // Not printing the piece number when it could be more than 1 digit
                        #else
                        printf(" %u ", piece_placing_history[matching_piece]+1);
                        #endif
                        printf(RESET);
                    } else{
                        printf("   ");
                    }
                }
                printf("│ ");
            }
            printf("\n");
        }
        for (uint z=0; z<SPACE_DEPTH; ++z){
            printf(" └");
            for (uint x=0; x<SPACE_WIDTH; ++x){
                printf("───");
            }
            printf("┘ ");
        }
        printf("\n");
    }

    printf("Pieces defined!\n");

    geom space = 0;
    geom full_space = 0;
    // Initialize full_space. There's got to be a better way of doing this but whatever:
    for (uint x=0; x<SPACE_WIDTH; ++x){
        for (uint y=0; y<SPACE_HEIGHT; ++y){
            for (uint z=0; z<SPACE_DEPTH; ++z){
                full_space |= l2b(x, y, z);
            }
        }
    }

    printf("full_space:\n");
    print_space(full_space);

    geom orientations[NUM_PIECES][PIECE_ORIENTATIONS_LIMIT] = {{0}};
    uint orientation_counts[NUM_PIECES] = {0};
    double total_permutations = 1;
    #ifdef TRACK_PROGRESS
    double permutations_tried = 0;
    #endif
    long unsigned int loop_counter = 0;
    long unsigned int previous_loop_counter = 0;

    clock_t start = clock();
    clock_t previous = start;
    clock_t end;
    double duration;

    for (uint i=0; i<NUM_PIECES; i++){
        orientation_counts[i] = populate_orientations(orientations[i], pieces[i]);
        printf("Found %u unique orientations for piece %u.\n", orientation_counts[i], i+1);
        total_permutations *= orientation_counts[i];
    }

    printf("Total permutations: %e\n", total_permutations);


    // geom piece1_orientations[PIECE_ORIENTATIONS_LIMIT];
    // uint num_piece1_orientations = populate_orientations(piece1_orientations, piece1);

    // printf("Found %u unique orientations.\n", num_piece1_orientations);


    // geom piece2_orientations[PIECE_ORIENTATIONS_LIMIT];
    // uint num_piece2_orientations = populate_orientations(piece2_orientations, piece2);

    // printf("Found %u unique orientations.\n", num_piece2_orientations);

    #ifndef STOP_AT_FIRST_SOLUTION
    uint solution_count = 0;
    #endif

    uint piece_history_index = 0; // The number of the piece we're placing (1, 2, 3, ...)
    uint piece_placing_index = 0; // The index of the piece we're currently trying to place.
        // Not necessarily equal to piece_history_index
    #ifndef VERBOSE
    uint maximum_piece_placing = 0;
    #endif
    uint orientation_placing = 0; // The index of the orientation we're trying for the
        // piece we're trying to place.

    uint orientation_history[NUM_PIECES] = {0}; // Once we've placed a piece using an
        // orientation, we'll keep track of that orientation's index here, so we can
        // resume where we left off if necessary.
    geom space_history[NUM_PIECES] = {0};


    // Once we've placed a piece, we'll trim down the orientations to those that still fit.
    // Keeping track of those and the history in this data structure so we can quickly backup
    // if we need to take out a piece:
    static geom orientations_history[NUM_PIECES][NUM_PIECES][PIECE_ORIENTATIONS_LIMIT] = {{{0}}};
    uint orientation_counts_history[NUM_PIECES][NUM_PIECES] = {{0}};

    // We're going to pick the next piece to place based on what we think is fastest.
    // So it won't necessarily be in order (1, 2, 3, ...). We need to keep track of what
    // piece we were placing last
    uint piece_placing_history[NUM_PIECES] = {0};

    #ifdef TRACK_PROGRESS
    double permutations_history[NUM_PIECES] = {total_permutations};

    long unsigned int backout_no_orientations_left_for_a_piece = 0;
    long unsigned int backout_some_part_of_space_cannot_be_filled = 0;
    #ifdef COMMON_PIECE_SIZE
    long unsigned int backout_are_empty_spaces_factors = 0;
    #endif
    #endif

    // Populating the initial history record (for piece_placing_index):
    for (uint i=0; i<NUM_PIECES; ++i){
        orientation_counts_history[0][i] = orientation_counts[i];
        for (uint j=0; j<orientation_counts[i]; ++j){
            orientations_history[0][i][j] = orientations[i][j];
        }
    }

    bool backout = false;

    end = clock();
    duration = ((double) (end - previous)) / CLOCKS_PER_SEC;
    previous = end;
    printf("Setup in %.1f seconds.\n", duration);

    while (keep_running) {

        // First, some checks we want to do each loop:
        #ifdef VERBOSE
//           printf("Piece %u, orientation %u.\n", piece_placing+1, orientation_placing+1);
        #endif
        SLOW_DOWN();

        if (!keep_running){
            printf("\nInterupt detected. Exiting.\n");
            printf("\nPlaced %u pieces.\n", piece_history_index+1);
            break;
        }
        if (print_status){
            print_status = 0;
            printf("\nPlaced %u pieces.\n", piece_history_index+1);
            // print_space(space);
            // printf("Space by piece number:\n");
            print_colored_pieces_in_space(orientations_history, orientation_history, piece_placing_history, piece_history_index);
        }

        ++loop_counter;
        if (loop_counter % 1000000 == 0){
            end = clock();
            duration = ((double) (end - previous)) / CLOCKS_PER_SEC;
            previous = end;
            #ifdef TRACK_PROGRESS
            printf("Tried %e permutations of %e (%.5f %%) in %.1f seconds at a rate of %.2f million loops/second.\n",
                permutations_tried, total_permutations, permutations_tried / total_permutations * 100.0, duration,
                ((double)(loop_counter-previous_loop_counter))/duration/1000000.0);
            // printf("  %.1f % backout_no_orientations_left_for_a_piece\n",
            //     100.0 * (double)backout_no_orientations_left_for_a_piece / (double)(backout_no_orientations_left_for_a_piece + backout_are_empty_spaces_factors + backout_some_part_of_space_cannot_be_filled));
            // printf("  %.1f % backout_some_part_of_space_cannot_be_filled\n",
            //     100.0 * (double)backout_some_part_of_space_cannot_be_filled / (double)(backout_no_orientations_left_for_a_piece + backout_are_empty_spaces_factors + backout_some_part_of_space_cannot_be_filled));
            // printf("  %.1f % backout_are_empty_spaces_factors\n",
            //     100.0 * (double)backout_are_empty_spaces_factors / (double)(backout_no_orientations_left_for_a_piece + backout_are_empty_spaces_factors + backout_some_part_of_space_cannot_be_filled));
            #else
            printf("%.1f seconds at a rate of %.2f million loops/second.\n", duration,
                ((double)(loop_counter-previous_loop_counter))/duration/1000000.0);
            #endif
            previous_loop_counter = loop_counter;
        }

        // The actual logic. We do one of two things: backup the piece we placed last or place a new piece:
        if (backout){ // The latest placed piece makes it impossible to solve the rest in one way or another.
            backout = false;

            // Trying the next orientation for this same piece.
            // But wait, have we run out of orientations? If yes, back up to the previous piece.
            // Doing this in a while loop as that piece might also have run out of orientations:
            // We need to keep backing up until we find a piece with more orientations.
            // printf("Piece %u: Couldn't place orientation %u/%u\n", piece_history_index+1, orientation_placing+1, orientation_counts_history[piece_placing][piece_placing]);
            do{
                // Backup, takout a piece, and try placing it differently.

                if (piece_history_index == 0){
                    printf("\nTried all the permutations.\n");
                    keep_running = 0;
                    break;
                }
                --piece_history_index; // Trying to place the previous piece again
                orientation_placing = orientation_history[piece_history_index]; // Starting back at the orientation we successfully placed.
                // printf("Back to piece %u at orientation %u/%u\n", piece_history_index+1, orientation_placing+1, orientation_counts_history[piece_history_index][piece_history_index]);
                space = space_history[piece_history_index]; // Resetting the space to what is was before the previous piece was placed
                piece_placing_index = piece_placing_history[piece_history_index];

                // Go to the next orientation:
                // If that was the last orientation, we loop again to backup even more:
            } while (++orientation_placing >= orientation_counts_history[piece_history_index][piece_placing_index]);
            // printf("On to piece %u: orientation %u/%u\n", piece_history_index+1, orientation_placing+1, orientation_counts_history[piece_history_index][piece_history_index]);
        } else {
            // Place this piece!
            geom placing = orientations_history[piece_history_index][piece_placing_index][orientation_placing];
            #ifdef VERIFY
            if (!placing){
                printf("\nWe're trying to place an empty piece (piece %u, orientation %u)!!! :(. Something went wrong.\n\nExiting.\n", piece_placing_index+1, orientation_placing);
                exit(1);
            }
            if (space & placing){
                printf("\nWe're about to place a piece (piece %u, orientation %u) into the space overlapping another piece!!! :(. Something wen wrong.\n\nExiting.\n", piece_placing_index+1, orientation_placing);
                exit(1);
            }
            #endif

            orientation_history[piece_history_index] = orientation_placing; // Keeping track of what orientation we're placing
            space_history[piece_history_index] = space; // Keeping track of what the space looked like before we place the piece
            space |= placing; // Putting the piece in the space.

            #ifndef VERBOSE
            if (piece_history_index > maximum_piece_placing){
                maximum_piece_placing = piece_history_index;
            #endif
                printf("Placed piece %u (%u/%u) with orientation %u/%u.\n",
                    piece_placing_index+1,
                    piece_history_index, NUM_PIECES,
                    orientation_placing+1,
                    orientation_counts_history[piece_history_index-1][piece_placing_index]);
                print_colored_pieces_in_space(orientations_history, orientation_history, piece_placing_history, piece_history_index);
            #ifndef VERBOSE
            }
            #endif

            #ifdef DEBUG_SOLUTION
            for (uint i=0; i<=piece_history_index; ++i){
                printf("%u ", piece_placing_history[i]);
            }
            printf("\n");
            uint continuous_matches = 0;
            for (uint i=0; i<=piece_history_index; ++i){
                geom our_orientation = orientations_history[i][piece_placing_history[i]][orientation_history[i]];
                if (our_orientation == solution[piece_placing_history[i]]){
                    printf("+ ", i);
                    ++continuous_matches;
                } else {
                    printf("  ");
                    continuous_matches = 0;
                }
            }
            printf("\n");
            #endif

            ++piece_history_index; // Moving on to the next piece
            orientation_placing = 0; // Starting with the first orientation for the next piece.

            // Now, checking if there's any reason to quit or undo this placement.
            // Set backout to true if we need to undo this placement and try the next orientation.

            // If we've placed the last piece, we either stop or backout to find more solutions:
            if (piece_history_index == NUM_PIECES){ // Have we placed all the pieces?
                #ifdef STOP_AT_FIRST_SOLUTION
                    printf("\nStopping at first solution!\n");
                    break; // We've placed all the pieces: we're done!
                #else
                    ++solution_count; // Counting this as a solution and continuing.
                    backout = true;
                #endif
            }

            #ifdef TRACK_PROGRESS
            double new_permutations = 1;
            #endif

            #ifdef SPACE_WILL_BE_FULL
            geom potential_space_fill = space;
            #endif

            // Trimming down what remaining pieces and orientations we have:
            // Also, if a piece doesn't fit anymore, we backout.
            uint *orientations_counts_at_previous_piece = orientation_counts_history[piece_history_index-1];
            uint *orientations_counts_at_this_piece = orientation_counts_history[piece_history_index];
            uint smallest_orientations_count = 10000;
            uint piece_placing_index_for_smallest_orientations_count;
            for (uint i=0; i<NUM_PIECES; ++i){ // Loop over all pieces
                if (i == piece_placing_index || orientations_counts_at_previous_piece[i] == 0){ // Zero here is a sentinel for already placed
                    // printf("Skipping orientations for piece %u\n", i+1);
                    orientations_counts_at_this_piece[i] = 0; // Setting the sentinel of 0 to mena already placed.
                    continue; // Only worrying about the  remaining pieces
                }
                geom *piece_orientations = orientations_history[piece_history_index-1][i];
                uint new_orientation_count = 0;
                uint orientation_count = orientations_counts_at_previous_piece[i];
                for (uint remaining_orientation=0; remaining_orientation<orientation_count; ++remaining_orientation){ // Loop over it's orientations
                    geom piece_orientation = piece_orientations[remaining_orientation];
                    if (!(space & piece_orientation)){
                        // If this piece still fits in the space in this orientation:
                        orientations_history[piece_history_index][i][new_orientation_count] = piece_orientation;
                        ++new_orientation_count;
                        #ifdef SPACE_WILL_BE_FULL
                        potential_space_fill |= piece_orientation;
                        #endif
                    }
                }
                orientations_counts_at_this_piece[i] = new_orientation_count;

                #ifdef TRACK_PROGRESS
                new_permutations *= new_orientation_count;
                #endif

                // printf("Trimmed piece %u from %u to %u.\n", remaining_piece+1, orientation_count, new_orientation_count);
                // printf("Down to %u orientations for piece %u.\n", new_orientation_count, remaining_piece+1);

                if (new_orientation_count == 0){ // Some piece does not fit anymore
                    // printf("  backout\n");
                    #ifdef VERBOSE
                    printf("Backing out: no orientations left for piece %u.\n", i+1);
                    #endif
                    backout = true;
                    #ifdef TRACK_PROGRESS
                    ++backout_no_orientations_left_for_a_piece;
                    #endif
                    break;
                    // printf("Piece %u does not fit anymore.\n", remaining_piece+1);
                }

                // Tracking which piece has the fewest orientations left so we can try it next for speed:
                if (new_orientation_count < smallest_orientations_count){
                    smallest_orientations_count = new_orientation_count;
                    piece_placing_index_for_smallest_orientations_count = i;
                }

            }

            // Checking if it's still possible to fill in every spot in the space:
            #ifdef SPACE_WILL_BE_FULL
            if (!backout && potential_space_fill != full_space){
                #ifdef VERBOSE
                printf("Backing out: some part of space cannot be filled.\n");
                // print_space(potential_space_fill);
                #endif
                backout = true;
                #ifdef TRACK_PROGRESS
                ++backout_some_part_of_space_cannot_be_filled;
                #endif
            }
            #endif

            // Checking if it's still possible to fit the pieces into the divisions in the space:
            #ifdef COMMON_PIECE_SIZE
            if (!backout && !are_empty_spaces_factors(space)){
                // If all our pieces are of size 3 unit cubes (for example) and we've split the space into two (or more)
                // separate holes, the space isn't solvable unless each of those holes has a number of unit cubes
                // that's a multiple of 3.
                #ifdef VERBOSE
                printf("Backing out: empty spaces are not factors.");
                #endif
                backout = true;
                #ifdef TRACK_PROGRESS
                ++backout_are_empty_spaces_factors;
                #endif

                // print_space(space);
                // return 0;
            }
            #endif

            // Figure out the best order to try and place the remaining pieces in:
            #ifdef VERBOSE
            if (!backout){
                printf("Decided to place piece %u next (%u orientations).\n",
                    piece_placing_index_for_smallest_orientations_count+1,
                    orientation_counts_history[piece_history_index][piece_placing_index_for_smallest_orientations_count]);
            }
            #endif
            piece_placing_index = piece_placing_index_for_smallest_orientations_count;
            // piece_placing_index = piece_history_index;
            piece_placing_history[piece_history_index] = piece_placing_index;

            #ifdef TRACK_PROGRESS
            // printf("Scanning pieces ruled out %e permutations.\n", permutations_history[piece_history_index-1] - new_permutations);
            permutations_tried += (
                permutations_history[piece_history_index-1] / (double)orientations_counts_at_previous_piece[piece_history_index-1]
                ) - new_permutations;

            // printf("%e / %e = %e... - %e = %e\n",
            //     permutations_history[piece_history_index-1],
            //     (double)orientations_counts_at_previous_piece[piece_history_index-1],
            //     permutations_history[piece_history_index-1] / (double)orientations_counts_at_previous_piece[piece_history_index-1],
            //     new_permutations,
            //     (permutations_history[piece_history_index-1] / (double)orientations_counts_at_previous_piece[piece_history_index-1]) - new_permutations
            //     );
            // printf("permutations_tried: %e\n", permutations_tried);
            // usleep(200000);
            permutations_history[piece_history_index] = new_permutations;

            #endif

            // if (backout){
            //     print_colored_pieces_in_space(orientations_history, orientation_history, piece_placing_history, piece_history_index);
            //     #ifdef SPACE_WILL_BE_FULL
            //     print_space(potential_space_fill);
            //     #endif
            //     printf("\n");
            // }
        }
    }

    printf("\nStopped while placing piece %u orientation %u.\n", piece_history_index+1, orientation_placing+1);

    #ifdef TRACK_PROGRESS
    printf("Tried %e permutations of %e (%.4f %%).\n", permutations_tried, total_permutations, permutations_tried / total_permutations * 100.0);
    #endif

    #ifndef STOP_AT_FIRST_SOLUTION
    printf("Found %u solutions.\n", solution_count);
    printf("\nLast solution:\n");
    #endif

    printf("Space:\n\n");
    print_space(space);

    print_colored_pieces_in_space(orientations_history, orientation_history, piece_placing_history, piece_history_index);

    // printf("Orientations:\n\n");
    // for (uint i=0; i<piece_history_index; ++i){
    //     printf("Piece %u:\n", piece_placing_history[i]+1);
    //     print_piece(orientations_history[i][i][orientation_history[i]]);
    //     // print_space_fill(orientations_history[i][i][orientation_history[i]], i+1);
    //     printf("\n");
    // }

    duration = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Done in %.1f seconds.\n", duration);
    // printf("Done!\n");


    return 0;
}
