"""
Solves a 3-dimensional packing problem using brute force.

Pieces must be composed of any number of unit-cubes (parts). They don't
have to be connected, but in a typical physical puzzle, they would be.

The space that the pieces are placed in is a rectangular cuboid (aka
a box) of any size. ex. 5x5x5 or 3x3x10.

The problem (pieces and space to place them in) can be solved with
the answer(s) written to a text file. Blender (blender.org) can also
be used to visualize the solution or intermediate results.
"""

from itertools import permutations, product
import numpy
import time
import os
import sys
import random
import inspect


class DoesNotFitError(Exception):
    pass

class StopNow(Exception):
    pass

class Piece:
    def __init__(self, geometry, id=None, color=None, is_copy=False):

        self.geometry = set([(part[0], part[1], part[2]) for part in geometry])
        self.id = id
        self.color = color or (random.random()*255, random.random()*255, random.random()*255)
        self.object_name = None

    def __unicode__(self):
        return u"Piece: %s" % (list(self.geometry),)

    def __str__(self):
        return "Piece: %s" % (list(self.geometry),)

    def __repr__(self):
        return "Piece(%s, %s)" % (list(self.geometry), self.id)

    def __eq__(self, other):
        return isinstance(other, self.__class__) and self.geometry == other.geometry

    def __ne__(self, other):
        return not self.__eq__(other)

    def copy(self):
        return Piece(self.geometry, self.id, color=self.color, is_copy=True)

    def clear_geometry(self):
        self.geometry = set([])
        return self

    def analyze(self):
        self.set_up_orientations()
        return self

    def set_up_orientations(self):
        """
        Pre-calculating all orientations of the piece (rotations and shifts)
        for faster access later.
        """

        self.rotations = []
        self.orientations = []

        for axis in ("x", "y", "z"):
            for unique_rotation in (0, 1, 2, 3):
                if unique_rotation == 0:
                    rotated_piece = self
                else:
                    rotated_piece = self.rotate(axis, unique_rotation).normalize()

                if rotated_piece in self.rotations:
                    continue
                else:
                    self.rotations += [rotated_piece]
                    for part in self.geometry:
                        oriented_piece = rotated_piece.shift_to_part(part)
                        self.orientations += [oriented_piece]

        # print("Piece %s has: %s unique rotations, %s unique orientations." % (self.id, len(self.rotations), len(self.orientations)))
        return self

    def parts(self):
        return self.geometry

    def normalize(self):
        """
        Shift all parts of the piece so that it's just barely in the positive quadrant.
        """
        copy = self.copy().clear_geometry()

        min_values = [None, None, None]
        for axis in (0, 1, 2):
            min_values[axis] = min([part[axis] for part in self.geometry])

        for part in self.geometry:
            copy.geometry |= set([(part[0]-min_values[0], part[1]-min_values[1], part[2]-min_values[2])])
        return copy

    def shift_to_part(self, part_to_shift_to):
        """
        Shift's the definition of the piece so that the specified part is at the origin.
        """
        copy = self.copy().clear_geometry()

        for part in self.geometry:
            copy.geometry |= set([(part[0] - part_to_shift_to[0], part[1] - part_to_shift_to[1], part[2] - part_to_shift_to[2])])

        return copy

    def rotate(self, axis, count=1):
        """
        Rotates the part count times around the specified axis.
        """
        copy = self.copy()

        if count == 0:
            return copy
        else:
            copy.clear_geometry()
            for part in self.geometry:
                if axis == 'x':
                    copy.geometry |= set([(part[0], part[2], -part[1])])
                elif axis == 'y':
                    copy.geometry |= set([(-part[2], part[1], part[0])])
                elif axis == 'z':
                    copy.geometry |= set([(part[1], -part[0], part[2])])

            return copy.rotate(axis, count-1)

    def to_blender(self, location=None, size=1):
        """
        Returns verts and faces (in a tuple) for use with Blender's
        Mesh.from_pydata() function for creating meshes.

        The size argument determines how big the resulting object will be.
        """
        verts = []
        faces = []
        index = 0
        for i, part in enumerate(self.geometry):
            # Creating eight corners of the part
            for bottom_or_top in [0, 1]:
                # 0 is bottom. 1 is top
                verts.append((part[0]*size,        part[1]*size,        part[2]*size + bottom_or_top*size))
                verts.append((part[0]*size + size, part[1]*size,        part[2]*size + bottom_or_top*size))
                verts.append((part[0]*size + size, part[1]*size + size, part[2]*size + bottom_or_top*size))
                verts.append((part[0]*size,        part[1]*size + size, part[2]*size + bottom_or_top*size))

            # Creating 6 sides of the part
            po = 8*i # part offset
            faces.append([po, po+1, po+2, po+3]) # Bottom
            faces.append([po+4, po+5, po+6, po+7]) # Top
            faces.append([po, po+1, po+5, po+4])
            faces.append([po, po+3, po+7, po+4])
            faces.append([po+1, po+2, po+6, po+5])
            faces.append([po+3, po+2, po+6, po+7])

        if location:
            location = [component * size for component in location]
        else:
            location = (0, 0, 0)

        return (verts, faces, location)

