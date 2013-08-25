/*
	GLIMMER

	Multidimensional Scaling on GPU hardware that supports framebuffer 
	objects.  Currently implements the gpusf 96 Algorithm as a smoothing
	function in a multilevel optimization of the MDS stress function.

	Version:	0.3
	Author:		Stephen Ingram
	Date:		8/21/2006	

	Disclaimer:	Neither I nor UBC warrant this code in any way whatsoever.  
				This code is provided "as-is".  Use at your own risk.     
	
*/

//#include "stdafx.h" // Not in linux
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <cstring>
#include <stdio.h>
#include <cmath>
#include <vector>
#include <time.h>
#include <string>
using std::string;
using std::vector;

#include "texture.h"
#include "fbo.h"
#include "vbo.h"
#include "shader.h"
#include "sh_dist.h"
#include "sh_sort.h"
#include "sh_force.h"
#include "sh_rand.h"
#include "sh_stress.h"
#include "data.h"
#include "params.h"
#include "feeder.h"
#include "graph.h"

/*
	Constants
*/

#define ERROR_GLEW		1
#define EXIT_SUCCESS	0
#define PRINT_TIME		0
#define STEP_TIME		0

#define FOOBIE	
#define USEBLENDING

/*
	Global Params/Data
*/

extern Feeder g_feeder;			//  Distance matrix paging data-structure
Tier *g_Pyramid;				//  Pyramid of grids (naive and now singletier with implicit structure)
int n_current_tier = 0;			//  current tier of pyramid

bool b_drawPoints	= true;		// display function state variables
bool b_movePoints	= true;		// UNUSED
bool b_sim_CPU		= false;	// UNUSED

int n_w = WINDOW_WIDTH;			// width in pixels of main window
int n_h = WINDOW_HEIGHT;		// height in pixels of main window
int n_set_size;					// size of indepenedent sets
int n_set_sub_size;				// number of points to subsample from the independent sets
int n_set_min_size=500;//1000;				// threshold at which to stop 
int n_dims  = 0;				// source dimensionality
int n_embed_dims = 0;			// target dimensionality
int n_points = 0;				// point count
//int n_set_points = 0;			// S texture set point count
int n_vmax = 0;					// V set cardinality
int n_smax = 0;					// S set cardinality
float *vec_indices = NULL;		// sparse vector index data
float *data   = NULL;			// input data
float *zero_data   = NULL;		// input data
float *result = NULL;			// output results
float *randr  = NULL;			// random number resource
float *randr_killme  = NULL;	// random number resource
int n_current_pass = 0;			// generic multipass variable
//int n_threshold = 2000000;
int n_debug = 0;
bool b_demo_data = true;
int g_num_points = 10000;
int g_n_decimation_factor = 8;	// order of decimation to build hierarchy
bool b_fast_interp = false;		// choose fast interpolation over elegant interpolation

bool b_lb_down = false;			// state variable for left mouse button
bool b_rb_down = false;			// state variable for right mouse button
int n_down_x = 0;				// state variable for mouse x
int n_down_y = 0;				// state variable for mouse x
float f_angle = 0.0f;			// navigation variable for image rotation
float f_trans_x = 0.0f;			// navigation variable for current translation x
float f_trans_y = 0.0f;			// navigation variable for current translation x
float n_cam_bound = CAMERA_BOUND;	// navigation variable for current zoom level
//int n_vcycles = 0;

int g_n_vcycles=0;							// counts current number of vcycles
int g_n_inner_iters=0;						// counts current number of inner gpusf iterations
bool g_b_up = false;						// determines whether we are going up or down in a vcycle
int g_n_vcycle_count = VCYCLE_COUNT;		// determines the max number of vcycles to execute
int g_n_inner_iters_count = INNER_ITERS;	// determines the max number of gpusf iterations within a vcycle

unsigned int g_n_framecount = 0;	// total number of frames (iterations) executed
int g_n_skipframes = 1;				// determines how many frames to skip when drawing
bool g_b_draw = true;				// determines whether to draw at all or not

clock_t g_time1, g_time2;					// used for overall time of execution
clock_t g_shcomptime1, g_shcomptime2;		// used for timing shader compilation
clock_t g_shopttime1, g_shopttime2;			// used for timing shader optimization
clock_t g_textime1, g_textime2;				// used for timing texture allocation
clock_t g_uploadtime1, g_uploadtime2;		// used for timing gpu upload
clock_t g_runtime1, g_runtime2;				// used for timing glimmer or gpusf iterations
clock_t g_dloadtime1, g_dloadtime2;			// used for timing gpu download

char g_shader_path[4092];	// stores location of shader source files

bool b_output_debug=false;	// output debug printfs ?

int g_n_itercount = 0;		// counts post-vcycle smoothing iterations
int g_n_totalcount = -1;	// determines total number of smoothing iterations to execute (-1 = infinite)
int g_n_lasttotalcount = 0;	// determines total number of smoothing iterations to execute (-1 = infinite)

bool g_b_useStress = true;		// determines whether to compute and store stress calculations
bool g_b_useVel = false;		// determines whether to compute and store velocity calculations
std::vector<int> g_timings;		// stores timings associated with stress per iteration for output
std::vector<int> g_iterates;	// stores iterates associated with stress per iteration for output
//std::vector<double> g_displace;	// stores displacement associated with stress per iteration for output
//std::vector<double> g_displace_smooth;	// stores low-pass filtered displacements
//std::vector<double> g_forces;	// stores displacement associated with stress per iteration for output
//std::vector<double> g_forces_dumbnorm;	// stores displacement associated with stress per iteration for output
//std::vector<double> g_forces_smooth;	// stores low-pass filtered displacements
std::vector<double> g_stresses_raw;	// stores stress per iterations for output
//std::vector<double> g_stresses_1;	// stores stress per iterations for output
std::vector<double> g_stresses_norm;	// stores stress per iterations for output
//std::vector<double> g_stresses_raw_smooth;	// stores stress per iterations for output
//std::vector<double> g_stresses_1_smooth;	// stores stress per iterations for output
std::vector<double> g_stresses_norm_smooth;	// stores stress per iterations for output
std::vector<double> g_stresses_norm_cosc;	// stores stress per iterations for output
std::vector<double> g_vels;		// stores velocities per iterations for output
//std::vector<double> g_vels_dumbnorm;		// stores velocities per iterations for output
std::vector<double> g_vels_smooth;		// stores low-pass velocities per iterations for output
float g_f_maxVelocity = 0.f;	// keeps track of the current maximum velocity
float g_f_minWeightDist = -1.f;	// ignores those distances > this value (ignored if negative)
float g_f_spacing = .1f;	// used in weighting to space smoothstep function edges
float g_f_file_percent = 1.0f;	// used in decimating an input dataset
bool g_b_Terminate = false;		// termination condition for algorithm (at any level)
bool g_b_Interpolating = false;	// iterppppolation!
bool g_b_donotfinish = false;	// don't perform final termination
bool g_b_donotfinished = false;	// don't perform final termination
float g_f_epsilon = 1e-4;		// termination constant epsilon

bool g_b_oneTimeStressEnd = false; // determines whether to simply compute the initial stress of the input configuration
bool g_b_oneTimeStress = false; // determines whether to simply compute the initial stress of the input configuration
bool g_b_initialConfig = false; // use an initial configuration file instead of random inital embedding
bool g_b_useSwissRoll = false;	// use the swissroll instead of the grid as demo data
bool g_b_outputDemoData = false;// output the demo data to a file (for use elsewheres)
bool g_b_useWeighting = false;	// use threshold weighting when computing forces
bool g_b_convertCSV = false;	// convert from CSV to Matlab mat format
bool g_b_useMaj		= false;	// use gpu-majorization algorithm instead of force (not real)
bool g_b_outputPPM  = false;	// output a single final PPM
bool g_b_outputallPPMs = false;	// output every single frame to a PPM
bool g_b_percent_output = false;// output a percentage of a random permutation of the input data (for generating test sets)

//int g_n_windowsize = 9;		// window size of low-pass filter
//float g_f_sinc[] = {0.,    0.0135,    0.0976,    0.2356,    0.3066,    0.2356,    0.0976,    0.0135,         0.};
//int g_n_windowsize = 21;
//float g_f_sinc[] = {0.0002,    0.0018,    0.0063,    0.0146,    0.0272,    0.0436,    0.0621,    0.0805,    0.0963,
    //0.1071,    0.1111,    0.1077,    0.0975,    0.0820,    0.0637,    0.0451,    0.0285,    0.0155,    0.0068,
    //0.0021,    0.0003};
int g_n_windowsize = 51;
float g_f_sinc[] = {-0,	0.000046946888883,	0.000198053598556,	0.000473608306464,	0.000900308709627,	0.001510404016081,	0.00233997650837,	0.003426439755341,	0.00480540834794,	0.006507161877537,	0.00855297748637,	0.010951635653797,	0.013696409587673,	0.016762828335177,	0.020107458355734,	0.023667880802067,	0.027363957060988,	0.031100379692954,	0.034770407291432,	0.038260587889533,	0.041456194060931,	0.044247030595921,	0.046533237917507,	0.048230704647003,	0.049275722116552,	0.04962856099512,	0.049275722116552,	0.048230704647003,	0.046533237917507,	0.044247030595921,	0.041456194060931,	0.038260587889533,	0.034770407291432,	0.031100379692954,	0.027363957060988,	0.023667880802067,	0.020107458355734,	0.016762828335177,	0.013696409587673,	0.010951635653797,	0.00855297748637,	0.006507161877537,	0.00480540834794,	0.003426439755341,	0.00233997650837,	0.001510404016081,	0.000900308709627,	0.000473608306464,	0.000198053598556,	0.000046946888883,	-0 };
float g_f_cosc[] = {0,	-0.00020937301404,	-0.00083238644375,	-0.00187445134867,	-0.003352219513758,	-0.005284158713234,	-0.007680040381756,	-0.010530536243981,	-0.013798126870435,	-0.017410416484704,	-0.021256733995966,	-0.025188599234624,	-0.029024272810166,	-0.032557220569071,	-0.035567944643756,	-0.037838297355557,	-0.039167132882787,	-0.039385989227318,	-0.038373445436298,	-0.036066871845685,	-0.032470479106137,	-0.027658859359265,	-0.02177557557417,	-0.015026761314847,	-0.007670107630023,	0,	0.007670107630023,	0.015026761314847,	0.02177557557417,	0.027658859359265,	0.032470479106137,	0.036066871845685,	0.038373445436298,	0.039385989227318,	0.039167132882787,	0.037838297355557,	0.035567944643756,	0.032557220569071,	0.029024272810166,	0.025188599234624,	0.021256733995966,	0.017410416484704,	0.013798126870435,	0.010530536243981,	0.007680040381756,	0.005284158713234,	0.003352219513758,	0.00187445134867,	0.00083238644375,	0.00020937301404,	-0 };
//int g_n_windowsize = 101;
//float g_f_sinc[] = {-0,	0.000008352469092,	0.000033573560797,	0.000076118365802,	0.000136713239984,	0.000216340194938,	0.000316215018646,	0.000437759491198,	0.000582568163948,	0.000752370266437,	0.000948987392332,	0.001174287692022,	0.001430137364137,	0.001718350290018,	0.002040636693235,	0.00239855172988,	0.002793444924245,	0.003226411358349,	0.003698245502721,	0.004209398540137,	0.004759939984231,	0.005349524331731,	0.00597736341162,	0.00664220500782,	0.007342318235514,	0.008075486046482,	0.00883900512735,	0.009629693338451,	0.010443904721659,	0.011277551985178,	0.012126136253686,	0.012984783755381,	0.013848289005197,	0.014711163937641,	0.015567692344939,	0.016411988888134,	0.017238061871846,	0.018039878908818,	0.018811434549247,	0.019546818912957,	0.020240286340525,	0.02088632307266,	0.021479712975823,	0.022015600355986,	0.022489548941334,	0.022897596167913,	0.023236301969026,	0.023502791348432,	0.023694790107951,	0.02381065320048,	0.023849385288133,	0.02381065320048,	0.023694790107951,	0.023502791348432,	0.023236301969026,	0.022897596167913,	0.022489548941334,	0.022015600355986,	0.021479712975823,	0.020886323072661,	0.020240286340525,	0.019546818912957,	0.018811434549247,	0.018039878908818,	0.017238061871846,	0.016411988888134,	0.015567692344939,	0.014711163937641,	0.013848289005197,	0.012984783755381,	0.012126136253686,	0.011277551985178,	0.010443904721659,	0.009629693338451,	0.00883900512735,	0.008075486046482,	0.007342318235514,	0.00664220500782,	0.005977363411621,	0.005349524331731,	0.004759939984231,	0.004209398540137,	0.003698245502721,	0.003226411358349,	0.002793444924245,	0.00239855172988,	0.002040636693235,	0.001718350290018,	0.001430137364137,	0.001174287692022,	0.000948987392332,	0.000752370266437,	0.000582568163948,	0.000437759491198,	0.000316215018646,	0.000216340194938,	0.000136713239984,	0.000076118365802,	0.000033573560797,	0.000008352469092,	-0};


