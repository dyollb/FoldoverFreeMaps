# Foldover-free maps in 50 lines of code

This repository contains the source code for 2d/3d constrained boundary mesh untangling published in the paper "Foldover-free maps in 50 lines of code", Vladimir Garanzha, Igor Kaporin, Liudmila Kudryavtseva, François Protais,
Nicolas Ray, Dmitry Sokolov, https://doi.org/10.1145/3450626.34598

Modifications to original code:
- use VTK to load/save meshes (published code only supports VTK file format v4.1)
- add option to pass csv file with locked vertices, instead of locking boundary
