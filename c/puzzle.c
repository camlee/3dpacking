#include <stdio.h>
#include <stdbool.h>

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

#define SPACE_WIDTH 3
#define SPACE_HEIGHT 3
#define SPACE_DEPTH 3

typedef struct {
    int width;
    int depth;
    int height;
} Space;

typedef struct {
    int x;
    int y;
    int z;
} Part;

typedef struct piece{
    int length;
    Part *parts;
} Piece;

void print_piece(const Piece *piece){
    for (unsigned short int i=0; i<piece->length; ++i){
        printf("(%i, %i, %i)\n", piece->parts[i].x, piece->parts[i].y, piece->parts[i].z);
    }
    printf("\n");
};

void print_space(int space[SPACE_WIDTH][SPACE_DEPTH][SPACE_HEIGHT]){
    for (unsigned short int x=0; x<SPACE_WIDTH; x++){
        for (unsigned short int y=0; y<SPACE_DEPTH; y++){
            for (unsigned short int z=0; z<SPACE_HEIGHT; z++){
                printf("%i ", space[x][y][z]);
            }
            printf("\n");
        }
        printf("\n");
    }
};

bool place_piece(int space[SPACE_WIDTH][SPACE_DEPTH][SPACE_HEIGHT], const Piece *piece){
    /*
    Will try to fit the provided piece in the provided space, updating the space accordingly.
    Will return true if the piece was placed or false if it was not. space will only be
    modified if the piece was placed.
    */
    Part part;
    int *space_location;
    bool failed_to_place_piece = false;

    // Trying to place each part of the piece:
    for (unsigned short int i=0; i<piece->length; ++i){
        part = piece->parts[i];

        // First, making sure it doesn't exceed the boundaries of the space:
        if (part.x > SPACE_WIDTH || part.y > SPACE_DEPTH || part.z > SPACE_HEIGHT){
            printf("Part exceeds space");
            failed_to_place_piece = true;
            break;
        } else {

            // Next, making sure the space location doesn't already have a piece in it.
            space_location = &space[part.x][part.y][part.z];
            if (*space_location == 1){
                printf("Part overlaps other piece.");
                failed_to_place_piece = true;
                break;
            } else {
                *space_location = 1;
            }
        }
    }

    if (failed_to_place_piece == true){
        printf("Failed to place piece.");
    }

    return !failed_to_place_piece;
}



int space[SPACE_WIDTH][SPACE_DEPTH][SPACE_HEIGHT] = {{{0}}};

int main(){
    // const Space space = {
    //     .width = 3,
    //     .depth = 3,
    //     .height = 3
    // };

    // const Piece piece1 = {
    //     .length = 4,
    //     .parts = (Part[]){{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}}
    // };

    const Piece piece1 = {
        .length = 3,
        .parts = (Part[]){{0, 0, 0}, {0, 1, 0}, {0, 2, 0}}
    };


    printf("\nStarting...\n");

    print_piece(&piece1);
    printf("Original space:\n\n");
    print_space(space);
    place_piece(space, &piece1);
    printf("Space after placing first piece:\n\n");
    print_space(space);

    printf("Done!\n");
}