//int g_n_windowsize = 29;		// window size of low-pass filter
//float g_f_sinc[] = {   -0.0002,   -0.0006,    0.0015,    0.0046,   -0.0000,   -0.0116,   -0.0105,    0.0148,    0.0330,   -0.0000,   -0.0631,   -0.0564,    0.0895,    0.2993,    0.3998,    0.2993,    0.0895,   -0.0564,   -0.0631,   -0.0000,    0.0330,    0.0148,   -0.0105,   -0.0116,   -0.0000,    0.0046,    0.0015,   -0.0006,   -0.0002};
float g_max_vel = 0.f;			// termination variable keeps track of current max velocity
bool g_b_delaySwitch = true;	// 
int g_n_delayCount = g_n_windowsize;	// how long to wait

bool g_b_outputStress = false;	// determines whether to output stress and velocity to a gnuplot friendly file
string output_PPM_name = "";
string output_allPPM_name = "";
string colors_filename = "";
string output_mat_name = "";
string output_filename = "";
string output_demodata_filename = "";
string stress_filename;
string input_config_filename = "";
string g_str_percent_filename = "";

bool g_b_useDistance = false;
string distance_filename;


float* displacement_window = NULL;
float* displacement_bounds = NULL;

bool b_sparse_vector_input = false;
string sparse_vector_filename = "";
int n_max_sparse_entries = 0;

bool b_graph_input = false;
bool b_edge_input = false;
bool b_adj_input = false;
string graph_input_filename = "";

bool g_b_useGraph = false;
bool b_use_paging = false;
int g_n_paging_iterations = 1;
vtx_data *glimmerGraph = NULL;
//GlimmerGraph *glimmerGraph = NULL;
float g_f_norm_dist = 1.f;

bool b_draw_current_time = false;
int g_n_pointsize = 1;
extern clock_t g_preproc_time;

/*
	externally determined camera parameters
*/
extern float f_embed_min_x;
extern float f_embed_max_x;
extern float f_embed_min_y;
extern float f_embed_max_y;

/*
	Vertex Shader (For VTF) UNUSED WE NOW USE RENDER-TO-VERTEX-ARRAY
*/
Shader vsh_copy;
Shader vsh_edges;

/*
	Uniform shader parameters
*/

Uniform u_copy_embed;
Uniform u_edges_embed;
extern Uniform u_sum_finalpass;
extern Uniform u_sum_debug;
bool one_pass = false;

std::vector<clock_t> run_times;

/*
	read times into a vector
*/
void readTimes( const char *filename ) {

	char line[1028];
	FILE *fp = fopen( filename, "r");
	while( fgets( line, 1027, fp ) != NULL ) {
		run_times.push_back( ((clock_t) atol( line )));
		//if( run_times.size( ) ) 
		//	run_times.push_back( ((clock_t) atol( line )) + run_times.at( run_times.size( ) - 1 ) );
		//else {
		//	run_times.push_back( ((clock_t) atol( line )));
		//}
	}
}

void drawCurrentTime( float x, float y ) {
	char time_str[128];
	clock_t curtime = (clock( )-g_time1);
	if( g_b_outputallPPMs ) {
		curtime = run_times.at( g_n_framecount-1 );
	}
	sprintf(time_str, "%0d.%d sec", curtime/1000, curtime % 1000 );
	glPushMatrix();
	glRasterPos3f(-0.7*n_cam_bound,-0.8*n_cam_bound,0.f);
	for( int i = 0; time_str[i]!='\0'; i++ ) {
		//glutStrokeCharacter( GLUT_STROKE_MONO_ROMAN, time_str[i] );
		glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, time_str[i] );
	}
	glPopMatrix();
	sprintf(time_str, "%00d iter", g_n_framecount );
	glPushMatrix();
	glRasterPos3f(0.7*n_cam_bound,-0.8*n_cam_bound,0.f);
	for( int i = 0; time_str[i]!='\0'; i++ ) {
		//glutStrokeCharacter( GLUT_STROKE_MONO_ROMAN, time_str[i] );
		glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, time_str[i] );
	}
	glPopMatrix();
}

/*
	Initialize GL extension wrangler
*/
void initGLEW (void) {
    // init GLEW, obtain function pointers
    int err = glewInit();
    // Warning: This does not check if all extensions used 
    // in a given implementation are actually supported. 
    // Function entry points created by glewInit() will be 
    // NULL in that case!
    if (GLEW_OK != err) {
        printf((char*)glewGetErrorString(err));
        exit(ERROR_GLEW);
    }  
}	

/*
	Generate our dimension-dependent shader source code
*/
void setup_vsh_copy( void ) {

	char shader_path[8192];

	//setup shader vsh_copy
	vsh_copy.program = glCreateProgramObjectARB();
	vsh_copy.shader  = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	glAttachObjectARB (vsh_copy.program, vsh_copy.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "vsh_copy.glsl");
	const GLcharARB *copy_source = readShaderSource(shader_path);
	glShaderSourceARB(vsh_copy.shader, 1, &copy_source, NULL);
	glCompileShaderARB(vsh_copy.shader);
	glLinkProgramARB(vsh_copy.program);
	printInfoLog( "vsh_copy.shader", vsh_copy.shader );
	printInfoLog( "vsh_copy.program", vsh_copy.program );
	u_copy_embed.isFloat = false;
	u_copy_embed.location = glGetUniformLocationARB( vsh_copy.program, "t_embed" );
}

/*
	Compile edge drawing shader code
*/
void setup_vsh_edges( void ) {

	char shader_path[8192];

	//setup shader vsh_copy
	vsh_edges.program = glCreateProgramObjectARB();
	vsh_edges.shader  = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	glAttachObjectARB (vsh_edges.program, vsh_edges.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "vsh_edges.glsl");
	const GLcharARB *edges_source = readShaderSource(shader_path);
	glShaderSourceARB(vsh_edges.shader, 1, &edges_source, NULL);
	glCompileShaderARB(vsh_edges.shader);
	glLinkProgramARB(vsh_edges.program);
	printInfoLog( "vsh_edges.shader", vsh_edges.shader );
	printInfoLog( "vsh_edges.program", vsh_edges.program );
	u_edges_embed.isFloat = false;
	u_edges_embed.location = glGetUniformLocationARB( vsh_edges.program, "t_embed" );
}


/*
	Compile pixel shaders store appropriate handles
*/
void genShaders( void ) {

	g_shcomptime1 = clock();

	setup_sh_dist( );
	setup_sh_force( );
	setup_sh_rand( );
	setup_sh_sort( );
	setup_vsh_copy( );
	setup_vsh_edges( );
	setup_sh_copy( );
	
	//if( g_b_useStress || g_b_oneTimeStress || g_b_useVel ) {
		setup_sh_stress( );
	//}

	g_shcomptime2 = clock();
}

/*
	Cleanup memory and GL data structures for a clean exit (NEEDS WORK)
*/
void cleanup( void ) {

	// free CPU buffers
	free(data);
    free(result);
    free(randr);

	// delete GPU data structures
/*	glDeleteFramebuffersEXT (1,&t_pts.frame_buffer);
	glDeleteTextures (t_pts.attachments,t_pts.texture_name);
	glDeleteFramebuffersEXT (1,&t_embed.frame_buffer);
	glDeleteTextures (t_embed.attachments,t_embed.texture_name);
	glDeleteFramebuffersEXT (1,&t_rand.frame_buffer);
	glDeleteTextures (t_rand.attachments,t_rand.texture_name);
	glDeleteFramebuffersEXT (1,&t_idx.frame_buffer);
	glDeleteTextures (t_idx.attachments,t_idx.texture_name);
	glDeleteFramebuffersEXT (1,&t_d.frame_buffer);
	glDeleteTextures (t_d.attachments,t_d.texture_name);
	glDeleteFramebuffersEXT (1,&t_g.frame_buffer);
	glDeleteTextures (t_g.attachments,t_g.texture_name);*/
}

void idle( void ) {

	glutPostRedisplay();
}

/*
	perform an iteration of GPU-Majorization on a tier

	X_(k+1) = X_k + 0.5 diag( V )^-1 ( B - V ) X_k

*/
void gpumaj( Tier *tier, bool b_fix ) {

	// update index set(s)

	random_sample( tier->t_idx, tier, b_fix );				

	// calculate the low/high-d differences

	if( ! g_b_useDistance ) {
		calc_diff( tier );
	}
	else {
		lookup_dist( tier, b_fix );
	}

	// turn OFF selective S set texture drawing
	tier->b_update_s = false;

	calc_dist_g( tier->t_g,  tier );

	// sort 
	for( int i = 0; i < (n_smax+n_vmax)/2; i++ ) {
		tier->t_g->attach_idx = tier->t_g->attach_idx?0:1;
		sort_odd( tier->t_g, tier->t_idx, tier) ;
		tier->t_d->attach_idx = tier->t_d->attach_idx?0:1;
		sort_odd( tier->t_d, tier->t_idx, tier) ;
		tier->t_idx->attach_idx = tier->t_idx->attach_idx?0:1;
		sort_odd( tier->t_idx, tier->t_idx, tier );
		tier->t_g->attach_idx = tier->t_g->attach_idx?0:1;
		sort_even( tier->t_g, tier->t_idx, tier );
		tier->t_d->attach_idx = tier->t_d->attach_idx?0:1;
		sort_even( tier->t_d, tier->t_idx, tier );
		tier->t_idx->attach_idx = tier->t_idx->attach_idx?0:1;
		sort_even( tier->t_idx, tier->t_idx, tier );
	}

	// mark duplicates
	tier->t_d->attach_idx = (tier->t_d->attach_idx)?0:1;
	mark_duplicates( tier->t_d, tier->t_idx, tier );

	// resort
	for( int i = 0; i < (n_smax+n_vmax)/2 ; i++ ) {
		tier->t_idx->attach_idx = tier->t_idx->attach_idx?0:1;
		sort_odd( tier->t_idx, tier->t_d, tier );
		tier->t_g->attach_idx = tier->t_g->attach_idx?0:1;
		sort_odd( tier->t_g, tier->t_d, tier );
		tier->t_d->attach_idx = tier->t_d->attach_idx?0:1;
		sort_odd( tier->t_d, tier->t_d, tier );
		tier->t_idx->attach_idx = tier->t_idx->attach_idx?0:1;
		sort_even( tier->t_idx, tier->t_d, tier );
		tier->t_g->attach_idx = tier->t_g->attach_idx?0:1;
		sort_even( tier->t_g, tier->t_d, tier );
		tier->t_d->attach_idx = tier->t_d->attach_idx?0:1;
		sort_even( tier->t_d, tier->t_d, tier );
	}

	// duplicate and include diagonal

	// sort by row

	// mark duplicates

	// compute diagonal of B and V (symmetrize)

	// compute 0.5 diag( V )^-1 ( B - V )

	// multiply X_k 
	
	// add to X_k

}

