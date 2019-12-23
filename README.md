# 3dpacking

Framework and algorithm for solving a specific set of bin packing problems.

![Puzzle of 3d pieces](https://d2t1xqejof9utc.cloudfront.net/screenshots/pics/e45170a1794e832865a5f219ca0c70bf/large.jpg)
Image Source: [http://grabcad.com/library/tetris-3d-puzzle](http://grabcad.com/library/tetris-3d-puzzle)

Specifically, the goal is to put pieces into a box. Pieces are [polycubes](http://en.wikipedia.org/wiki/Polycube). Typically, there is no extra room in the box after placing all of the pieces.

The particular puzzle I'm trying to solve is placing 25 pentacube pieces into a 5x5x5 box. Each piece is different: they enumerate many different ways to make a piece from 5 connected unit cubes.

## Python
The first attempt was done in Python. [Blender](http://www.blender.org/) can be used to visualize things. Specifically, both the algorithm in action as it places pieces and the solved puzzle. See bpi.py for directions on how to do this (bpi: Blender Puzzle Interface).

The following is a live preview of the puzzle as it is being solved:

![Preview of puzzle being solved](/img/solving_preview.png?raw=true)

The following is a render of all 25 pieces layed out:

![Render of 25 pentacube pieces](/img/pieces_render.png?raw=true)

## C
The C algorithm was written because the Python one wasn't fast enough: would run and run without solving the 5x5x5 puzzle. At first, the C algorithm would also run forever without solving the puzzle but with some major algorithm improvements, it now solves the problem quickly. It would be interesting to update the Python version with these changes and see how it behaves. It has not been proven that C was actually necessary to achieve the required speed.

The C algorithm can print a representation of the pieces/space to the terminal:

![Output of C algorithm as it starts placing pieces](/img/solving_start.png?raw=true)

![Output of C algorithm as it solves the problem](/img/solving_end.png?raw=true)