class Space:
    """
    Defines a 3-dimensional volume which pieces can be but in.
    Is a rectangular box of any size.
    """
    def __init__(self, size):
        self.length_x = size[0]
        self.length_y = size[1]
        self.length_z = size[2]

        self.placed_pieces = {}

        self.clear()

    def clear(self):
        self.index_x = 0
        self.index_y = 0
        self.index_z = 0

        self.current_geometry = [[[0 for _ in range(self.length_z)] for _ in range(self.length_y)] for _ in range(self.length_x)]
            # Each 0 defines an empty point in the space. A non-zero point indicates a piece has been placed there.

    def points_iterator(self):
        """
        Returns an iterable over all the points of the space.
        A point in the space is a place a part of a piece can be placed.
        """
        return product(range(self.length_x), range(self.length_y), range(self.length_z))

    def place(self, piece, x, y, z):
        """
        Put a piece in the space at position x, y, z.
        Raises a DoesNotFitError if the piece extends past the boundary
        of the space or intersects with any other already placed pieces.
        """
        temp_geometry = [[[element for element in row] for row in plane] for plane in self.current_geometry]

        for part in piece.parts():
            try:
                if (x+part[0]) < 0 or (y+part[1]) < 0 or (z+part[2]) < 0:
                    raise DoesNotFitError
                if temp_geometry[x+part[0]][y+part[1]][z+part[2]] != 0:
                    raise DoesNotFitError()
            except IndexError:
                raise DoesNotFitError()

            temp_geometry[x+part[0]][y+part[1]][z+part[2]] += piece.id

        self.placed_pieces[piece.id] = {"piece": piece, "location": (x, y, z)}

        self.current_geometry = temp_geometry

    def remove(self, piece):
        self.current_geometry = [[[element if element != piece.id else 0 for element in row] for row in plane] for plane in self.current_geometry]
        del self.placed_pieces[piece.id]

    def empty_regions(self):
        """
        Returns a list of the sizes of the empty regions.
        Useful for checking if the space can still be filled by pieces
        of a certain size.
        """
        geometry = numpy.array(self.current_geometry)
        regions = []
        points_in_regions = []

        points = list(self.points_iterator())

        for point in points:

            if geometry[point] == 0 and point not in points_in_regions:
                regions.append(1)
                points_in_regions.append(point)
                points_to_check = [point]
                points_checked = []

                # Now, finding the rest of the points:
                while len(points_to_check) > 0:
                    point_being_checked = points_to_check.pop()
                    points_checked.append(point_being_checked)

                    for delta_x, delta_y, delta_z in [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)]:
                        point_adjacent = (point_being_checked[0] + delta_x, point_being_checked[1] + delta_y, point_being_checked[2] + delta_z)
                        if point_adjacent not in points: # Don't need to check points outside of the space
                            continue
                        if point_adjacent in points_checked: # Don't need to check points that have already been checked
                            continue
                        if geometry[point_adjacent] == 0 and point_adjacent not in points_in_regions:
                            regions[-1] += 1
                            points_in_regions.append(point_adjacent)
                            points_to_check.append(point_adjacent)

        return regions


    def display(self):
        return "%s" % (self.current_geometry,)

    def to_blender(self):
        raise(NotImplementedError())