/*
	run the shaders on a small 2 by 2 patch to induce
	the driver to optimize the shaders
*/
void optimize_shaders( Tier *tier ) {
	
	// ensure the function only runs once
	static bool runonce = false;
	
	if( runonce )
		return;
	else
		runonce = true;

	int temp_level = tier->level;
	tier->level = tier->n_levels;

	int force_passes = (((int)ceil(((double)n_vmax) / 4.0 )) + ((int)ceil(((double)n_smax) / 4.0 )));

	shuffle_texture( tier->t_g, tier );
	copyFlip(tier->t_g,0,1,tier);
	if( ! g_b_useGraph ) {
		random_sample( tier->t_g, tier, false);				
	}

	if( ! g_b_useDistance ) {
		if( ! b_sparse_vector_input ) {
			calc_diff( tier );
		}
		else {
			calc_dot_product( tier );
		}
	}
	else if( ! g_b_useGraph ) {
		lookup_dist( tier, false );
	}

	calc_dist_g( tier->t_g,  tier );

	// update near set

	if( ! g_b_useGraph ) {

		// sort by index
		near_update_gen( tier->t_g, tier );

		// mark duplicates
		mark_duplicates( tier->t_g, tier->t_g, tier );

	}

	u_sum_finalpass.valuef = (float)force_passes;
	n_current_pass = 0;
	sum_forces( tier->t_g, tier, false );
	n_current_pass = 0;
	apply_forces( tier->t_g, tier, false);
	
	// integrate the forces and change the velocities of the items
	integrate_forces( tier->t_g, tier, false);
	apply_velocity( tier->t_g, tier, false );
	interpolate_points( tier->t_g, tier );
	sum_velocity( tier, false );
	calc_sp_stress( tier );

	// invalidate null results 

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_g->texture_name[ tier->t_g->attach_idx ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_g->width,tier->t_g->height,
                    GL_RGBA,GL_FLOAT,zero_data);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_g->texture_name[ tier->t_g->attach_idx?0:1 ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_g->width,tier->t_g->height,
                    GL_RGBA,GL_FLOAT,zero_data);

	tier->level = temp_level;
}

void gpusf_interpolate( Tier *tier, bool b_fix ) {

	// perform 10 iterations of stochastic search
	for( int bar = 0; bar < 10; bar++ ) {

		random_sample( tier->t_idx, tier, b_fix );				

		if( ! g_b_useDistance ) {
			if( ! b_sparse_vector_input ) {
				calc_diff( tier );
			}
			else {
				calc_dot_product( tier );
			}
		}
		else if( ! g_b_useGraph ) {
			lookup_dist( tier, b_fix );
		}

		// turn OFF selective S set texture drawing
		tier->b_update_s = false;

		calc_dist_g( tier->t_g,  tier );


		// update near set

		if( ! g_b_useGraph ) {
			//outputTexture( tier->t_d, "d0.txt" );
			//outputTexture( tier->t_idx, "idx0.txt" );

			// sort by index
			near_update_gen( tier->t_idx, tier );

			// mark duplicates
			tier->t_d->attach_idx = (tier->t_d->attach_idx)?0:1;
			mark_duplicates( tier->t_d, tier->t_idx, tier );

			// sort by h distance
			near_update_gen( tier->t_d, tier );

			//outputTexture( tier->t_d, "d1.txt" );
			//outputTexture( tier->t_idx, "idx1.txt" );
			//exit( 0 );
		}

		tier->b_update_s = true;
	}

	// move the points to the barycenter of the near set
	
	tier->t_embed->attach_idx = (tier->t_embed->attach_idx)?0:1;
	interpolate_points( tier->t_embed, tier );

	//if( tier->level < tier->n_levels ) {

	//	outputTexture( tier->t_idx, "idx.txt" );
	//	outputTexture( tier->t_embed, "embed.txt" );
	//	outputTexture( tier->t_d, "d.txt" );
	//	printf("%d by %d\n", tier->t_embed->tier_height[tier->n_levels-1], tier->t_embed->tier_width[tier->n_levels-1] );
	//	exit( 0 );
	//}

	//g_n_framecount++;	// increment the total number of frames
}

/*
	perform an iteration of GPU-SF on a tier
*/
void gpusf( Tier *tier, bool b_fix ) {

	int force_passes = (((int)ceil(((double)n_vmax) / 4.0 )) + ((int)ceil(((double)n_smax) / 4.0 )));

	for( int bar = 0; bar < TEST_ITERS; bar++ ) {

		//printf("\t1:%d\n",clock()-g_time1);		// PROFILE

		if( ! g_b_useGraph ) {
			random_sample( tier->t_idx, tier, b_fix );				
		}

		//printf("\t2:%d\n",clock()-g_time1);		// PROFILE

		if( ! g_b_useDistance ) {
			if( ! b_sparse_vector_input ) {
				calc_diff( tier );
			}
			else {
				calc_dot_product( tier );
			}
		}
		else if( ! g_b_useGraph ) {
			lookup_dist( tier, b_fix );
		}

		//printf("\t3:%d\n",clock()-g_time1);		// PROFILE

		//if( b_fix ) {

		//	outputTexture(tier->t_reference, "ref.txt");
		//if( g_n_framecount == 1000 ) {
			//outputTexture(tier->t_embed, "embed.txt");
			//outputTexture(tier->t_idx, "idx.txt");
			//outputTexture(tier->t_d, "dog.txt");
			//exit( 0 );
		//}
		//}

		// turn OFF selective S set texture drawing
		tier->b_update_s = false;

		calc_dist_g( tier->t_g,  tier );


		// update near set

		if( 1 ) {

			if( ! g_b_useGraph ) {
				//outputTexture( tier->t_d, "d0.txt" );
				//outputTexture( tier->t_idx, "idx0.txt" );

				// sort by index
				near_update_gen( tier->t_idx, tier );

				// mark duplicates
				tier->t_d->attach_idx = (tier->t_d->attach_idx)?0:1;
				mark_duplicates( tier->t_d, tier->t_idx, tier );

				// sort by h distance
				near_update_gen( tier->t_d, tier );

				//outputTexture( tier->t_d, "d1.txt" );
				//outputTexture( tier->t_idx, "idx1.txt" );
				//exit( 0 );
			}
		}
		else {
			// sort 
			for( int i = 0; i < (n_smax+n_vmax)/2; i++ ) {
				tier->t_g->attach_idx = tier->t_g->attach_idx?0:1;
				sort_odd( tier->t_g, tier->t_idx, tier) ;
				tier->t_d->attach_idx = tier->t_d->attach_idx?0:1;
				sort_odd( tier->t_d, tier->t_idx, tier) ;
				tier->t_idx->attach_idx = tier->t_idx->attach_idx?0:1;
				sort_odd( tier->t_idx, tier->t_idx, tier );
				tier->t_g->attach_idx = tier->t_g->attach_idx?0:1;
				sort_even( tier->t_g, tier->t_idx, tier );
				tier->t_d->attach_idx = tier->t_d->attach_idx?0:1;
				sort_even( tier->t_d, tier->t_idx, tier );
				tier->t_idx->attach_idx = tier->t_idx->attach_idx?0:1;
				sort_even( tier->t_idx, tier->t_idx, tier );
			}

			// mark duplicates
			tier->t_d->attach_idx = (tier->t_d->attach_idx)?0:1;
			mark_duplicates( tier->t_d, tier->t_idx, tier );

			// resort
			for( int i = 0; i < (n_smax+n_vmax)/2 ; i++ ) {
				tier->t_idx->attach_idx = tier->t_idx->attach_idx?0:1;
				sort_odd( tier->t_idx, tier->t_d, tier );
				tier->t_g->attach_idx = tier->t_g->attach_idx?0:1;
				sort_odd( tier->t_g, tier->t_d, tier );
				tier->t_d->attach_idx = tier->t_d->attach_idx?0:1;
				sort_odd( tier->t_d, tier->t_d, tier );
				tier->t_idx->attach_idx = tier->t_idx->attach_idx?0:1;
				sort_even( tier->t_idx, tier->t_d, tier );
				tier->t_g->attach_idx = tier->t_g->attach_idx?0:1;
				sort_even( tier->t_g, tier->t_d, tier );
				tier->t_d->attach_idx = tier->t_d->attach_idx?0:1;
				sort_even( tier->t_d, tier->t_d, tier );
			}
		}

		//printf("\t4:%d\n",clock()-g_time1);		// PROFILE

		u_sum_finalpass.valuef = (float)force_passes;
		if( one_pass ) {
			sum_forces( tier->t_force, tier, b_fix );
		}
		else {
			for( int i = 0; i < force_passes; i++ ) {
				//tier->t_sum->attach_idx = (tier->t_sum->attach_idx)?0:1;
				tier->t_force->attach_idx = (tier->t_force->attach_idx)?0:1;
				n_current_pass = i;
				//sum_forces( tier->t_sum, tier, b_fix );
				sum_forces( tier->t_force, tier, b_fix );
			}

			//tier->t_force->attach_idx = (tier->t_force->attach_idx)?0:1;
			//n_current_pass = i;
			//apply_forces( tier->t_force, tier, b_fix );
		}
		
		//printf("\t5:%d\n",clock()-g_time1);		// PROFILE

		// integrate the forces and change the velocities of the items
		tier->t_velocity->attach_idx = (tier->t_velocity->attach_idx)?0:1;
		integrate_forces( tier->t_velocity, tier, b_fix );
		tier->t_embed->attach_idx = (tier->t_embed->attach_idx)?0:1;
		apply_velocity( tier->t_embed, tier, b_fix );

		//printf("\t6:%d\n",clock()-g_time1);		// PROFILE

		//exit( 0 );
		tier->b_update_s = true;
	}

	g_n_framecount++;	// increment the total number of frames
}

