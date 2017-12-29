#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>

// #define VERIFY
// #define DEBUG
// #define VERBOSE

#ifdef DEBUG
    #include <unistd.h>
    #define SLOW_DOWN() usleep(300000)
    #define VERIFY
    #define VERBOSE
#include <unistd.h>
#else
    #define SLOW_DOWN() // Do nothing
#endif

#define SPACE_WIDTH 5
#define SPACE_HEIGHT 5
#define SPACE_DEPTH 5
// Note: rotate_piece() only works with cubes right now.



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

#define NUM_PIECES 25
#define PIECE_ORIENTATIONS_LIMIT 1000

typedef unsigned __int128 geom;
// Use one of the following, as per the needed bits:
// uint_fast8_t
// uint_fast16_t
// uint_fast32_t
// uint_fast64_t
// uint128_t

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

void print_space(geom space){
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
                    printf("1 ");
                } else{
                    printf("0 ");
                }
            }
            printf("\n");
        }
        printf("\n");
    }
};

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
        case SIGINT:
            print_status = 1;
            break;
        case SIGUSR1:
            print_status = 1;
            break;
        default:
            keep_running = 0;
    }
}

#define assertGeomEqual(value1, value2, message) do { if (!((value1) == (value2))){ printf("\nfailed: %u != %u    %s", value1, value2, message); ++failures;}} while (0)
#define assertFalse(value, message) do { if (value){ printf("\nfailed: %u is true    %s", value, message); ++failures;}} while (0)
#define assertTrue(value, message) do { if (!value){ printf("\nfailed: %u is false    %s", value, message); ++failures;}} while (0)
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

    }   }

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
    // geom piece1 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(3, 0, 0) | l2b(4, 0, 0);
    // geom piece2 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0);
    // geom piece3 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0);
    // geom piece4 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0);
    // geom piece5 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0);
    // geom piece6 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0);



    // Problem 4:
    // Space: 3 x 3 x 3
    // geom piece1 = l2b(0, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0) | l2b(2, 1, 0);
    // geom piece2 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(1, 1, 0) | l2b(2, 0, 0);
    // geom piece3 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(1, 1, 0) | l2b(2, 0, 1);
    // geom piece4 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(0, 1, 0) | l2b(0, 1, 1);
    // geom piece5 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(1, 1, 0) | l2b(1, 1, 1);
    // geom piece6 = l2b(0, 0, 0) | l2b(0, 1, 0) | l2b(0, 1, 1) | l2b(1, 1, 0) | l2b(1, 2, 0);



    // Problem 5:
    // Space: 3 x 3 x 3
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



    // Real problem:
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


    printf("Pieces defined!\n");

    geom space = 0;
    //geom pieces[NUM_PIECES] = {piece1, piece2, piece3, piece4, piece5, piece6};
    geom pieces[NUM_PIECES] = {piece1, piece2, piece3, piece4, piece5, piece6, piece7, piece8, piece9, piece10, piece11, piece12, piece13, piece14, piece15, piece16, piece17,
        piece18, piece19, piece20, piece21, piece22, piece23, piece24, piece25};

    geom orientations[NUM_PIECES][PIECE_ORIENTATIONS_LIMIT] = {{0}};
    uint orientation_counts[NUM_PIECES] = {0};
    double total_permutations = 1;
    long unsigned int permutations_counter = 0;
    double tried_permutations = 0;


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


    uint piece_placing = 0; // The index of the piece we're currently trying to place.
        // Also equal to the number of pieces placed.
    uint orientation_placing = 0; // The index of the orientation we're trying for the
        // piece we're trying to place.

    uint orientation_history[NUM_PIECES] = {0}; // Once we've placed a piece using an
        // orientation, we'll keep track of that orientation's index here, so we can
        // resume where we left off if necessary.
    geom space_history[NUM_PIECES] = {0};

    while (piece_placing < NUM_PIECES){ // Keep going until all the pieces have been placed.
        #ifdef VERBOSE
//            printf("Piece %u, orientation %u.\n", piece_placing+1, orientation_placing+1);
        #endif
        SLOW_DOWN();

        ++permutations_counter;
        if (permutations_counter % 1000000000 == 0){
            tried_permutations += 1000000000;
            printf("Tried %e permutations (%.4f %%).\n", tried_permutations, tried_permutations / total_permutations * 100.0);
        }
        
        if (space & orientations[piece_placing][orientation_placing]){ // Does the piece overlap another piece in the space?
            // There was overlap: can't place this piece using this orientation.
            while (++orientation_placing == orientation_counts[piece_placing]){ // Increment the orientation. But wait, is it over the limit? Keep looking until we find one that's not.
                // No more available orientations to place this piece. We need to backup,
                // takout a piece, and try placing it differently.
                #ifdef VERBOSE
                    printf("Can't place piece %u.\n", piece_placing + 1);
                #endif
                if (piece_placing == 0){
                    printf("\nTried all the permutations: can't place all the pieces. Therefore no solution!\n\nExiting.\n");
                    exit(1);
                }
                --piece_placing; // Trying to place the previous piece again
                orientation_placing = orientation_history[piece_placing]; // Starting back at the orientation we successfully placed.
                space = space_history[piece_placing]; // Resetting the space to what is was before the previous piece was placed
                // Fall out of the loop naturally so that the while clause is evaluated again: brings us to the next orientation
            }
        } else {
            #ifdef VERBOSE
                printf("Placed piece %u.\n", piece_placing + 1);
            #endif
            // There is no overlap: we can put this piece in this spot.
            // printf("Placed piece %u.\n", piece_placing + 1);
            orientation_history[piece_placing] = orientation_placing; // Keeping track of what orientation was placed
            space_history[piece_placing] = space; // Keeping track of what the space looked like before we place the piece
            space |= orientations[piece_placing][orientation_placing]; // Putting the piece in the space.
            ++piece_placing; // Moving on to the next piece
            orientation_placing = 0; // Starting with the first orientation for the next piece.

            if (piece_placing == NUM_PIECES){ // Have we placed all the pieces?
                // We've placed all the pieces: we're done!
                printf("\nPlaced all the pieces!\n");
                break; // Break!
            }

            if (!keep_running){
                printf("\nInterupt detected. Exiting.\n");
                printf("\nPlaced %u pieces.\n", piece_placing);
                break;
            }
            if (print_status){
                print_status = 0;
                printf("\nPlaced %u pieces.\n", piece_placing);
                print_space(space);
            }
        }
    }

    printf("Space:\n\n");
    print_space(space);

    printf("Orientations:\n\n");
    for (uint i=0; i<piece_placing; ++i){
        printf("Piece %u:\n", i+1);
        print_piece(orientations[i][orientation_history[i]]);
        printf("\n");
    }

    printf("Done!\n");

    return 0;
}
