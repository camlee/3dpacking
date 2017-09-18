#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// #define DEBUG

#ifdef DEBUG
    #include <unistd.h>
    #define SLOW_DOWN() usleep(300000)
#include <unistd.h>
#else
    #define SLOW_DOWN() // Do nothing
#endif

#define SPACE_WIDTH 3
#define SPACE_HEIGHT 3
#define SPACE_DEPTH 3

#ifdef DEBUG
    #define ASSERT_WITHIN_BOUNDS(x, y, z) if ((x) >= SPACE_WIDTH || (y) >= SPACE_HEIGHT || (z) >= SPACE_DEPTH) \
        {printf("\nx, y, or z out of bounds\n\n(File %s, Line %d, in %s().\n\nTerminating!\n\n", __FILE__, __LINE__, __func__); exit(1);}
#else
    #define ASSERT_WITHIN_BOUNDS(x, y, z) // Do nothing
#endif

#define NUM_PIECES 6
#define PIECE_ORIENTATIONS_LIMIT 100

typedef uint_fast32_t geom;
// Use one of the following, as per the needed bits:
// uint_fast8_t
// uint_fast16_t
// uint_fast32_t
// uint_fast64_t

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
    uint value;
    for (uint x=0; x<SPACE_WIDTH; ++x){
        for (uint y=0; y<SPACE_HEIGHT; ++y){
            for (uint z=0; z<SPACE_DEPTH; ++z){
                if (space & l2b(x, y, z)){
                    value = 1;
                } else{
                    value = 0;
                }
                printf("%i ", value);
            }
            printf("\n");
        }
        printf("\n");
    }
};

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

    if (count == 0){
        return piece;
    }

    if (axis == 0){ // x axis
        for (uint x=0; x<SPACE_WIDTH; ++x){
            for (uint y=0; y<SPACE_HEIGHT; ++y){
                for (uint z=0; z<SPACE_DEPTH; ++z){
                    if (piece & l2b(x, y, z)){
                            output |= l2b(x, z, -y + SPACE_DEPTH - 1);
                    }
                }
            }
        }
    } else if (axis == 1){ // y axis
        for (uint x=0; x<SPACE_WIDTH; ++x){
            for (uint y=0; y<SPACE_HEIGHT; ++y){
                for (uint z=0; z<SPACE_DEPTH; ++z){
                    if (piece & l2b(x, y, z)){
                        output |= l2b(z, y, x);
                    }
                }
            }
        }
    } else if (axis == 2){ // z axis
        for (uint x=0; x<SPACE_WIDTH; ++x){
            for (uint y=0; y<SPACE_HEIGHT; ++y){
                for (uint z=0; z<SPACE_DEPTH; ++z){
                    if (piece & l2b(x, y, z)){
                        output |= l2b(y, -x + SPACE_HEIGHT - 1, z);
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
                        // printf("Making piece %u.\n", ++orientation_attempts);
                        new_piece = shift_piece(rotate_piece(piece, axis, rotation), x_shift, y_shift, z_shift);
                        if (piece_in_array(orientations, orientation_count, new_piece)){
                            // printf("Same piece. :(\n");
                            // getchar();
                            continue;
                        } else {
                            // printf("New piece!\n");
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

#define asssertGeomEqual(value1, value2, message) do { if (!((value1) == (value2))){ printf("\nfailed: %u != %u    %s", value1, value2, message); ++failures;}} while (0)
#define assertFalse(value, message) do { if (value){ printf("\nfailed: %u is true    %s", value, message); ++failures;}} while (0)
#define assertTrue(value, message) do { if (!value){ printf("\nfailed: %u is false    %s", value, message); ++failures;}} while (0)

uint test(){
    uint failures = 0;

    asssertGeomEqual(l2b(0, 0, 0), 1, "");
    asssertGeomEqual(l2b(0, 0, 1), 2, "");

    asssertGeomEqual(l2b(0, 0, 0), 0b1, "");
    asssertGeomEqual(l2b(0, 0, 1), 0b10, "");

    asssertGeomEqual(l2b(0, 0, 0), 1 << 0, "");
    asssertGeomEqual(l2b(0, 0, 1), 1 << 1, "");

    asssertGeomEqual(l2b(0, 1, 0), 0b1000, "");
    asssertGeomEqual(l2b(1, 0, 0), 0b1000000000, "");
    asssertGeomEqual(l2b(1, 1, 1), 0b10000000000000, "");

    uint array_len = 3;
    geom array[3] = {0b001, 0b010, 0b011};

    assertFalse(piece_in_array(array, array_len, 0b100), "");
    assertTrue(piece_in_array(array, array_len, 0b010), "");

    asssertGeomEqual(shift_piece(l2b(0, 0, 0), 0, 0, 0), l2b(0, 0, 0), "No shifting.");
    asssertGeomEqual(shift_piece(l2b(0, 0, 0), 1, 0, 0), l2b(1, 0, 0), "Shift by one.");
    asssertGeomEqual(shift_piece(l2b(0, 0, 0), 2, 0, 0), l2b(2, 0, 0), "Shift by two.");
    asssertGeomEqual(shift_piece(l2b(2, 0, 0), -1, 0, 0), l2b(1, 0, 0), "Shift by minus one.");
    asssertGeomEqual(shift_piece(l2b(2, 0, 0), -2, 0, 0), l2b(0, 0, 0), "Shift by minus two.");

    asssertGeomEqual(shift_piece(l2b(0, 0, 0), 0, 1, 0), l2b(0, 1, 0), "Shift by one y.");
    asssertGeomEqual(shift_piece(l2b(0, 0, 0), 0, 0, 1), l2b(0, 0, 1), "Shift by one z.");

    asssertGeomEqual(shift_piece(l2b(0, 0, 0), 1, 1, 1), l2b(1, 1, 1), "Shift by one x, y, and z.");


    asssertGeomEqual(shift_piece(l2b(1, 0, 0) | l2b(2, 0, 1), -1, 2, 1), l2b(0, 2, 1) | l2b(1, 2, 2), "Shift multiple locations.");

    asssertGeomEqual(shift_piece(l2b(1, 0, 0), SPACE_WIDTH, 0, 0), l2b(1, 0, 0),
        "Shift past edge of space in positive direction.");
    asssertGeomEqual(shift_piece(l2b(SPACE_WIDTH-1, 0, 0), 1, 1, 0), l2b(SPACE_WIDTH-1, 0, 0),
        "Shift past edge of space in positive direction (should return same geom).");
    asssertGeomEqual(shift_piece(l2b(0, 0, 0), -1, 0, 0), l2b(0, 0, 0),
        "Shift past edge of space in negative direction.");

    return failures;
}


int main(){

    printf("\nRunning tests...");
    uint failures = test();
    if (failures == 0){
        printf(" passed!\n");
    } else {
        printf("\nThere were %u test failures. Exiting.\n", failures);
        return 1;
    }


// problem4_pieces = (
//     Piece([(0,0,0), (0,1,0), (1,1,0), (2,1,0)]), # L
//     Piece([(0,0,0), (1,0,0), (1,1,0), (2,0,0)]), # T
//     Piece([(0,0,0), (1,0,0), (2,0,0), (1,1,0), (2,0,1)]),
//     Piece([(0,0,0), (1,0,0), (0,1,0), (0,1,1)]),
//     Piece([(0,0,0), (1,0,0), (2,0,0), (1,1,0), (1,1,1)]),
//     Piece([(0,0,0), (0,1,0), (0,1,1), (1,1,0), (1,2,0)]),
//     )
// problem4_space = Space((3,3,3))
// problem4 = Problem(problem4_pieces, problem4_space)

    printf("\nStarting...\n");

    geom piece1 = l2b(0, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0) | l2b(2, 1, 0);
    geom piece2 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(1, 1, 0) | l2b(2, 0, 0);
    geom piece3 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(1, 1, 0) | l2b(2, 0, 1);
    geom piece4 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(0, 1, 0) | l2b(0, 1, 1);
    geom piece5 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(1, 1, 0) | l2b(1, 1, 1);
    geom piece6 = l2b(0, 0, 0) | l2b(0, 1, 0) | l2b(0, 1, 1) | l2b(1, 1, 0) | l2b(1, 2, 0);



    // geom piece1 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0);
    // geom piece2 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0);
    // geom piece3 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0);
    // geom piece4 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0) | l2b(2, 1, 0);
    // geom piece5 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0) | l2b(2, 1, 0);
    // geom piece6 = l2b(0, 0, 0) | l2b(1, 0, 0) | l2b(2, 0, 0) | l2b(0, 1, 0) | l2b(1, 1, 0) | l2b(2, 1, 0);

    geom space = 0;
    geom pieces[NUM_PIECES] = {piece1, piece2, piece3, piece4, piece5, piece6};
    geom orientations[NUM_PIECES][PIECE_ORIENTATIONS_LIMIT] = {{0}};
    uint orientation_counts[NUM_PIECES] = {0};

    for (uint i=0; i<NUM_PIECES; i++){
        orientation_counts[i] = populate_orientations(orientations[i], pieces[i]);
        printf("Found %u unique orientations for piece %u.\n", orientation_counts[i], i+1);
    }




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
        // printf("Piece %u, orientation %u.\n", piece_placing+1, orientation_placing+1);
        SLOW_DOWN();

        if (space & orientations[piece_placing][orientation_placing]){ // Does the piece overlap another piece in the space?
            // There was overlap: can't place this piece using this orientation.
            while (++orientation_placing == orientation_counts[piece_placing]){ // Increment the orientation. But wait, is it over the limit? Keep looking until we find one that's not.
                // No more available orientations to place this piece. We need to backup,
                // takout a piece, and try placing it differently.
                // printf("Can't place piece %u.\n", piece_placing + 1);
                if (piece_placing == 0){
                    printf("\nTried all the permutations: can't place all the pieces. Therefore no solution!\n\nExiting.\n");
                    exit(1);
                }
                --piece_placing; // Trying to place the previous piece again
                orientation_placing = orientation_history[piece_placing]; // Starting back at the orientation we successfully placed.
                space = space_history[piece_placing]; // Resetting the space to what is was before the previous piece was placed
                // Fall out of the loop naturally so that the while clause is evaluated again: brings us to the next orientation
            }
            // ++orientation_placing; // Try the next orientation
            // if (orientation_placing == orientation_counts[piece_placing]){ // If we've already tried all the oridentations
            //     printf("Can't place piece %u.\n", piece_placing + 1);
            //     --piece_placing; // Trying to place the previous piece again
            //     orientation_placing = ++orientation_history[piece_placing]; // Starting at the next orientation after the one that was used last time
            //     space = space_history[piece_placing]; // Resetting the space to what is was before the previous piece was placed
            //     continue; // Go!
            // }
        } else {
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
        }
    }

    printf("Space:\n\n");
    print_space(space);

    printf("Orientations:\n\n");
    for (uint i=0; i<NUM_PIECES; ++i){
        printf("Piece %u:\n", i+1);
        print_piece(orientations[i][orientation_history[i]]);
        printf("\n");
    }

    printf("Done!\n");

    return 0;
}