/*
	Routine to render contents of frame buffer into vertex array
*/
void rtva( Tier *tier ) {

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tier->t_embed->frame_buffer);
	glReadBuffer( (tier->t_embed->attach_idx)?GL_COLOR_ATTACHMENT1_EXT:GL_COLOR_ATTACHMENT0_EXT );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_embed->texture_name[tier->t_embed->attach_idx]);
	glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, tier->vb_vertexBuffer);	
	glReadPixels(0, 0, tier->t_embed->tier_width[tier->level], tier->t_embed->tier_height[tier->level], 
		GL_RGBA, GL_FLOAT, NULL);
	glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);

}

void drawPoints( Tier *tier ) {

	if( !g_b_draw )
		return;

	if( g_n_framecount % g_n_skipframes )
		return;

	// render to vertex array
	rtva(tier);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	glDrawBuffer( GL_BACK );
	glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(-n_cam_bound,n_cam_bound,-n_cam_bound,n_cam_bound);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,n_w,n_h);

    glRotatef( f_angle, 0.0f,0.0f,1.0f );	
	glTranslatef( f_trans_x, f_trans_y, 0.0f );

//#ifdef USEBLENDING
	//glEnable(GL_BLEND);		// Turn Blending On
	//glDisable(GL_DEPTH_TEST);	// Turn Depth Testing Off
//#endif
	int sizepts = (tier->t_embed->tier_width[tier->level])*(tier->t_embed->tier_height[tier->level]);


	if( b_edge_input || b_adj_input ) {
		//glBindBufferARB(GL_ARRAY_BUFFER_ARB, NULL);
		//glColorPointer( 4, GL_FLOAT, 0, 0 );
		glDisable(GL_COLOR_ARRAY);
//#ifdef USEBLENDING
		glEnable(GL_BLEND);		// Turn Blending On
		glDisable(GL_DEPTH_TEST);	// Turn Depth Testing Off
//#endif
		glUseProgramObjectARB(vsh_edges.program);
		glEnable(GL_VERTEX_ARRAY);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_edgeBuffer);
		glVertexPointer( 4, GL_FLOAT, 0, 0 );
		glDrawArrays(GL_LINES, 0, tier->n_edgecount*2 );
//#ifdef USEBLENDING
		glDisable(GL_BLEND);		// Turn Blending Off
		glEnable(GL_DEPTH_TEST);	// Turn Depth Testing On
//#endif
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, NULL);
		glDisable(GL_VERTEX_ARRAY);
	}

	glUseProgramObjectARB(0);

	glEnable(GL_VERTEX_ARRAY);
	glEnable(GL_COLOR_ARRAY);

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_vertexBuffer);
	glVertexPointer( 4, GL_FLOAT, 0, 0 );
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_colorBuffer);
	glColorPointer( 4, GL_FLOAT, 0, 0 );
	glDrawArrays(GL_POINTS, 0, (sizepts > tier->n_points)?tier->n_points:sizepts );


	glBindBufferARB(GL_ARRAY_BUFFER_ARB, NULL);
	glDisable(GL_VERTEX_ARRAY);
	glDisable(GL_COLOR_ARRAY);


	if( b_draw_current_time ) {
		glColor4f( 0.0, 0.0, 0.0, 1.0 );
		drawCurrentTime( 0., 0.);
	}

//#ifdef USEBLENDING
	//glDisable(GL_BLEND);		// Turn Blending On
	//glEnable(GL_DEPTH_TEST);	// Turn Depth Testing Off
//#endif

	glutSwapBuffers( );

	if( g_b_outputallPPMs ) {
		char PPMname[1028];
		sprintf(PPMname, "%s%03d.ppm", output_allPPM_name.c_str( ), g_n_framecount );
		dumpPPM( PPMname, n_w, n_h );
	}
}

void outputStressVelocity( const char *filename ) {
	FILE *fp = NULL;
	fp = fopen( filename, "w" );
	for( std::vector<int>::size_type i = 0; i < g_timings.size( ); i++ ) {
		fprintf( fp, "%d %d ", g_iterates.at( i ), g_timings.at( i ) );
		if( g_b_useStress ) {
			if( g_timings.size( ) > (unsigned int)g_n_windowsize && i >= g_n_windowsize/2 && 
				i < (g_stresses_norm_cosc.size()+(g_n_windowsize/2)) ) {
				fprintf( fp, "%f %f %f ", 
					g_stresses_norm.at( i ), 
					//g_stresses_raw.at( i ),
					g_stresses_norm_smooth.at( i-(g_n_windowsize/2) ),
					g_stresses_norm_cosc.at( i-(g_n_windowsize/2) ) );
			}
			else {
				fprintf( fp, "%f %f 0 0", 
					g_stresses_norm.at( i ) /*g_stresses_raw.at( i )*/ );
			}
		}
		if( g_b_useVel ) {
#ifdef FULL_DATA_OUTPUT
			if( g_timings.size( ) > (unsigned int)g_n_windowsize && i >= g_n_windowsize/2 && 
				i < (g_vels_smooth.size()+(g_n_windowsize/2)) ) {
				fprintf( fp, "%f %f %f", g_vels.at( i ), g_vels_smooth.at( i-(g_n_windowsize/2) ), g_vels_dumbnorm.at( i ) );
			}
			else {
				fprintf( fp, "%f 0 %f", g_vels.at( i ), g_vels_dumbnorm.at( i ) );
			}

			if( g_timings.size( ) > (unsigned int)g_n_windowsize && i >= g_n_windowsize/2 && 
				i < (g_forces_smooth.size()+(g_n_windowsize/2)) ) {
				//fprintf( fp, " %f %f", g_displace.at( i ), g_displace_smooth.at( i-(g_n_windowsize/2) ) );
				fprintf( fp, " %f %f %f", g_forces.at( i ), g_forces_smooth.at( i-(g_n_windowsize/2) ), g_forces_dumbnorm.at( i ) );
			}
			else {
				//fprintf( fp, " %f 0", g_displace.at( i ) );
				fprintf( fp, " %f 0 %f", g_forces.at( i ), g_forces_dumbnorm.at( i )  );
			}
			fprintf( fp, " %f", g_vels.at( i ), g_displace.at(i) );
#else
			if( g_timings.size( ) > (unsigned int)g_n_windowsize && i >= g_n_windowsize/2 && 
				i < (g_vels_smooth.size()+(g_n_windowsize/2)) ) {
				fprintf( fp, "%f %f", g_vels.at( i ), g_vels_smooth.at( i-(g_n_windowsize/2) ) );
			}
			else {
				fprintf( fp, "%f 0", g_vels.at( i ) );
			}
#endif
		}
		fprintf( fp, "\n" );
	}

	fclose( fp );
}

/*
	Convolve velocity curve with sinc function
*/
bool velTerminate( ) {
	// Check termination based on fixed number of iterations

	//if( !g_b_useVel && !g_b_useStress){
		//if( g_n_totalcount > 0 && (g_n_framecount-g_n_lasttotalcount) == g_n_totalcount ) {
		//	g_n_lasttotalcount = g_n_framecount;
		//	return true;
		//}
		//else {
		//	return false;
		//}
	//}

	bool b_return = false;			// don't signal convergence by default

	float sum = 0.f;				// summation used in filtering
	float vel_noise = 0.00001f;		// positive noise tolerance

	static bool b_positive_slope = true;	// positive slope must be false for convergence
	static bool b_negative_slope = false;	// negative must be true for convergence

	int convergence_window	= 5;	// window across which we check convergence
	int pos_slope_window	= 3;	// window across which we check for positive slope
	int neg_slope_window	= 10;	// window across which we check for negative slope

	// failsafe for termination by convergence

	static int last_iterate = 0;
	int maximum_iterate = 2000;
	if( g_n_framecount-last_iterate > maximum_iterate ) {
		last_iterate = g_n_framecount;
		return true;
	}

	// termination condition fractions

	double term_cond = g_b_Interpolating?g_f_epsilon:g_f_epsilon;	// DEFAULT TERMINATION CONDITION	
	if(g_Pyramid->level == g_Pyramid->n_levels-1 && !g_b_Interpolating )
		term_cond = g_f_epsilon;							// CONDITION WHEN AT HIGHEST 
	if( (g_Pyramid->level == 0 && !g_b_Interpolating ) )
		term_cond = g_f_epsilon;							// CONDITION WHEN AT LOWEST LEVELS

	// we haven't caught up to the starting point of our smooth velocity

	if( g_b_delaySwitch ) {

		// adjust filtering window delay index

		if( ! (--g_n_delayCount) ) {
			g_n_delayCount = g_n_windowsize;
			g_b_delaySwitch = false;
			if( b_output_debug ) {
				printf("\tswitched at %d\n", g_n_framecount);
			}
		}

		// only keep track of the largest velocity

		if( g_b_useVel && g_vels.at( g_n_framecount-1 ) > g_max_vel ) {
			g_max_vel = g_vels.at( g_n_framecount-1 );
		}

		// low-pass filter velocity value (if we have enough samples)
		
		if( ( g_b_useVel && (g_vels.size( ) >= ((unsigned int)g_n_windowsize)) ) ||
			( g_b_useStress && (g_stresses_norm.size( ) >= ((unsigned int)g_n_windowsize)) ) ) {

			if( g_b_useVel ) {	// filter velocity
				sum = 0.f;
				for( int v = 0; v < g_n_windowsize; v++ ) {
					sum += g_f_sinc[v] * g_vels.at( g_n_framecount - v - 1 );
				}
				g_vels_smooth.push_back( sum );
			}
			if( g_b_useStress ) {	// filter stress(es)
				//sum = 0.f;
				//for( int v = 0; v < g_n_windowsize; v++ ) {
				//	sum += g_f_sinc[v] * g_stresses_norm.at( g_n_framecount - v - 1 );
				//}
				//g_stresses_norm_smooth.push_back( sum );
				//sum = 0.f;
				//for( int v = 0; v < g_n_windowsize; v++ ) {
				//	sum += g_f_cosc[v] * g_stresses_norm.at( g_n_framecount - v - 1 );
				//}
				//g_stresses_norm_smooth.push_back( sum );
				g_stresses_norm_cosc.push_back( 0.f );
				g_stresses_norm_smooth.push_back( 0.f );
			}
		}

		return false;
	}
	else {	// ! g_b_DelaySwitch

#ifdef FULL_DATA_OUTPUT
		// filter displacement/force/velocity
		for( int v = 0; v < g_n_windowsize; v++ ) {
			//sum += g_f_sinc[v] * g_displace.at( g_n_framecount - v - 1 );
			sum += g_f_sinc[v] * g_forces.at( g_n_framecount - v - 1 );
		}
		//g_displace_smooth.push_back( sum );
		g_forces_smooth.push_back( sum );
#endif

		if( g_b_useVel ) {
			sum = 0.f;
			for( int v = 0; v < g_n_windowsize; v++ ) {
				sum += g_f_sinc[v] * g_vels.at( g_n_framecount - v - 1 );
			}
			g_vels_smooth.push_back( sum );

			// keep track of the largest filtered velocity
			if( sum > g_max_vel ) {
				g_max_vel = sum;
			}
		}
		if( g_b_useStress ) {
			sum = 0.f;
			for( int v = 0; v < g_n_windowsize; v++ ) {
				sum += g_f_sinc[v] * g_stresses_norm.at( g_n_framecount - v - 1 );
			}
			g_stresses_norm_smooth.push_back( sum );
			sum = 0.f;
			for( int v = 0; v < g_n_windowsize; v++ ) {
				sum += g_f_cosc[v] * g_stresses_norm.at( g_n_framecount - v - 1 );
			}
			g_stresses_norm_cosc.push_back( sum );
		}

		if( g_b_useStress ) {

			if( g_stresses_norm_cosc.at(g_stresses_norm_cosc.size( ) - 1) < term_cond )
				return true;
			else
				return false;

			//// check for positive slope (to handle second or third humps)
	
			//if( g_stresses_norm_smooth.size( ) < ((unsigned int)pos_slope_window) )
			//	b_positive_slope = true;
			//else {
			//	float value1 = g_stresses_norm_smooth.at(g_stresses_norm_smooth.size( )-1);
			//	float value2 = g_stresses_norm_smooth.at(g_stresses_norm_smooth.size( )-pos_slope_window);
			//	b_positive_slope = ( value1 - value2 ) > vel_noise ? true : false ;
			//}

			//// check for negative slope

			//if( g_stresses_norm_smooth.size( ) < 31 )
			//	b_negative_slope = false;
			//else {
			//	float value1 = g_stresses_norm_smooth.at(g_stresses_norm_smooth.size( )-neg_slope_window);
			//	float value2 = g_stresses_norm_smooth.at(g_stresses_norm_smooth.size( )-1);
			//	b_negative_slope =  (value1-value2) < -vel_noise ? false : true ;
			//}

			//// only check for convergence if we have negative slope of the velocity curve

			//if( !b_positive_slope && b_negative_slope) {			

			//	// check for convergence

			//	if( g_stresses_norm_smooth.size( ) > ((unsigned int)convergence_window+1) ) {

			//		float value1 = g_stresses_norm_smooth.at( g_stresses_norm_smooth.size( ) - 1 );
			//		float value2 = g_stresses_norm_smooth.at( g_stresses_norm_smooth.size( ) - 1 - convergence_window );
			//		if( fabs(value1-value2)  < term_cond ) {
			//			b_return = true;
			//		}
			//	}
			//}
		}
		else if( g_b_useVel ) {

			// check for positive slope (to handle second or third humps)

			if( g_vels_smooth.size( ) < ((unsigned int)pos_slope_window) )
				b_positive_slope = true;
			else {
				float value1 = g_vels_smooth.at(g_vels_smooth.size( )-1);
				float value2 = g_vels_smooth.at(g_vels_smooth.size( )-pos_slope_window);
				b_positive_slope = ( value1 - value2 ) > vel_noise ? true : false ;
			}

			// check for negative slope

			if( g_vels_smooth.size( ) < 31 )
				b_negative_slope = false;
			else {
				float value1 = g_vels_smooth.at(g_vels_smooth.size( )-neg_slope_window);
				float value2 = g_vels_smooth.at(g_vels_smooth.size( )-1);
				b_negative_slope =  (value1-value2) < -vel_noise ? false : true ;
			}

			// only check for convergence if we have negative slope of the velocity curve

			if( !b_positive_slope && b_negative_slope) {			

				// check for convergence

				if( g_vels_smooth.size( ) > ((unsigned int)convergence_window+1) ) {

					float value1 = g_vels_smooth.at( g_vels_smooth.size( ) - 1 );
					float value2 = g_vels_smooth.at( g_vels_smooth.size( ) - 1 - convergence_window );
					if( fabs(value1-value2)  < term_cond ) {
						if( b_output_debug ) {
							printf("condition met at %f and %f ", fabs(value1-value2), term_cond*(g_max_vel) );
							printf("betwixt %f and %f\n", value1, value2 );
						}
						b_return = true;
					}
				}
			}
		}
	}

	return b_return;
}