class Problem:
    """
    Defines the space and the pieces to put in that space. Also provides
    an algorithm for putting the peices in the space.
    """
    def __init__(self, pieces, space):
        self.pieces = list(pieces)
        self.space = space

        # Giving each piece a unique number for use when solving
        # and preparing pieces for solving by analyzing them
        for i, piece in enumerate(self.pieces):
            piece.id = i + 1
            piece.analyze()

        self.solutions_history = []

    def solve(self, out_file_name="results.txt", **kwargs):

        start_time = time.time()

        try:
            os.remove(out_file_name)
        except OSError:
            pass
        place_all_pieces_in_all_spots_and_check_if_solution(self.space, self.pieces, self.solutions_history, out_file_name, start_time, float("inf"), **kwargs)

        end_time = time.time()

        print("\n\nDone. Found %s solutions. Took %s minutes." % (len(self.solutions_history), (end_time - start_time)/60))



def place_all_pieces_in_all_spots_and_check_if_solution(space, pieces, solutions_history, out_file_name, start_time,
        minimum_pieces, parent=True, stop_at=None, stop_after=None, timeout=None, placed_cb=None, placing_cb=None, failed_place_cb=None):
    """
    Brute force algorithm for placing all pieces in the space. Calls
    itself after successfully placing a piece to place all of the other
    pieces.
    """

    if len(pieces) < minimum_pieces:
        minimum_pieces = len(pieces)
        print("Down to %s pieces now." % minimum_pieces)
        if stop_at is not None and minimum_pieces <= stop_at:
            raise StopNow("Stopping solution: only %s pieces left to place." % stop_at)
        if stop_after is not None and len(space.placed_pieces) >= stop_after:
            raise StopNow("Stopping solution: placed %s pieces." % stop_after)

        end_time = time.time()
        time_delta = (end_time - start_time)/60
        with open(out_file_name, "a") as out_file:
            out_file.write("Partial Solution (%s pieces left) (at %.1f minutes in):\n" % (len(pieces), time_delta))
            out_file.write(space.display())
            out_file.write("\n\n")

    if len(pieces) == 0:
        if space.current_geometry in solutions_history:
            return 0
        else:
            solutions_history += [space.current_geometry]
            end_time = time.time()
            time_delta = (end_time - start_time)/60
            print("Found solution %s. %s minutes in." % (len(solutions_history), time_delta))
            # print("Space: %s" % (space.display()))

            with open(out_file_name, "a") as out_file:
                out_file.write("Solution %s (at %.1f minutes in):\n" % (len(solutions_history), time_delta))
                out_file.write(space.display())
                out_file.write("\n\n")

        return 0
    else:
        pass

    piece = pieces[0]
    print("Placing piece %s" % piece.id)

    for (x, y, z) in space.points_iterator():
        # print("x, y, z: %s, %s, %s" % (x,y,z))
        if timeout and time.time() - start_time > timeout:
            raise StopNow("Stopping solution: timed out after %.1f s." % (time.time() - start_time,))

        if space.current_geometry[x][y][z] > 0: # Shortcut
            continue

        for rotated_piece in piece.orientations:

            try:
                # print("Placing piece %s" % (piece.id))
                if placing_cb:
                    placing_cb(rotated_piece, (x, y, z))

                space.place(rotated_piece, x, y, z)

            except DoesNotFitError as e:
                # print("Piece doesn't fit %s" % (piece.id))
                if failed_place_cb:
                    failed_place_cb(rotated_piece, (x, y, z))

                continue
            else:
                regions = space.empty_regions()
                if len(regions) == 1 or sum([region % 5 for region in regions]) == 0:
                    # Shortcut: we know the puzzle isn't solvable if the unconnected empty
                    # spaces aren't of the correct size. So only proceeding if this is not the case.

                    if placed_cb:
                        placed_cb(rotated_piece, (x, y, z))

                    minimum_pieces = place_all_pieces_in_all_spots_and_check_if_solution(space, pieces[1:], solutions_history,
                            out_file_name, start_time, minimum_pieces, parent=False, stop_at=stop_at, stop_after=stop_after, timeout=timeout,
                            placed_cb=placed_cb, placing_cb=placing_cb, failed_place_cb=failed_place_cb)

                else:
                    pass
                    # print("Space split into unsolvable regions of sizes: %s. Backing out last placement." % (regions,))

                # print("Removing piece %s" % (piece.id))
                space.remove(rotated_piece)

                if failed_place_cb:
                    failed_place_cb(rotated_piece, (x, y, z))


        # print("Unable to place a piece after trying all posibilities. Must remove a piece and try again.")
    return minimum_pieces

    # print("Done trying all placements of piece %s" % piece.id)


