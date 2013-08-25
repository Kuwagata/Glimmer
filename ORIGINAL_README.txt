GLIMMER README
==============

0.  INTRODUCTION
1.  INPUT/OUTPUT FILE FORMATS
	1.1.  CSV FILE FORMAT (DENSE)
	1.2.  VEC FILE FORMAT (SPARSE)
2.  CPU GLIMMER
	2.1.  BUILDING
	2.2.  RUNNING
3.  GPU GLIMMER
	3.1.  BUILDING
	3.2.  RUNNING
	3.3.  MODULE DESCRIPTION
	


=== 0.  INTRODUCTION

Glimmer is a program for nonlinear multidimensional scaling.  For more information on the algroithm consult the paper "Glimmer: Multilevel MDS on the GPU" by Ingram, Munzner, and Olano.  

Glimmer consists of two visual c++ projects:  one for the use on the CPU only and one for employing the GPU.  The programs support two different file formats CSV and VEC.  



=== 1.  INPUT/OUTPUT FILE FORMATS

1.1. CSV FILE FORMAT (DENSE) 

Glimmer expects the CSV file to begin with two lines, the contents of which are ignored (to support legacy CSV files).  ALL following lines are of the format:

<FLOAT 1>,<FLOAT 2>, ... ,<FLOAT H>

Where each line represents a point and <FLOAT D> is the Dth coordinate of the point.  All points must have the same number of measurements.  CSV is the only output file format for the programs.



1.2. VEC File format (sparse)

Each line of the VEC file represents a point.  Each line is of the following format:

(<DIM>,<VALUE>) (<DIM>,<VALUE>) ... (<DIM>,<VALUE>)

Each set of parenthesis is a nonzero coordinate of the point.  <DIM> is the dimension of the coordinate and <VALUE> is the point with dimensions zero indexed.  Output is not written in VEC format.



=== 2. CPU Glimmer

CPU Glimmer consits of a single module, 'glimmer.cpp' .  The executable can compute the same layouts as GPU glimmer but with differen command line options.  

2.1.  Building CPU Glimmer

	1.  Open the 'glimmer_cpu/glimmer.sln' file in Visual Studio
	1.1 	(optional) replace glut folder in glimmer_cpu/glimmer/ directory with latest glut build and update link and include directories in project properties.	
	2.  Build the solution
	3.  Output exe is located in glimmer_cpu/release

2.2.  Running CPU Glimmer

2.2.1  Command line

Command line use of CPU glimmer is as follows

glimmer.exe <INPUTFILE> <OUTPUTFILE> <TYPE>

<INPUTFILE> may be of type CSV or VEC.  If CSV then <TYPE> must be "csv".  If VEC then <TYPE> must be "vec".  <OUTPUTFILE> always outputs CSV format.  For example, to load the CSV file "test.txt" into CPU glimmer and save the results to 'test_results.txt' you would type

glimmer.ext test.txt test_results.txt csv


2.2.2  Hard Coded options

Hard coded options are located at the opt of the 'glimmer.cpp' file in a section labeled "CONSTANTS".  The useful options to change are as follows.

	V_SET_SIZE	= the size of the set of neighbor points to use in the stochastic force simulation.
	S_SET_SIZE 	= the size of the set of randomly chosen point to use in the stochastic force simulation.
	USE_GLUT 	= don't invoke opengl to view the results at the end of the run.
	MAX_ITERATION	= maximum iterations of a force simulation.  Increasing this improves quality but increases running times.
	EPS		= force simulation termination threshold.  Increasing this improves quality but increases running times.



=== 3. GPU Glimmer

3.1.  Building glimmer

	1.  Open the 'glimmer_gpu/gltest.sln' file in Visual Studio
	1.1 	(optional) replace glut folder in glimmer_gpu/ directory with latest glut build and update link and include directories in project properties.	
	1.2 	(optional) replace glew folder in glimmer_gpu/ directory with latest glew build and update link and include directories in project properties.	
	2.  Build the solution
	3.  Output exe is located in glimmer_gpu/Debug

3.2.  Running Glimmer (Command line options)

All glimmer command line options are of the form dash and letter.  For example, to bring up the glimmer command line help, you type 'gltest.exe -h' for the entire big list.  Most of the listed command line options are defunct or require some extra coding to get to work.  These are all noted below.  Any options not mentioned below are deprecated.

3.2.1  h option (Help)

Displays all the command line options with a brief description and then quits.  Invalidates any other selected options.

3.2.2  p option (shader folders)

This is the most important command line option.   Pass it the directory where the glimmer shaders reside as an argument.  Example 'gltest.exe -p sources/gpusources'

3.2.3  i option (input file)

Input points from a CSV file.

3.2.4  M option (matrix file)

Reads a distance matrix from file.  The file format is assumed to be lower triangular where each line is a row in the matrix and the Nth line contains N distances.

3.2.5  o option (output file)

Writes output to a CSV file.

3.2.6  v option (near set size)

Sets the V or Near set size in the stochastic force algorithm.  Rounds to the nearest multiple of 4.

3.2.7  s option (random set size)

Sets the S or Random set size in the stochastic force algorithm.  Rounds to the nearest multiple of 4.

3.2.8  n option (demo points)

Sets the cardinality of the demo data set.  The demo data are sampled from an 8 dimensional plane.  Overrides the 'i' option.

3.2.9  k option (skip frames)

Specifies the open gl window to draw the kth frame.  Default is 1.