void restrict( Tier *tier, int level ) {

	// restriction only needs to be done at the first level with a random permutation

	if( level == 0 ) {

		int temp_level = tier->level;
		tier->level = 0;

		// begin initial timing
		g_time1 = clock();

		// permute the contents of our state data
		tier->t_perm->attach_idx = tier->t_perm->attach_idx?0:1;
		tier->t_idx->attach_idx = tier->t_idx->attach_idx?0:1;
		shuffle_texture( tier->t_idx, tier );
		if( ! g_b_useDistance ) {
			tier->t_pts->attach_idx = tier->t_pts->attach_idx?0:1;
			shuffle_texture( tier->t_pts, tier );
		}
		if( b_sparse_vector_input ) {
			tier->t_vec_idx->attach_idx = tier->t_vec_idx->attach_idx?0:1;
			shuffle_texture( tier->t_vec_idx, tier );
		}
		tier->t_embed->attach_idx = tier->t_embed->attach_idx?0:1;
		shuffle_texture( tier->t_embed, tier );
		tier->t_velocity->attach_idx = tier->t_velocity->attach_idx?0:1;
		shuffle_texture( tier->t_velocity, tier );
		tier->t_force->attach_idx = tier->t_force->attach_idx?0:1;
		shuffle_texture( tier->t_force, tier );
		//tier->t_sum->attach_idx = tier->t_sum->attach_idx?0:1;
		//shuffle_texture( tier->t_sum, tier );
		tier->t_reference->attach_idx = tier->t_reference->attach_idx?0:1;
		shuffle_texture( tier->t_reference, tier );
		shuffle_texture( tier->t_perm, tier );

		// copy to both sides of the framebuffer objects

		copyFlip( tier->t_idx, tier->t_idx->attach_idx, tier->t_idx->attach_idx?0:1, g_Pyramid );
		if( ! g_b_useDistance ) {
			copyFlip( tier->t_pts, tier->t_pts->attach_idx, tier->t_pts->attach_idx?0:1, g_Pyramid );
			if( b_sparse_vector_input ) {
				copyFlip( tier->t_vec_idx, tier->t_vec_idx->attach_idx, tier->t_vec_idx->attach_idx?0:1, g_Pyramid );
			}
		}
		copyFlip( tier->t_embed, tier->t_embed->attach_idx, tier->t_embed->attach_idx?0:1, g_Pyramid );
		copyFlip( tier->t_velocity, tier->t_velocity->attach_idx, tier->t_velocity->attach_idx?0:1, g_Pyramid );
		copyFlip( tier->t_force, tier->t_force->attach_idx, tier->t_force->attach_idx?0:1, g_Pyramid );
		//copyFlip( tier->t_sum, tier->t_sum->attach_idx, tier->t_sum->attach_idx?0:1, g_Pyramid );
		copyFlip( tier->t_reference, tier->t_reference->attach_idx, tier->t_reference->attach_idx?0:1, g_Pyramid );

		tier->level = temp_level; // jump back to where we were
	}
}