class CommandClass:
    def __new__(cls, *args, **kwargs):
        instance = super().__new__(cls, *args, **kwargs)

        cls.commands = []
        for name in dir(cls):
            method = getattr(cls, name)
            if hasattr(method, "is_command") and method.is_command:
                cls.commands.append(name)

        return instance

def command(method):
    method.is_command = True
    return method


class BlenderApi(CommandClass):
    """
    Provides an API for running puzzle stuff from within blender
    and viewing the results. Requires bpy module, so this class
    can only be instantiated from within a blender console.
    """
    def __init__(self):
        self.object_id = 0

        self.problem = real_problem
        self.problem_name = "real_problem"

        import bpy
        self.bpy = bpy

    def run(self, command=None, *args, **kwargs):
        if command is None:
            print("Welcome to bpi (Blender Puzzle Interface). Here are the available commands:\n")
            self.help()
            print("For now, running draw_all_pieces:")
            self.draw_all_pieces()

        else:
            try:
                method = getattr(self, command)
            except AttributeError:
                print("No command found by the name of %s" % command)
                self.help()
            else:
                method(*args, **kwargs)

    @command
    def help(self):
        """
        print this help text and exit.
        """
        for command in sorted(self.commands):
            method = getattr(self, command)
            arg_spec = inspect.getfullargspec(method)
            arg_strings = ["'%s'" % command]
            if arg_spec.defaults:
                for i, default in enumerate(arg_spec.defaults):
                    key = arg_spec.args[i - len(arg_spec.defaults)]
                    arg_strings.append("%s=%s" % (key, default))

            args = ", ".join(arg_strings)
            print("bpi.run(%s)" % args)
            print("    %s" % method.__doc__.strip())
            print("")

    def refresh_preview(self):
        self.bpy.ops.wm.redraw_timer(type='DRAW_WIN_SWAP', iterations=1)

    def _layout_pieces(self, pieces, num_rows=5, spacing=5):
        bpy = self.bpy

        for i, piece in enumerate(pieces):
            x_offset = -10 + spacing * (i / num_rows)
            y_offset = -10 + spacing * (i % num_rows)
            self.draw_piece(piece, (x_offset, y_offset, 0))

    @command
    def draw_all_pieces(self):
        """
        draw all of the pieces for the current puzzle, laid out in a grid
        """
        print("Drawing all pieces...")
        self._layout_pieces(self.problem.pieces)

    @command
    def draw_all_orientations(self, num_rows=8, piece_index=0):
        """
        draw all orientations for a given piece. Optional arguments:
            num_rows: the number of pieces to place in a single line before wrapping
            piece_index: the index of the piece to draw the orientations for
        """
        print("Drawing all orientations...")
        self._layout_pieces(self.problem.pieces[piece_index].orientations)

    @command
    def draw_all_rotations(self, num_rows=8, piece_index=0):
        """
        draw all rotations for a given piece. Optional arguments:
            num_rows: the number of pieces to place in a single line before wrapping
            piece_index: the index of the piece to draw the orientations for
        """
        print("Drawing all orientations...")
        self._layout_pieces(self.problem.pieces[piece_index].rotations)

    @command
    def solve(self, stop_at=None, stop_after=None, timeout=10, redraw=True):
        """
        solve the problem and draw the solution or partial solution. Optional arguments:
            stop_at: Stop solving when only this many pieces are left to place
            stop_after: Stop solving when this many pieces have been placed
            timeout: Number of seconds to run for before stopping, no matter how many pieces have been placed.
            redraw: Whether to refresh the render while solving or not (i.e. only at the end)
                    The API that does this is described by Blender as unsupported so it needs to be set to False
                    in some cases. ex:
                    * To see the output of print statements
        """
        print("Solving and drawing...")
        bpy = self.bpy

        self.problem.pieces = reorder_pieces(self.problem.pieces)

        def placing_cb(piece, location):
            self.draw_piece(piece, location)
            if redraw:
                self.refresh_preview()
            pass

        def placed_cb(piece, location):
            self.undraw_piece(piece)
            self.draw_piece(piece, location)
            if redraw:
                self.refresh_preview()

        def failed_place_cb(piece, location):
            self.undraw_piece(piece)

        try:
            self.problem.solve(stop_at=stop_at, stop_after=stop_after, placing_cb=placing_cb, placed_cb=placed_cb, failed_place_cb=failed_place_cb, timeout=timeout)
        except StopNow as e:
            print(e)
            return

    def draw_piece(self, piece, location=None):
        bpy = self.bpy

        object_name = "Obj %s" % self.object_id

        mesh = bpy.data.meshes.new(object_name)
        obj = bpy.data.objects.new(object_name, mesh)
        mat = bpy.data.materials.new(object_name)

        piece.object_name = object_name
        self.object_id += 1

        # mat.specular_color = piece.color
        # mat.specular_intensity = 1
        mat.diffuse_color = [component / 255 for component in piece.color]
        # mat.diffuse_intensity = 0.1
        # mat.specular_shader = 'COOKTORR'
        # mat.ambient = 1
        # mat.emit=1
        obj.data.materials.append(mat)

        verts, faces, location = piece.to_blender(location)

        obj.location = location
        bpy.context.scene.objects.link(obj)

        mesh.from_pydata(verts, [], faces)
        mesh.update()

    def undraw_piece(self, piece):
        bpy = self.bpy

        for obj in bpy.data.objects:
            obj.select = False

        bpy.data.objects[piece.object_name].select = True
        bpy.ops.object.delete()
        piece.object_name = None

    def cleanup(self):
        print("Cleaning up")
        bpy = self.bpy

        objects_being_deleted = []

        for obj in bpy.data.objects:
            if obj.type in ["LAMP", "CAMERA"]:
                obj.select = False
            else:
                obj.select = True
                objects_being_deleted.append(obj)

        if len(objects_being_deleted) > 0:
            # print("\n".join([obj.name for obj in objects_being_deleted]))

            bpy.ops.object.delete()