3.2.10  D option (don't draw)

Turns off opengl window and doesn't draw the points.  Good for speed.

3.2.11  T option (hard termination criteria)

Turns off special termination criteria and uses a hard number of iterations (specified as an argument to this command line option).

3.2.12  f option (stress diagnostic)

Write the current stress and aggregate velocity out to a file.  The filename is the argument to the option.

3.2.13  V option (velocity termination criteria)

Use the change in velocity as the termination criteria instead of stress.

3.2.14  N option (noise)

Add noise to the demo data created in option 'n'.  The argument is a percent.

3.2.15  I option (initial configuration)

2D CSV file that specifies the intial configuration of the embedding.  Useful for testing.

3.2.16  Q option (full stress computation)

Outputs the full N^2 stress metric between the initial embedding and the data.  Useful for testing the stress of the layouts produced by other algorithms... set their layouts as the initial input with I.

3.2.17  K option (swiss roll)

Use the swiss roll as the demo dataset to sample from instead of a plane.

3.2.18  L option (write demo data)

Output a CSV file with the demo data in it.  Useful for exporting the demo data to test on other algorithms.

3.2.19  X and x (for sparse input and always together)

The big X tells gltest.exe where to find the input VEC file.  The little x tells gltest.exe what is the largest number of nonzeros for a point in the input.

3.2.20  A option (class filename)

Points to a class file which is a file with a line for each point and a single integer on each line denoting the class of the point.  This permits the algorithm to draw points with colors.

3.2.21  B option (label screen string)

Draws the current time and iteration as a string in the corner of the screen.

3.2.22  b option (point size)

Sets the size in pixels of the points drawn to the screen.

3.2.23  Z and H options (make a movie)

The Z option tells where to put the ppms and the base of the filename (as an argument).  The H option is a single file where each line is the number of milliseconds to display in the movie.  The idea is to generate the timings using the 'f' option and then strip away the timings from the other stats and then input it using the H option with the Z option to make a movie from the ppms.

3.2.24  w (width of screen)

Sets the width of the viewport in pixels.  Viewport is always square.

3.2.25  F (decimation factor)

Set the decimation factor.  A decimation factor of 8 removes 7/8 points randomly to build the next level in the heirarchy.  The default is 8.

3.2.26  y (minimum set size)

Set the minimum set size.  This is the threshold of the pyramid building algorithm where it decides not to compute another level.

3.2.27  a (View final results)

Rather than closing the window at the end, keep it rolling.  Press 'q' to close the window.  Useful for demos.

3.2.28  j (set termination epsilon)

Set the value of the termination threshold of the force algorithm.  The default is 0.0001.

3.2.29  l (full stress at the end)

Calculate the stress between the final embedding and the data.


3.3.  Glimmer architecture 

GPU glimmer is the result of a sustained 2-year research project.  The resulting code is full of legacy artifacts of the experimental nature of the project.  It is overly complex.  It contains not only the code for Glimmer algorithm but code for an entire GPU graph layout algorithm as well (GLUG), code for using pre-computed distance matrices, code for generating movies, timings, and synthetic benchmark datasets, and even a stochastic Distance Paging architecture.  

3.3.1  CPU Modules

3.3.1.1  gltest.cpp

The main module.  Contains the command line parsing, glut initialization, main loop, and termination condition.

3.3.1.2  data.cpp

Contains functions for file i/o.

3.3.1.3  fbo.cpp

Functions for Framebuffer objects.  These are objects used by opengl for offscreen rendering.

3.3.1.4  graph.cpp

Algorithms for graph loading and search.  Taken mostly from Yehuda Koren.  Used in GPU graph layout.

3.3.1.5  mmio.c

Matrix market i/o functions.  Loads graphs in the matrix market format.

3.3.1.6  sh_dist.cpp

Wrapper functions for setting up and compiling GPU shader code for calculating or fetching high and low dimensional distances.

3.3.1.7  sh_force.cpp

Wrapper functions for setting up and compiling GPU shader code for computing force simulation.

3.3.1.8  sh_rand.cpp

Wrapper functions for setting up and compiling GPU shader code for computing knuth-shuffle permutations.

3.3.1.9  sh_sort.cpp

Wrapper functions for setting up and compiling GPU shader code for sorting textures using sorting networks.

3.3.1.10  sh_stress.cpp

Wrapper functions for setting up and compiling GPU shader code for computing various stress metrics.

3.3.1.11  shader.cpp

Functions for loading and debugging shader source.

3.3.1.12  texture.cpp

Functions for allocating and constructing texture pyramids as well as running shaders for copying textures and dumping textures to disk for debugging.

3.3.1.13  vbo.cpp

Code for setting up vertex buffer data and colors.  Glimmer points are drawn quickly by rendering to vertex buffers.


3.4 Glimmer output

Upon termination, the algorithm outputs the following to standard out:

<NUM_POINTS> <SHADER COMP TIME> <SHADER OPT TIME> <TEXTURE ALLOC TIME> <LAYOUT TIME> <DLOAD RESULTS TIME> <NUM ITERATIONS> <SPARSE STRESS> <FULL STRESS>

NUM_POINTS = Number of points in simulation.
SHADER COMP TIME = Milliseconds required to compile shader code.
SHADER OPT TIME = Milliseconds required to optimize shader code.
TEXTURE ALLOC TIME = Milliseconds required to allocate textures.
LAYOUT TIME = Milliseconds required to compute the embedding coordinates.
DLOAD RESULTS TIME = Milliseconds required to read results from texture memory.
NUM ITERATIONS = Number of force simulation iterations before termination.
SPARSE STRESS = The final value of the sparse stress function at termination.
FULL STRESS = The final normalized stress of the embedding at termination.  Only computed with the 'l' option otherwise it is 0.000 .