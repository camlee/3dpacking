from itertools import permutations, product
import time

try:
    input = raw_input
except Exception:
    pass



class DoesNotFitError(Exception):
    pass

class Piece:
    def __init__(self, geometry, id, symetric_axis=[]):
        self.geometry = [[part[0], part[1], part[2]] for part in geometry]
        self.id = id
        self.symetric_axis = symetric_axis

    def __unicode__(self):
        return "%s" % (self.geometry,)

    def __str__(self):
        return "%s" % (self.geometry,)

    def parts_index_iterator(self):
        return range(len(self.geometry))

    def parts(self):
        return self.geometry

    def shift_to_part(self, i):
        """Shift's the definition of the part so that piece i is at the origin."""
        new_piece = Piece(self.geometry, self.id)
        part_to_shift_to = new_piece.geometry[i]

        for j, part in enumerate(new_piece.geometry):
            new_piece.geometry[j] = (part[0] - part_to_shift_to[0], part[1] - part_to_shift_to[1], part[2] - part_to_shift_to[2])

        return new_piece

    def rotate(self, axis, count):

        new_piece = Piece(self.geometry, self.id, symetric_axis=self.symetric_axis)

        if axis in self.symetric_axis:
            return new_piece

        if count == 0:
            return new_piece
        else:
            for j, part in enumerate(new_piece.geometry):
                if axis == 'x':
                    new_piece.geometry[j] = (part[0], part[2], -part[1])
                elif axis == 'y':
                    new_piece.geometry[j] = (-part[2], part[1], part[0])
                elif axis == 'z':
                    new_piece.geometry[j] = (part[1], -part[0], part[2])

            return new_piece.rotate(axis, count-1)


class Space:
    def __init__(self, size):
        self.length_x = size[0]
        self.length_y = size[1]
        self.length_z = size[2]

        self.clear()

    def clear(self):
        self.index_x = 0
        self.index_y = 0
        self.index_z = 0

        self.current_geometry = [[[0 for _ in range(self.length_z)] for _ in range(self.length_y)] for _ in range(self.length_x)]
        #self.current_geometry = (((0,)*self.length_z,)*self.length_y,)*self.length_x

    def is_valid(self):
        return max(max(max(self.current_geometry))) <= 1

    def points_iterator(self):
        return product(range(self.length_x), range(self.length_y), range(self.length_z))

    def orientations_iterator(self):
        return product(("x", "y", "z"), (0, 1, 2, 3))

    def place(self, piece, i, x, y, z, axis, axis_rotations):
        self._add_or_remove(piece, i, x, y, z, axis, axis_rotations, "add")

    def remove(self, piece, i, x, y, z, axis, axis_rotations):
        self._add_or_remove(piece, i, x, y, z, axis, axis_rotations, "subtract")

    def _add_or_remove(self, piece, i, x, y, z, axis, axis_rotations, operation):

        diff = piece.id if operation == "add" else -piece.id

        temp_geometry = [[[element for element in row] for row in plane] for plane in self.current_geometry]

        # if x == 0 and y == 0:
        #     pass
        # else:            
        #     import pdb
        #     pdb.set_trace()

        for part in piece.rotate(axis, axis_rotations).shift_to_part(i).parts():
            try:
                if (x+part[0]) < 0 or (y+part[1]) < 0 or (z+part[2]) < 0:
                    raise DoesNotFitError
                if temp_geometry[x+part[0]][y+part[1]][z+part[2]] != 0 and operation == "add":
                    raise DoesNotFitError()
            except IndexError:
                raise DoesNotFitError()

            temp_geometry[x+part[0]][y+part[1]][z+part[2]] += diff

        # if piece.id == 6:
        #     import pdb
        #     pdb.set_trace()

        # print("Successful placed piece %s" % (piece.id))
        # import pdb
        # pdb.set_trace()
        self.current_geometry = temp_geometry
 

    def display(self):
        return "%s" % (self.current_geometry,)

# Define components of the puzzle here

#piece2 = Piece([(0,0,0),(0,0,1),(0,0,2),(0,1,1),(0,2,1)]) # T

#Problem 1:

# piece1 = Piece([(0,0,0),(0,0,1),(0,0,2),(0,0,3),(0,0,4)], 1) # Straight
# piece2 = Piece([(0,0,0),(1,0,0),(2,0,0),(0,1,0),(1,1,0)], 2)
# piece3 = Piece([(0,0,0),(1,0,0),(2,0,0),(0,1,0),(1,1,0)], 3)
# piece4 = Piece([(0,0,0),(1,0,0),(2,0,0),(0,1,0),(1,1,0)], 4)
# piece5 = Piece([(0,0,0),(1,0,0),(2,0,0),(0,1,0),(1,1,0)], 5)
# piece6 = Piece([(0,0,0),(1,0,0),(2,0,0),(0,1,0),(1,1,0)], 6)