/*
	Main loop for GLUG algorithm
*/
void glug( Tier *tier, int level ) {

	// only draw once if we are outputting a PPM

	if( g_b_outputPPM ) {
		tier->level = 0;

		n_cam_bound = 0.5 * (( (f_embed_max_x-f_embed_min_x) > (f_embed_max_y-f_embed_min_y) )?(f_embed_max_x-f_embed_min_x):(f_embed_max_y-f_embed_min_y));
		n_cam_bound *= 1.1;
		f_trans_x = -((0.5*(f_embed_max_x-f_embed_min_x))+f_embed_min_x);
		f_trans_y = -((0.5*(f_embed_max_y-f_embed_min_y))+f_embed_min_y);

		//n_cam_bound = 0.5 * (( (f_embed_max_x-f_embed_min_x) > (f_embed_max_y-f_embed_min_y) )?(f_embed_max_x-f_embed_min_x):(f_embed_max_y-f_embed_min_y));
		//n_cam_bound *= 0.3;
		//f_trans_x = -((0.1*(f_embed_max_x-f_embed_min_x))+f_embed_min_x);
		//f_trans_y = -((0.5*(f_embed_max_y-f_embed_min_y))+f_embed_min_y);

		drawPoints( tier );
		glFinish();
		dumpPPM(output_PPM_name.c_str(), n_w, n_h);
		exit( 0 );
	}

	tier->level = 0;		// we are only working at the lowest level

	g_b_Terminate = false;
	tier->b_update_s = false;

	gpusf( tier, false ); 
	drawPoints( tier ); 

	g_iterates.push_back( g_n_framecount );
	g_timings.push_back( clock()-g_time1);

	if( g_b_useStress ) {
		g_stresses_norm.push_back( calc_sp_stress( tier ) );
	}
	//float myvel = sum_velocity( tier, true );
	//if( myvel > g_f_maxVelocity ) {
	//	g_f_maxVelocity = myvel;
	//}
	//g_vels.push_back( myvel );
#ifdef FULL_DATA_OUTPUT
	g_vels_dumbnorm.push_back( sum_velocity( tier, false) );
	g_displace.push_back( calc_displacement( tier ) );
	g_forces.push_back( sum_all_forces( tier,true ) );
	g_forces_dumbnorm.push_back( sum_all_forces( tier,false ) );
#endif
	// determine termination condition

	if( velTerminate( ) ) {
		g_b_Terminate = true;

		if( g_b_donotfinish && g_b_draw ) {
			g_b_donotfinished = true;
			return;
		}

		g_time2 = clock( );

		if( g_b_outputStress )
			outputStressVelocity( stress_filename.c_str( ) );

		if( strcmp( output_filename.c_str( ), "" ) )
			outputPoints( output_filename.c_str( ) );

		//if( b_output_debug ) {
		printf("%d %d %d %d %d\n", tier->n_points, g_preproc_time, (g_time2-g_time1), g_preproc_time+(g_time2-g_time1) ,g_n_framecount );
		//}

		exit( 0 );
	}
}
/*
	Main loop for GLIMMER algorithm
*/
void glimmer( Tier *tier, int level ) {

	// only draw once if we are outputting a PPM

	if( g_b_outputPPM ) {
		tier->level = 0;

		n_cam_bound = 0.5 * (( (f_embed_max_x-f_embed_min_x) > (f_embed_max_y-f_embed_min_y) )?(f_embed_max_x-f_embed_min_x):(f_embed_max_y-f_embed_min_y));
		n_cam_bound *= 1.1;
		f_trans_x = -((0.5*(f_embed_max_x-f_embed_min_x))+f_embed_min_x);
		f_trans_y = -((0.5*(f_embed_max_y-f_embed_min_y))+f_embed_min_y);
		drawPoints( tier );
		glFinish();
		dumpPPM(output_PPM_name.c_str(), n_w, n_h);
		exit( 0 );
	}

	// recursion condition
	if( g_n_vcycle_count ) {

		//printf("glimmer %d\n", level );

		if( level >= tier->n_levels ) {
			//printf("done.\n", level );
			return;
		}

		restrict( tier, level );	// this can be eliminated

		glimmer( tier, level+1 );

		tier->level = level;

		// interpolate

		g_max_vel = 0.f;			// reset terimation criteria
		tier->b_update_s = false;
		g_b_Terminate = false;

		if( b_fast_interp ) {
			// run *new* interpolation step
			if( tier->level != tier->n_levels-1 ) {
				gpusf_interpolate( tier, true );
				//outputTexture(tier->t_embed, "foo.txt");
				//exit( 0 );
			}
			drawPoints( tier );
		}
		else {
			//if( tier->level != tier->n_levels-1 ) {
			//	gpusf_interpolate( tier, true );
			//}
			//drawPoints( tier );

			while( !g_b_Terminate && tier->level != tier->n_levels-1) {
				
				g_b_Interpolating = true;
				//printf("interpolating\n");

				gpusf( tier, true ); // freeze the upper tier and gpusf the rest
				drawPoints( tier ); 

				g_iterates.push_back( g_n_framecount );
				g_timings.push_back( clock()-g_time1);

				if( g_b_useStress ) {
					g_stresses_norm.push_back( calc_sp_stress_norm( tier ) );
					//g_stresses.push_back( calc_stress( tier ) );
				}
				if( g_b_useVel ) {
					float myvel = sum_velocity( tier, true );
					if( myvel > g_f_maxVelocity ) {
						g_f_maxVelocity = myvel;
					}
					g_vels.push_back( myvel );
				}
	#ifdef FULL_DATA_OUTPUT
				g_vels_dumbnorm.push_back( sum_velocity( tier, false ) );

				g_displace.push_back( calc_displacement( tier ) );
				g_forces.push_back( sum_all_forces( tier, true ) );
				g_forces_dumbnorm.push_back( sum_all_forces( tier, true ) );
	#endif
				// determine termination condition

				if( velTerminate( ) ) {
					g_b_delaySwitch = true;
					g_b_Terminate = true;
				}

			}
			g_b_Interpolating = false;
		}

		if( b_output_debug ) {
			printf("i:%d\n", g_n_framecount);
		}

		// smooth

		//g_max_vel = 0.f;			// reset terimation criteria
		//tier->b_update_s = false;
		g_b_Terminate = false;
		while( !g_b_Terminate ) {

			//printf("smoothing\n");

			gpusf( tier, false ); // freeze the upper tier and gpusf the rest

			drawPoints( tier ); 

			g_iterates.push_back( g_n_framecount );
			g_timings.push_back( clock()-g_time1);

			if( g_b_useStress ) {
				g_stresses_norm.push_back( calc_sp_stress_norm( tier ) );
				//g_stresses.push_back( calc_sp_stress( tier ) );
			}
			if( g_b_useVel ) {
				float myvel = sum_velocity( tier, true ); //sum_velocity( tier, true );
				if( myvel > g_f_maxVelocity ) {
					g_f_maxVelocity = myvel;
				}
				g_vels.push_back( myvel );
			}
#ifdef FULL_DATA_OUTPUT
			g_vels_dumbnorm.push_back( sum_velocity( tier, false ) );
			g_displace.push_back( calc_displacement( tier ) );
			g_forces.push_back( sum_all_forces( tier, true ) );
			g_forces_dumbnorm.push_back( sum_all_forces( tier, false ) );
#endif
			// determine termination condition

			if( velTerminate( ) ) {
				g_b_delaySwitch = true;
				g_b_Terminate = true;
			}
		}

		if( b_output_debug ) {
			printf("s:%d\n", g_n_framecount);
		}

		// final termination
		if( level == 0 ) {


			if( g_b_donotfinish && g_b_draw ) {
				g_b_donotfinished = true;
				return;
			}

			g_time2 = clock( );

			if( g_b_outputStress )
				outputStressVelocity( stress_filename.c_str( ) );

			if( strcmp( output_filename.c_str( ), "" ) )
				outputPoints( output_filename.c_str( ) );

			float final_stress = calc_sp_stress( tier );// todo KILLME

			//if( b_output_debug ) {
			printf("%d %d %d %d %d %d %d %f %f\n", tier->n_points, g_shcomptime2-g_shcomptime1, g_shopttime2-g_shopttime1, g_textime2-g_textime1, (g_time2-g_time1), g_dloadtime2-g_dloadtime1, g_n_framecount, final_stress, (g_b_oneTimeStressEnd?calc_stress( tier ):0.) );
			//}
			
			exit( 0 );

		}
		else {
			copyFlip( tier->t_embed, tier->t_embed->attach_idx, tier->t_embed->attach_idx?0:1, g_Pyramid );
		}
	}
	else { // handle GPU-SF

		// begin initial timing
		g_time1 = clock();

		tier->level = 0;		// we are only working at the lowest level

		g_b_Terminate = false;
		tier->b_update_s = false;
		while( !g_b_Terminate ) {

			gpusf( tier, false ); 
			drawPoints( tier ); 

			g_iterates.push_back( g_n_framecount );
			g_timings.push_back( clock()-g_time1);

			if( g_b_useStress ) {
				g_stresses_norm.push_back( calc_sp_stress_norm( tier ) );
				//g_stresses_raw.push_back( calc_stress( tier ) );	// DEBUG: KILL ME
			}
			if( g_b_useVel ) {
				float myvel = sum_velocity( tier, true );
				if( myvel > g_f_maxVelocity ) {
					g_f_maxVelocity = myvel;
				}
				g_vels.push_back( myvel );
			}
#ifdef FULL_DATA_OUTPUT
			g_vels_dumbnorm.push_back( sum_velocity( tier, false) );
			g_displace.push_back( calc_displacement( tier ) );
			g_forces.push_back( sum_all_forces( tier,true ) );
			g_forces_dumbnorm.push_back( sum_all_forces( tier,false ) );
#endif
			// determine termination condition

			if( velTerminate( ) ) {
				g_b_Terminate = true;
			}
		}

		if( g_b_donotfinish && g_b_draw ) {
			g_b_donotfinished = true;
			return;
		}

		g_time2 = clock( );

		if( g_b_outputStress )
			outputStressVelocity( stress_filename.c_str( ) );

		if( strcmp( output_filename.c_str( ), "" ) )
			outputPoints( output_filename.c_str( ) );

		float final_stress = calc_sp_stress( tier );// todo KILLME

		//if( b_output_debug ) {
		printf("%d %d %d %d %d %d %d %f %f\n", tier->n_points, g_shcomptime2-g_shcomptime1, g_shopttime2-g_shopttime1, g_textime2-g_textime1, (g_time2-g_time1), g_dloadtime2-g_dloadtime1, g_n_framecount, final_stress, (g_b_oneTimeStressEnd?calc_stress( tier ):0.) );
		//}

		exit( 0 );
	}
}

///*
//	Recursively apply gpusf on coarse-to-fine
//	grids until a threshold is met.
//*/
//void multigrid_vcycle( Tier *tier ) {
//
//	static clock_t time1,time2,footime;
//
//	if( g_n_framecount == 0 ) {
//		g_time1 = clock();
//		time1 = clock( );
//	}
//
//	// only draw once if we are outputting a PPM
//
//	if( g_b_outputPPM ) {
//		tier->level = 0;
//		drawPoints( tier );
//		dumpPPM(output_PPM_name.c_str(), n_w, n_h);
//		exit( 0 );
//	}
//
//
//	// performing V-CYCLE
//
//	if( g_n_vcycles < g_n_vcycle_count ) {
//
//		if( ( g_n_inner_iters == 0 ) && (tier->level == tier->n_levels-1) && ( g_b_up ) ){
//
//			tier->level = 0; // jump to the bottom level
//
//			// permute the contents of our state data
//			tier->t_perm->attach_idx = tier->t_perm->attach_idx?0:1;
//			tier->t_idx->attach_idx = tier->t_idx->attach_idx?0:1;
//			shuffle_texture( tier->t_idx, tier );
//			if( ! g_b_useDistance ) {
//				tier->t_pts->attach_idx = tier->t_pts->attach_idx?0:1;
//				shuffle_texture( tier->t_pts, tier );
//				if( b_sparse_vector_input ) {
//					tier->t_vec_idx->attach_idx = tier->t_vec_idx->attach_idx?0:1;
//					shuffle_texture( tier->t_vec_idx, tier );
//				}
//			}
//			tier->t_embed->attach_idx = tier->t_embed->attach_idx?0:1;
//			shuffle_texture( tier->t_embed, tier );
//			tier->t_velocity->attach_idx = tier->t_velocity->attach_idx?0:1;
//			shuffle_texture( tier->t_velocity, tier );
//			tier->t_force->attach_idx = tier->t_force->attach_idx?0:1;
//			shuffle_texture( tier->t_force, tier );
//			tier->t_sum->attach_idx = tier->t_sum->attach_idx?0:1;
//			shuffle_texture( tier->t_sum, tier );
//			tier->t_reference->attach_idx = tier->t_reference->attach_idx?0:1;
//			shuffle_texture( tier->t_reference, tier );
//			shuffle_texture( tier->t_perm, tier );
//
//			// copy to both sides of the framebuffer objects
//
//			copyFlip( tier->t_idx, tier->t_idx->attach_idx, tier->t_idx->attach_idx?0:1, g_Pyramid );
//			if( ! g_b_useDistance ) {
//				copyFlip( tier->t_pts, tier->t_pts->attach_idx, tier->t_pts->attach_idx?0:1, g_Pyramid );
//				if( b_sparse_vector_input ) {
//					copyFlip( tier->t_vec_idx, tier->t_vec_idx->attach_idx, tier->t_vec_idx->attach_idx?0:1, g_Pyramid );
//				}
//			}
//			copyFlip( tier->t_embed, tier->t_embed->attach_idx, tier->t_embed->attach_idx?0:1, g_Pyramid );
//			copyFlip( tier->t_velocity, tier->t_velocity->attach_idx, tier->t_velocity->attach_idx?0:1, g_Pyramid );
//			copyFlip( tier->t_force, tier->t_force->attach_idx, tier->t_force->attach_idx?0:1, g_Pyramid );
//			copyFlip( tier->t_sum, tier->t_sum->attach_idx, tier->t_sum->attach_idx?0:1, g_Pyramid );
//			copyFlip( tier->t_reference, tier->t_reference->attach_idx, tier->t_reference->attach_idx?0:1, g_Pyramid );
//
//			tier->level = tier->n_levels-1; // jump back to the top
//		}
//
//		g_n_inner_iters++; // increase our inner iteration count
//
//		// if we're not at the top level and our inner iterations are under some threshold
//		if( tier->level < tier->n_levels-1 && g_n_inner_iters < g_n_inner_iters_count ) {
//			gpusf( tier, true ); // freeze the upper tier and gpusf the rest
//			g_b_Terminate = false;
//		}
//		else 
//			gpusf( tier, false ); // otherwise run gpusf on the entire subset
//
//		drawPoints( tier ); 
//
//		//if(!( g_b_useVel && (tier->level == tier->n_levels-1) ) ){
//		if(!g_b_useVel ){
//			g_b_Terminate = false;
//			if( g_n_inner_iters > g_n_inner_iters_count ) {
//				g_b_Terminate = true;
//			}
//		}
//
//		// if our terminating condition for our single tier is met
//		if( g_b_Terminate ) {
//
//			g_b_Terminate = false;
//
//			g_n_inner_iters = 0; // reset our inner iteration counter
//
//			// check which direction in the v-cycle we're travelling
//			if( g_b_up ) {
//				tier->level--;
//				tier->b_update_s = !tier->b_update_s;
//
//				// have we descended too far?
//				if( tier->level < 0 ) {
//					tier->level = 0;
//					g_b_up = false;
//					g_n_vcycles ++;
//				}
//			}
//			else {
//				copyFlip( tier->t_idx, tier->t_idx->attach_idx, tier->t_idx->attach_idx?0:1, g_Pyramid );
//				if( ! g_b_useDistance ) {
//					copyFlip( tier->t_pts, tier->t_pts->attach_idx, tier->t_pts->attach_idx?0:1, g_Pyramid );
//					if( b_sparse_vector_input ) {
//						copyFlip( tier->t_vec_idx, tier->t_vec_idx->attach_idx, tier->t_vec_idx->attach_idx?0:1, g_Pyramid );
//					}
//				}
//				copyFlip( tier->t_embed, tier->t_embed->attach_idx, tier->t_embed->attach_idx?0:1, g_Pyramid );
//				copyFlip( tier->t_velocity, tier->t_velocity->attach_idx, tier->t_velocity->attach_idx?0:1, g_Pyramid );
//				copyFlip( tier->t_force, tier->t_force->attach_idx, tier->t_force->attach_idx?0:1, g_Pyramid );
//				copyFlip( tier->t_sum, tier->t_sum->attach_idx, tier->t_sum->attach_idx?0:1, g_Pyramid );
//				copyFlip( tier->t_reference, tier->t_reference->attach_idx, tier->t_reference->attach_idx?0:1, g_Pyramid );
//				tier->level++;
//				tier->b_update_s = !tier->b_update_s;
//				if( tier->level == tier->n_levels ) {
//					tier->b_update_s = !tier->b_update_s;
//					tier->level = tier->n_levels-1;
//					g_b_up = true;
//				}
//			}
//		}
//	}
//
//	// gpusf 96 Refinement Stage
//	else {
//
//		if( !g_b_useVel ) {
//			if( g_n_totalcount > 0 && g_n_itercount == g_n_totalcount ) 
//				g_b_Terminate = true;
//		}
//
//		if( g_b_Terminate ) {
//			if( g_b_outputStress )
//				outputStressVelocity( stress_filename.c_str( ) );
//
//			if( strcmp( output_filename.c_str( ), "" ) )
//				outputPoints( output_filename.c_str( ) );
//
//			g_time2 = clock( );
//			//if( b_output_debug ) {
//			printf("%d %d %d\n", tier->n_points, (g_time2-g_time1), g_n_framecount );
//			//}
//
//			exit( 0 );
//		}
//
//		tier->level = 0;
//		if( tier->level < tier->n_levels-1 && g_n_inner_iters < g_n_inner_iters_count ) {
//			gpusf( tier, true ); // freeze the upper tier and gpusf the rest
//			g_b_Terminate = false;
//		}
//		else 
//			gpusf( tier, false ); // otherwise run gpusf on the entire subset
//		drawPoints( tier );
//		g_n_itercount++;
//		g_n_inner_iters++; // increase our inner iteration count
//	}
//
//	// calculate stress, velocity, and time for this iteration
//
//	if( g_b_useStress || g_b_useVel ) {
//
//		if( g_b_useStress ) {
//			footime = clock();
//			g_stresses.push_back( calc_sp_stress( tier ) );
//			time2 = clock();
//			time1 += time2 - footime;
//		}
//		if( g_b_useVel ) {
//			float myvel = calc_sp_stress( tier );//sum_velocity( tier, true );
//			if( myvel > g_f_maxVelocity ) {
//				g_f_maxVelocity = myvel;
//			}
//			g_vels.push_back( myvel );
//
//			// determine termination condition
//
//			if( velTerminate( ) ) {
//				g_b_Terminate = true;
//			}
//
//			
//		}
//
//		g_timings.push_back( g_n_framecount );
//	}
//
//	g_n_framecount++;	// increment the total number of frames
//}

