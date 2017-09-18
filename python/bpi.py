"""
bpi: Blender Puzzle Interface

This module is intended to provide a convenient way for running 
the puzzle script from within Blender. In particular, if changes
are made to the file(s) that define the puzzle module, this allows
them to easily be reloaded without restarting Blender.

To use, first make this module accessible from within Blender. Then
from the Python Console in Blender, do:

import bpi
# puzzle will run

# You make changes to puzzle
bpi.run() # will clean up the blender environment (delete objects, etc...)

bpi.run() # will reload and re-run puzzle

# Repeat stop() and run() as much as desired. Or just do run() (it cleans up first if necessary)

Positional and keyword arguments to run() are simply passed to the 
run() method of puzzle's BlenderApi. Depending on what the run() 
method does, it might expect some arguments.
"""

import sys
import importlib
import traceback

class Api():
    def __init__(self):
        self.puzzle_imported = False

    def run(self, *args, **kwargs):
        if self.puzzle_imported == False:            
            print("Running")
            try:
                import puzzle
            except KeyboardInterrupt:
                raise
            except Exception as e:
                print("Failed to import puzzle. Stack trace:")
                traceback.print_exception(*sys.exc_info(), limit=2)
                return
            else:
                self.puzzle_imported = True
                self.puzzle = puzzle
        else:
            print("ReRunning")
            self.blender_api.cleanup()
            try:
                importlib.reload(self.puzzle)
            except KeyboardInterrupt:
                raise
            except Exception as e:
                print("Failed to re-import puzzle. Stack trace:")
                traceback.print_exception(*sys.exc_info(), limit=2)                                              
                return

        self.blender_api = self.puzzle.BlenderApi()
        self.blender_api.run(*args, **kwargs)

    def stop(self):
        print("Stopping")
        self.blender_api.cleanup()


api = Api()

def run(*args, **kwargs):
    api.run(*args, **kwargs)

def stop(*args, **kwargs):
    api.stop(*args, **kwargs)

run()