# pieces = (piece1, piece2, piece3, piece4, piece5, piece6)

# space = Space((5,3,2))

#Problem 2:

piece1 = Piece(((0,0,0),(1,0,0),(2,0,0),(2,1,0),(2,2,0)), 1)
piece2 = Piece(((0,0,0),(1,0,0),(0,1,0),(1,1,0)), 2)
piece3 = Piece(((0,0,0),(1,0,0),(2,0,0),(0,1,0),(1,1,0)), 3)
piece4 = Piece(((0,0,0),(1,0,0),(2,0,0)), 4, symetric_axis = ["x"])
piece5 = Piece(((0,0,0),(0,0,1),(0,1,1),(0,0,2)), 5)
# piece6 = Piece(((0,0,0),(1,0,0),(2,0,0),(2,1,0),(2,-1,0),(2,0,1)), 6)
piece6 = Piece(((0,0,0),(1,0,0),(2,0,0),(2,1,0),(2,-1,0)), 6)

pieces = (piece1, piece2, piece3, piece4, piece5, piece6)

space = Space((3,3,3))

#Problem 3:

# piece1 = Piece([(0,0,0), (1,0,0)], 1)
# piece2 = Piece([(0,0,0), (1,0,0), (1,1,0)], 2)
# piece3 = Piece([(0,0,0)], 3)
# piece4 = Piece([(0,0,0), (0,1,0)], 4)

# pieces = (piece1, piece2, piece3, piece4)

# space = Space((2,2,2))

# End of definition of puzzle pieces


def place_piece_in_all_spots(piece):    
    for (x, y, z) in space.points_iterator():
        for i in piece.parts_index():
            for orientation in (0, 1, 2, 3, 4, 5, 6):
                space.place(piece, i, x, y, z, orientation)

def place_all_pieces_in_all_spots_and_check_if_solution(pieces, solutions_history, parent=True):
    # print("Pieces left: %s" % len(pieces))

    if len(pieces) == 0:
        if space.current_geometry in solutions_history:
            return
        else:
            solutions_history += [space.current_geometry]
            print("Found solution %s" % (len(solutions_history)))
            # print("Space: %s" % (space.display()))

            out_file.write("Solution %s:\n" % (len(solutions_history)))
            out_file.write(space.display())
            out_file.write("\n\n")

            # _ = input("Continue? ")
        return
    else:
        pass

    piece = pieces[0]
    # print("Placing piece %s" % piece.id)

    for (x, y, z) in space.points_iterator():
        if parent:
            print("Currently placing first piece at: (%s, %s, %s)" % (x, y, z))
        # print("x, y, z: %s, %s, %s" % (x,y,z))
        if space.current_geometry[x][y][z] > 0: # Shortcut
            continue

        # print("Position: (%s, %s, %s)" % (x, y, z))
        for i in piece.parts_index_iterator():
            if parent:
                print("and placing piece %s" % (i))
            # print("Part: %s" % i)
            for (axis, axis_rotations) in space.orientations_iterator():
                if parent:
                    print("with rotation: (%s, %s)" % (axis, axis_rotations))
                # print("Position: (%s, %s, %s)" % (x, y, z))
                # print("Part: %s" % i)
                # print("Rotation: (%s, %s)" % (axis, axis_rotations))
                try:
                    # print("Placing piece %s" % (piece.id))
                    space.place(piece, i, x, y, z, axis, axis_rotations)
                    # print("Space: %s" % (space.display()))
                except DoesNotFitError as e:
                    # print("Piece doesn't fit %s" % (piece.id))
                    continue
                    # print("A piece didn' fit.")
                else:
                    # print("Successfully placed piece %s" % (piece.id))
                    place_all_pieces_in_all_spots_and_check_if_solution(pieces[1:], solutions_history, parent=False)
                    # print("Removing piece %s" % (piece.id))
                    space.remove(piece, i, x, y, z, axis, axis_rotations)


    # print("Done trying all placements of piece %s" % piece.id)



if __name__ == '__main__':

    solutions_history = []

    start_time = time.time()

    with open("results.txt", "wr") as out_file:
        place_all_pieces_in_all_spots_and_check_if_solution(pieces, solutions_history)

    end_time = time.time()

    print("\n\nDone. Found %s solutions. Took %s minutes." % (len(solutions_history), (end_time - start_time)/60))