/*
	GLUT keyboard function
*/
void kbfunc( unsigned char key, int x, int y ) {

	switch( key ) {
		case 'a':
		case 'A':
			g_f_minWeightDist+=0.05;
			printf("cutoff = %f\n", g_f_minWeightDist);
			break;
		case 'z':
		case 'Z':
			g_f_minWeightDist-=0.05;
			printf("cutoff = %f\n", g_f_minWeightDist);
			break;
		case 's':
		case 'S':
			g_f_spacing+=0.01;
			printf("spacing = %f\n", g_f_spacing);
			break;
		case 'x':
		case 'X':
			g_f_spacing-=0.01;
			printf("spacing = %f\n", g_f_spacing);
			break;
		case 'v':
		case 'V':
			g_n_vcycle_count++;
			break;
		case 'q':
		case 'Q':
		case '\27':
			if( g_b_outputStress )
				outputStressVelocity( stress_filename.c_str( ) );

			if( strcmp( output_filename.c_str( ), "" ) )
				outputPoints( output_filename.c_str( ) );

			g_time2 = clock( );
			if( g_b_useGraph) 
				printf("%d %d %d %d %d\n", g_Pyramid->n_points, g_preproc_time, (g_time2-g_time1), g_preproc_time+ (g_time2-g_time1), g_n_framecount );
			else
				printf("%d %d %d\n", g_Pyramid->n_points, (g_time2-g_time1), g_n_framecount );
			exit( 0 );
			break;
		case ' ':
			glutPostRedisplay();
		default:
			break;
	}

}

void oneTimeStress( Tier *tier ) {
	float stress = calc_stress( tier );
	fprintf( stdout, "%f\n", stress );
	exit( 0 );
}

/*
	GLUT display function for processing our points
*/
void display( void ) {

	if( g_b_oneTimeStress ) 
		oneTimeStress( g_Pyramid );
	else {

		// optimize shaders
		g_shopttime1 = clock();
		optimize_shaders( g_Pyramid );
		g_shopttime2 = clock();

		if( !g_b_useGraph ) {

			if( !g_b_donotfinished )
				glimmer(g_Pyramid,0);		// MDS case
			else
				drawPoints( g_Pyramid );
		}
		else {
			if( !g_b_donotfinished )
				glug( g_Pyramid, 0);		// Graph Layout case
			else
				drawPoints( g_Pyramid );
		}
	}
}

    
void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if(h == 0)
		h = 1;

	n_w = w;
	n_h = h;

}


/*
	Glut callback handles mouseclicks
*/
void mouseCallback( int button, int state, int x, int y ) {

	if( state == GLUT_DOWN ) {
		if( button == GLUT_LEFT_BUTTON ) {
			b_lb_down = true;
		}
		else if( button == GLUT_RIGHT_BUTTON ) {
			b_rb_down = true;
		}
	}
	else { 
		if( button == GLUT_LEFT_BUTTON ) {
			b_lb_down = false;
		}
		else if( button == GLUT_RIGHT_BUTTON ) {
			b_rb_down = false;
		}
	}
	n_down_x = x;
	n_down_y = y;
}

/*
	GLUT callback handles mouse motion
*/
void mouseMotionCallback( int x, int y ) {
	int pixelx = x - n_down_x;
	int pixely = y - n_down_y;
	if( b_lb_down ) {
		f_trans_x += ((float)pixelx) * (2.0f*n_cam_bound)/((float)n_w);
		f_trans_y += -((float)pixely) * (2.0f*n_cam_bound)/((float)n_h);
	}
	if( b_rb_down ) {
		//f_angle += ((float)pixely);
		n_cam_bound += ((float)pixely)/50.0f;
		n_cam_bound = (n_cam_bound <= 0)?(1.f/50.f):n_cam_bound;
	}
		n_down_x = x;
		n_down_y = y;
}

/*
	Populate the multigrid tier with proper GPU data structures
*/
Tier *fillTier( Tier *tier ) {


	g_textime1 = clock();

	// contruct a random number resource
	initRand( tier );
	initPerm( tier );
	initZero( tier );

	// create textures and framebuffer objects
	
	genTextures( tier );

	// gen texture multilevel coordinates

	sizeTextures( tier, g_n_decimation_factor );
	tier->level = tier->n_levels-1;
	g_b_up = true;

	// initialize vertex buffers
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	if( colors_filename.length() )
		initVBO( tier, colors_filename.c_str() );
	else 
		initVBO( tier, NULL );

	g_textime2 = clock();

	// build coordinates

	return NULL;

}

/*
	Initialize GLUT, GLEW, etc.
*/
void initGL(int argc, char **argv) {

	glutInit ( &argc, argv );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB );
	glutInitWindowSize(n_w,n_h);
    glutCreateWindow("MDSGPU");  
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutMouseFunc( mouseCallback );
	glutMotionFunc( mouseMotionCallback );
	if( STEP_TIME )
		glutIdleFunc(NULL);
	glutKeyboardFunc( kbfunc );
	glutReshapeFunc(changeSize);
#ifdef USEBLENDING
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
#endif
	initGLEW();

	if (GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader) {
		if( b_output_debug ) {
			printf("Ready for GLSL\n");
		}
	}
	else {
		printf("Not totally ready :( \n");
		exit(1);
	}
	if( b_output_debug ) {
		GLint params[10];
		glGetIntegerv( GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB, params );
		printf("\tMAX_TEXTURE_RECTANGLE_SIZE_ARB=%d\n", params[0]);
	}

	// generate, compile, and link our shader source
	if( b_output_debug ) {
		printf("Generating Shaders ...");
	}
	genShaders();

	if( b_output_debug ) {
		printf("done.\n");
	}

	// fill our pyramid with texture data

	fillTier( g_Pyramid );

	if( b_edge_input ) {
		initEdges( g_Pyramid, graph_input_filename.c_str( ) );
	}
	if( b_adj_input ) {
		//initEdgesAdj( g_Pyramid, graph_input_filename.c_str( ) );
		initEdgesAdj( g_Pyramid, glimmerGraph, g_Pyramid->n_points );
	}

	glClearColor( 1.0, 1.0, 1.0, 1.0 );
	//glClearColor( 0.0, 0.0, 0.0, 1.0 );
	glPointSize( g_n_pointsize ); 

}