def reorder_pieces(pieces):
    pieces = list(pieces)
    output = []
    while len(pieces) > 0:
        picking_at = (len(pieces) - 1) % int(random.random()*100 + 1)
        output.append(pieces.pop(picking_at))
    return output


real_pieces = (
    Piece(((0,0,0),(1,0,0),(2,0,0),(2,1,0),(3,1,0)), color=[238, 238, 0]), # Yellow
    Piece(((0,0,0),(1,0,0),(0,1,0),(0,2,0),(1,2,0)), color=[245, 238, 0]), # Yellow "U"
    Piece(((0,0,0),(1,0,0),(2,0,0),(0,1,0),(0,2,0)), color=[255, 165, 0]), # Light Orange "Symetric L"
    Piece(((0,0,0),(0,0,1),(0,0,2),(0,0,3),(0,0,4)), color=[255, 180, 0]), # Light Orange "Chocolate Bar"
    Piece(((0,0,0),(1,0,0),(1,1,0),(1,0,1),(2,0,1)), color=[238, 154, 0]), # Dark Orange "Y-ish"
    Piece(((0,0,0),(1,0,0),(2,0,0),(2,1,0),(2,1,1)), color=[238, 145, 0]), # Dark Orange "L with hook off short end"
    Piece(((0,0,0),(0,0,1),(1,0,0),(2,0,0),(2,1,0)), color=[238, 154, 0]), # Dark Orange "L with hook off long end"
    Piece(((0,0,0),(1,0,0),(2,0,0),(1,1,0),(1,2,0)), color=[255, 0, 0]), # Red "T"
    Piece(((0,0,0),(1,0,0),(1,1,0),(2,1,0),(2,2,0)), color=[255, 0, 20]), # Red "W"
    Piece(((0,0,0),(1,0,0),(2,0,0),(2,1,0),(2,0,1)), color=[200, 0, 0]), # Dark Red "L" with hook off corner"
    Piece(((0,0,0),(0,1,0),(1,0,0),(2,0,0),(2,0,1)), color=[200, 20, 0]), # Dark Red "L with hook off long end"
    Piece(((0,0,0),(1,0,0),(2,0,0),(3,0,0),(3,1,0)), color=[142, 56, 142]), # Purple "L"
    Piece(((0,1,0),(1,1,0),(2,1,0),(1,0,0),(1,2,0)), color=[142, 40, 142]), # Purple "Cross"
    Piece(((0,0,0),(0,0,1),(1,0,0),(1,1,0),(1,1,1)), color=[0, 0, 205]), # Blue "Two towers"
    Piece(((0,0,0),(1,0,0),(2,0,0),(1,1,0),(2,0,1)), color=[0, 20, 205]), # Blue "L with hook off middle of long end"
    Piece(((0,0,0),(1,0,0),(1,1,0),(2,0,0),(2,1,0)), color=[0, 128, 128]), # Teal "Foam finger"
    Piece(((0,0,0),(0,1,0),(1,1,0),(2,1,0),(2,2,0)), color=[20, 128, 128]), # Teal "Z"
    Piece(((0,0,0),(1,0,0),(1,0,1),(2,0,1),(2,1,1)), color=[173, 255, 47]), # Yellow-Green "Left-handed"
    Piece(((0,0,0),(0,0,1),(1,0,0),(1,1,0),(2,1,0)), color=[173, 234, 47]), # Yellow-Green "Right-handed"
    Piece(((0,0,0),(1,0,0),(1,1,0),(1,0,1),(2,0,0)), color=[154, 255, 154]), # Light Green "Bent Cross"
    Piece(((0,0,0),(1,0,0),(2,0,0),(1,1,0),(2,0,1)), color=[170, 255, 154]), # Light Green "L with hook off side of long end"
    Piece(((0,0,0),(1,0,0),(2,0,0),(3,0,0),(2,1,0)), color=[162, 205, 90]), # Olive Green "Rifle"
    Piece(((0,0,0),(1,0,0),(1,1,0),(1,2,0),(2,1,0)), color=[150, 205, 90]), # Olive Green "Y-ish"
    Piece(((0,0,0),(1,0,0),(0,1,0),(1,1,0),(1,1,1)), color=[0, 100, 0]), # Dark Green "Base and tower"
    Piece(((0,0,0),(1,0,0),(1,0,1),(1,1,0),(2,1,0)), color=[20, 100, 0]), # Dark Green "Y-ish"
    )
