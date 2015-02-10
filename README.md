# 3dpacking

Framework and algorithm for solving a specific set of bin packing problems.

![Puzzle of 3d pieces](https://d2t1xqejof9utc.cloudfront.net/screenshots/pics/e45170a1794e832865a5f219ca0c70bf/large.jpg)
Image Source: [http://grabcad.com/library/tetris-3d-puzzle](http://grabcad.com/library/tetris-3d-puzzle)

Specifically, the goal is to put pieces into a box. Pieces are [polycubes](http://en.wikipedia.org/wiki/Polycube). Typically, there is no extra room in the box after placing all of the pieces.

The particular puzzle I'm trying to solve is placing 25 pentacube pieces into a 5x5x5 box. Each piece is different: I believe that they enumerate out all possible ways to make a piece from 5 connected unit cubes.

[Blender](http://www.blender.org/) can be used to visualize things. Specifically, both the algorithm in action as it places pieces and the solved puzzle. See bpi.py for directions on how to do this (bpi: Blender Puzzle Interface).

The following is a live preview of the puzzle as it is being solved:

![Preview of puzzle being solved](/img/solving_preview.png?raw=true)

The following is a render of all 25 pieces layed out:


![Render of 25 pentacube pieces](/img/pieces_render.png?raw=true)