/*
	Display program usage statistics
*/
void usage( const char *name ) {
	printf("Usage:\n\t%s -[hioevsnctkpdDTSVM] \n", name );
	printf("\n\t\th - display this message\n" );
	printf("\t\ti - input points filename [DEMO GRID]\n" );
	printf("\t\tM - distance matrix \n" );
	printf("\t\to - output filename [UNUSED]\n" );
	printf("\t\te - embedding dimension [2]\n" );
	printf("\t\tv - V set size[4]\n" );
	printf("\t\ts - S set size[4]\n" );
	printf("\t\tn - Number of demo points [10000]\n" );
	printf("\t\tc - V cycles [1] \n" );
	printf("\t\tt - Inner iterations [30] \n" );
	printf("\t\tk - only draw the kth frame [1] \n" );
	printf("\t\tp - shader sourch path [.] \n" );
	printf("\t\td - output debug prints [DON'T] \n" );
	printf("\t\tD - do not draw output [DRAW OUTPUT] \n" );
	printf("\t\tT - terminate after T iterations [DON'T] \n" );
	printf("\t\tf - stress/velocity filename [DON'T] \n" );
	printf("\t\tV - calculate (and terminate) using velocity differential [DON'T] \n" );
	printf("\t\tS - calculate stress [DON'T] \n" );
	printf("\t\tN - demo data noise magnitude [0.0] \n" );
	printf("\t\tO - demo data noise dimensions [1] \n" );
	printf("\t\tI - CSV file as init config of embedding [RANDOM CONFIG] \n" );
	printf("\t\tQ - one-time stress run [NORMAL SIMULATION] \n" );
	printf("\t\tK - use swiss roll [DON'T]\n" );
	printf("\t\tL - output demo data [DON'T]\n" );
	printf("\t\tW - set weighting threshold [DON'T]\n" );
	printf("\t\tR - conveRt csv to matlab matrix\n" );
	printf("\t\tP - output the initial configuration as a ppm\n" );
	printf("\t\tx - max number of nonzeros in sparse vec file\n" );
	printf("\t\tX - sparse vec filename\n" );
	printf("\t\tG - matrix market graph filename\n" );
	printf("\t\tE - matrix market graph filename (for drawing edges)\n" );
	printf("\t\tm - distance matrix for paging only\n" );
	printf("\t\tg - edge edge filename for distance paging only\n" );
	printf("\t\tq - max graph distance for option g\n" );
	printf("\t\tA - clAss filename, an integer for each point on a line alone\n" );
	printf("\t\tJ - adJacency filename for drawing edges for edge edge files\n" );
	printf("\t\tB - render current time and iteration as a string\n" );
	printf("\t\tb - set point size\n" );
	printf("\t\tZ - output all frames as ppms with base filename, also outputs timing\n" );
	printf("\t\tz - sets the zoom [1]\n" );
	printf("\t\tw - sets the screen width/height in pixels [500]\n" );
	printf("\t\tH - times file for inserting into PPM output\n" );
	printf("\t\tC - perCent of input file to use [100]\n");
	printf("\t\tF - decimation Factor [8]\n");
	printf("\t\ty - minimum set size [500]\n");
	printf("\t\tY - use slow interpolation [DON'T]\n");
	printf("\t\ta - do not perform final termination\n");
	printf("\t\tj - set termination epsilon [1e-4]\n");
	printf("\t\tl - calculate the full normalized stress at end\n");
	printf("\t\tr - output a random percent of the input file (see -C)\n");
	exit( 0 );
}

void proc_command_args( int argc, char **argv ) {

	int i = 0; 
	n_embed_dims = 2;
	char *input_filename = NULL;
	n_vmax = 4;
	n_smax = 4;
	int n_noise_dimensions = 1;
	double d_noise_magnitude = 0.0;

	while( i < argc ) {
		if( ( argv[i][0] == '-' ) && (strlen( argv[i] ) > 1 ) ){
			switch( argv[i][1] ) {
				case 'h':
					usage( argv[0] );
					break;
				case 'r':
					g_b_percent_output = true;
					g_str_percent_filename = argv[i+1];
					break;
				case 'l':
					g_b_oneTimeStressEnd = true;
					break;
				case 'j':
					g_f_epsilon = atof( argv[i+1] );
					break;
				case 'a':
					g_b_donotfinish = true;
					break;
				case 'Y':
					b_fast_interp = true;
					break;
				case 'y':
					n_set_min_size = atoi( argv[i+1] );
					break;
				case 'F':
					g_n_decimation_factor = atoi( argv[i+1] );
					break;
				case 'C':
					g_f_file_percent = ((double)atoi(argv[i+1]))/1000.f;
					if( g_f_file_percent == 0.f ) {
						printf("ERROR: percent must be > 0.0" );
						exit( 0 );
					}
					break;
				case 'H':
					readTimes( argv[i+1] );	
					break;
				case 'Z':
					g_b_outputallPPMs = true;
					output_allPPM_name = argv[i+1];
					break;
				case 'z':
					n_cam_bound = atof(argv[i+1]);
					break;
				case 'w':
					n_h = n_w = atoi(argv[i+1]);
					break;
				case 'B':
					b_draw_current_time = true;
					break;
				case 'b':
					g_n_pointsize = atoi(argv[i+1]);
					break;
				case 'A':
					colors_filename = argv[i+1];
					break;
				case 'q':
					g_f_norm_dist = atof(argv[i+1]);
					break;
				case 'U':
					b_use_paging = true;
					g_n_paging_iterations = atoi(argv[i+1]);
					break;
				case 'g':
					b_demo_data = false;
					g_b_useGraph = true;
					g_b_useDistance = true;
					b_use_paging = true;
					graph_input_filename = argv[i+1];
					break;
				case 'm':
					b_demo_data = false;
					g_b_useDistance = true;
					b_use_paging = true;
					distance_filename = argv[i+1];
					break;
				case 'E':
					b_edge_input = true;
					b_demo_data = false;
					graph_input_filename = argv[i+1];
					break;
				case 'J':
					b_adj_input = true;
					b_demo_data = false;
					//graph_input_filename = argv[i+1];
					break;
				case 'G':
					b_graph_input = true;
					b_demo_data = false;
					graph_input_filename = argv[i+1];
					break;
				case 'x':	// max number of nonzeros in sparse vec file
					n_max_sparse_entries = atoi(argv[i+1]);
					break;
				case 'X':   // sparse vec filename
					sparse_vector_filename = argv[i+1];
					b_sparse_vector_input = true;
					b_demo_data = false;
					break;
				case 'R':	// convert CSV to mat file
					g_b_convertCSV = true;
					output_mat_name = argv[i+1];
					break;
				case 'P':	// output PPM
					g_b_outputPPM = true;
					output_PPM_name = argv[i+1];
					break;
				case 'W':	// initial configuration
					g_b_useWeighting = true;
					g_f_minWeightDist = atof( argv[i+1] );
					break;
				case 'L':	// initial configuration
					g_b_outputDemoData = true;
					output_demodata_filename = argv[i+1];
					break;
				case 'K':	// initial configuration
					g_b_useSwissRoll = true;
					break;
				case 'Q':	// initial configuration
					g_b_oneTimeStress = true;
					break;
				case 'I':	// initial configuration
					input_config_filename = argv[i+1];
					g_b_initialConfig = true;
					break;
				case 'N':	// noise magnitude
					d_noise_magnitude = atof( argv[i+1] );
					break;
				case 'O':	// noise dimensions
					n_noise_dimensions = atoi( argv[i+1] );
					break;
				case 'M':
					b_demo_data = false;
					g_b_useDistance = true;
					distance_filename = argv[i+1];
					break;
				case 'D':
					g_b_draw = false;
					break;
				case 'T':
					g_n_totalcount = atoi( argv[i+1] );
					break;
				case 'S':
					g_b_useStress = true;
					break;
				case 'V':
					g_b_useVel = true;
					break;
				case 'f':
					g_b_outputStress = true;
					stress_filename = argv[i+1];
					break;
				case 'd':
					b_output_debug = true;
					break;
				case 'p':
					sprintf(g_shader_path, "%s", argv[i+1] );
					if( g_shader_path[strlen(g_shader_path)-1] != '\\' ) {
						sprintf(g_shader_path, "%s\\", argv[i+1] );
					}
					break;
				case 'c':
					g_n_vcycle_count = atoi( argv[i+1] );
					break;
				case 'k':
					g_n_skipframes = atoi( argv[i+1] );
					if( g_n_skipframes < 0 )
						g_b_draw = false;
					break;
				case 't':
					g_n_inner_iters_count = atoi( argv[i+1] );
					break;
				case 'i':
					b_demo_data = false;
					input_filename = argv[i+1];
					break;
				case 'o':
					output_filename = argv[i+1];
					break;
				case 'e':
					n_embed_dims = atoi( argv[i+1] );
					if( n_embed_dims > n_dims ) {
						printf("\nERROR:  Embedding dimensionality must be less than or equal to sample dimensionality." );
						exit( 0 );
					}
					break;
				case 'v':
					n_vmax = atoi( argv[i+1] );
					if( n_vmax < 1 ) {
						printf("\nERROR:  V set must have cardinality of at least 1." );
						exit( 0 );
					}
					break;
				case 's':
					n_smax = atoi( argv[i+1] );
					if( n_smax < 1 ) {
						printf("\nERROR:  S set must have cardinality of at least 1." );
						exit( 0 );
					}
					break;
				case 'n':
					g_num_points = atoi( argv[i+1] );
					break;
			}
		}
		i++;
	}

	if( g_b_convertCSV ) {
		convert_to_mat( input_filename, output_mat_name.c_str( ) );
		exit( 0 );
	}

	// load data into main memory

	if( b_output_debug )
		printf("Loading data ...");
	if( b_demo_data ) {
		if( g_b_useSwissRoll ) {
			data = swissRoll( g_Pyramid, g_num_points );
		}
		else {
			data = demoData( g_Pyramid, g_num_points, d_noise_magnitude, n_noise_dimensions );
		}
	}
	else if( g_b_useDistance ) {
		if( !b_use_paging ) {
			data = readDistances( g_Pyramid, distance_filename.c_str( ) );
		}
		else if( g_b_useGraph ) {
			data = setup_feeder( g_Pyramid, graph_input_filename.c_str(), 1 );
		}
		else{
			data = setup_feeder( g_Pyramid, distance_filename.c_str( ), 0 );
		}
	}
	else if( b_graph_input ) {
		procgraph( graph_input_filename.c_str( ), output_filename.c_str() );		
		exit( 0 );
	}
	else if( b_sparse_vector_input ) {
		if( n_max_sparse_entries ) {
			n_dims = n_max_sparse_entries;
			loadVecData( sparse_vector_filename.c_str(), g_Pyramid, &vec_indices, &data, n_max_sparse_entries, g_f_file_percent );
		}
		else {
			printf("ERROR: must specify largest number of nonzero entries in the dataset with -x option\n");
			exit( 0 );
		}
	}
	else {
		data = readCSVData( g_Pyramid, input_filename, g_f_file_percent );
	}

	// handle IO error
	if( data == NULL ) {
		printf("\nERROR:  Cannot read file %s\n", argv[1] );
		exit( 0 );
	}

	// output data if requested
	if( g_b_outputDemoData ) {
		outputDemoData( g_Pyramid, data, output_demodata_filename.c_str( ) );
		exit( 0 );
	}

	if( b_output_debug && ! g_b_useDistance )
		printf("done. \n\t%d points with %d dimensions. \n\tAllocated texture with dimensions %d x %d \n", g_Pyramid[0].n_points, n_dims, g_Pyramid[0].t_pts->width, g_Pyramid[0].t_pts->height);

}

int main(int argc, char* argv[])
{
	sprintf(g_shader_path, "");

	//if( argc < 2 ) { 
	//	usage( argv[ 0 ] );
	//}

	//srand ( 1 );
	srand ( time(NULL) );
	
	// construct base tier of pyramid 
	g_Pyramid = (Tier *) malloc( sizeof( Tier ) );
	initTier( g_Pyramid );
	
	proc_command_args( argc, (char **)argv );

	// construct a results buffer	
	if( g_b_initialConfig )
		readEmbed( g_Pyramid, input_config_filename.c_str( ) );
	else
		initEmbed( g_Pyramid );

	// allocate our displacement window 

	displacement_window = (float*)malloc(sizeof(float)*DISPLACEMENT_WINDOW_SIZE*4*g_Pyramid->t_embed->width*g_Pyramid->t_embed->height);
	displacement_bounds = (float*)malloc(sizeof(float)*DISPLACEMENT_WINDOW_SIZE*4);

	// setup gl
	initGL(argc,argv);

	g_Pyramid[0].b_update_s = false;

	// begin initial timing
	g_time1 = clock();

	// begin event loop
	glutMainLoop();

	free( displacement_window );
	free( displacement_bounds );
	// clean up data structures
	//cleanup( );

    return EXIT_SUCCESS;
}