real_space = Space((5,5,5))
real_problem = Problem(real_pieces, real_space)


problem1_pieces = (
    Piece([(0,0,0),(0,0,1),(0,0,2),(0,0,3),(0,0,4)]),
    Piece([(0,0,0),(1,0,0),(2,0,0),(0,1,0),(1,1,0)]),
    Piece([(0,0,0),(1,0,0),(2,0,0),(0,1,0),(1,1,0)]),
    Piece([(0,0,0),(1,0,0),(2,0,0),(0,1,0),(1,1,0)]),
    Piece([(0,0,0),(1,0,0),(2,0,0),(0,1,0),(1,1,0)]),
    Piece([(0,0,0),(1,0,0),(2,0,0),(0,1,0),(1,1,0)]),
    )
problem1_space = Space((5,3,2))
problem1 = Problem(problem1_pieces, problem1_space)


problem2_pieces = (
    Piece(((0,0,0),(1,0,0),(2,0,0),(2,1,0),(2,2,0)), 1),
    Piece(((0,0,0),(1,0,0),(0,1,0),(1,1,0)), 2),
    Piece(((0,0,0),(1,0,0),(2,0,0),(0,1,0),(1,1,0)), 3),
    Piece(((0,0,0),(1,0,0),(2,0,0)), 4),
    Piece(((0,0,0),(0,0,1),(0,1,1),(0,0,2)), 5),
    # Piece(((0,0,0),(1,0,0),(2,0,0),(2,1,0),(2,-1,0),(2,0,1)), 6),
    Piece(((0,0,0),(1,0,0),(2,0,0),(2,1,0),(2,-1,0)), 6),
    )
problem2_space = Space((3,3,3))
problem2 = Problem(problem2_pieces, problem2_space)


problem3_pieces = (
    Piece([(0,0,0), (1,0,0)], 1),
    Piece([(0,0,0), (1,0,0), (1,1,0)], 2),
    Piece([(0,0,0)], 3),
    Piece([(0,0,0), (0,1,0)], 4),
    )
problem3_space = Space((2,2,2))
problem3 = Problem(problem3_pieces, problem3_space)


problem4_pieces = (
    Piece([(0,0,0), (0,1,0), (1,1,0), (2,1,0)]), # L
    Piece([(0,0,0), (1,0,0), (1,1,0), (2,0,0)]), # T
    Piece([(0,0,0), (1,0,0), (2,0,0), (1,1,0), (2,0,1)]),
    Piece([(0,0,0), (1,0,0), (0,1,0), (0,1,1)]),
    Piece([(0,0,0), (1,0,0), (2,0,0), (1,1,0), (1,1,1)]),
    Piece([(0,0,0), (0,1,0), (0,1,1), (1,1,0), (1,2,0)]),
    )
problem4_space = Space((3,3,3))
problem4 = Problem(problem4_pieces, problem4_space)

code_challenge_pieces = (
    Piece([(0, 0, 0), (1, 0, 0), (2, 0, 0), (0, 1, 0)]),
    Piece([(0, 0, 0), (1, 0, 0), (2, 0, 0), (0, 1, 0), (1, 0, 1)]),
    Piece([(0, 0, 0), (1, 0, 0), (2, 0, 0), (1, 0, 1)]),
    Piece([(0, 0, 0), (1, 0, 0), (1, 1, 0), (1, 0, 1), (2, 0, 1)]),
    Piece([(0, 0, 0), (1, 0, 0), (1, 0, 1), (1, 1, 1)]),
    Piece([(0, 0, 0), (1, 0, 0), (2, 0, 0), (1, 0, 1), (1, 1, 1)]),
    )
code_challenge_space = Space((3,3,3))
code_challenge = Problem(code_challenge_pieces, code_challenge_space)


if __name__ == '__main__':
    if len(sys.argv) > 1:
        problem_name = sys.argv[1]
        globals()[problem_name].solve()
    else:
        real_problem.solve()
