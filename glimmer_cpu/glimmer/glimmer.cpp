// glimmer.cpp : Console program to compute Glimmer CPU MDS on a set of input coordinates
//				
//				Stephen Ingram (sfingram@cs.ubc.ca) 02/08
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "glut.h"

/*
	CONSTANTS
*/
#define MIN_NUM_ARGS	2		// minimum command line arguments
#define SKIP_LINES		2		// number of lines to skip in the input CSV
//#define V_SET_SIZE		4		// number of close neighbors
//#define S_SET_SIZE		4		// number of randomly chosen neighbors
#define V_SET_SIZE		14		// number of close neighbors
#define S_SET_SIZE		10		// number of randomly chosen neighbors
#define USE_GLUT		1		// comment this when timing tests are done
#define MAX_ITERATION	50000	// maximum number of iterations
#define COSCLEN			51		// length of cosc filter
#define EPS				1.e-5f	// termination threshold
#define MIN_SET_SIZE	1000	// recursion termination condition
#define DEC_FACTOR		8		// decimation factor

/*
	FORCE CONSTANTS
*/
#define SIZE_FACTOR		(1.f / ((float)(V_SET_SIZE+S_SET_SIZE)))
#define DAMPING			(0.3f)
#define SPRINGFORCE		(0.7f)
#define FREENESS		(0.85f)
#define DELTATIME		(0.3f)

/*
	DATA STRUCTURES
*/

typedef struct _INDEXTYPE {
	int index;		// index of the other point
	float highd;	// high dimensional distance
	float lowd;		// low dimensional distance
} INDEXTYPE;

typedef struct _VECTYPE {
	int index;
	float value;
} VECTYPE;

/*
	GLOBALS
*/
int g_done = 0;					// controls the movement of points
int g_interpolating=0;			// specifies if we are interpolating yet
int g_current_level=0;			// current level being processed
int g_heir[50];					// handles up to 8^50 points
int g_levels=0;					// stores the point-counts at the associated levels
int iteration=0;				// total number of iterations
int stop_iteration=0;			// total number of iterations since changing levels
int N = 0;						// number of points |V|
int n_original_dims = 2;		// original dimension h of the data (set in loadCSV)
int n_embedding_dims = 2;		// embedding dimensions l
float *g_embed = NULL;			// pointer to embedding coords
float *g_force = NULL;			// pointer to embedding coords' force vectors
float *g_vel = NULL;			// pointer to embedding coords' velocity vectors
float *g_data = NULL;			// pointer to input data coords
VECTYPE *g_vec_data = NULL;		// pointer to the sparse input data coordinates
int g_vec_dims = 0;				// max number of nonzero dims in vec dataset
INDEXTYPE *g_idx = NULL;		// pointer to INDEXTYPE coords
int g_chalmers = 0;				// flag for doing chalmers

// cosc filter 
float cosc[] = {0.f,  -0.00020937301404f,      -0.00083238644375f,      -0.00187445134867f,      -0.003352219513758f,     -0.005284158713234f,     -0.007680040381756f,     -0.010530536243981f,     -0.013798126870435f,     -0.017410416484704f,     -0.021256733995966f,     -0.025188599234624f,     -0.029024272810166f,     -0.032557220569071f,     -0.035567944643756f,     -0.037838297355557f,     -0.039167132882787f,     -0.039385989227318f,     -0.038373445436298f,     -0.036066871845685f,     -0.032470479106137f,     -0.027658859359265f,     -0.02177557557417f,      -0.015026761314847f,     -0.007670107630023f,     0.f,      0.007670107630023f,      0.015026761314847f,      0.02177557557417f,       0.027658859359265f,      0.032470479106137f,      0.036066871845685f,      0.038373445436298f,      0.039385989227318f,      0.039167132882787f,      0.037838297355557f,      0.035567944643756f,      0.032557220569071f,      0.029024272810166f,      0.025188599234624f,      0.021256733995966f,      0.017410416484704f,      0.013798126870435f,      0.010530536243981f,      0.007680040381756f,      0.005284158713234f,      0.003352219513758f,      0.00187445134867f,       0.00083238644375f,       0.00020937301404f,       0.f};
float sstress[MAX_ITERATION];	// sparse stress calculation


/*
	32 bit random number generation (default is 16 bit)
*/
int myrand( ) {

	unsigned int n = (unsigned int)rand();
	unsigned int m = (unsigned int)rand();

	return ((int)((n << 16) + m));
}

/*
	Output the embedding coordinates to a CSV file
*/
void outputCSV( const char *filename, float *embedding ) {

	// open the file
	FILE *fp = NULL;
	if( (fp = fopen( filename, "w" )) == NULL ) {

		printf("ERROR: Can't open points output file %s\n", filename );
		exit( 0 );
	}

	// output header
	fprintf(fp,"X,Y\nDOUBLE,DOUBLE\n"); 

	// output data to file
	for( int i = 0; i < N; i++ ) {
		for( int j = 0; j < n_embedding_dims; j++ ) {
			fprintf(fp, "%f",  embedding[(i*n_embedding_dims)+j] );
			if( j < n_embedding_dims-1) 
				fprintf(fp, "," );
		}
		fprintf(fp,"\n");
	}

	// close the file
	fclose( fp );
}

/*
	Load VECTYPE data from the .vec sparse matrix format
*/
VECTYPE *loadVec( const char *filename ) {

	char line[65536];
	char item[256];
	int i = 0, j = 0, k = 0, m = 0, skip = 0, l = 0;
	bool line_done = 0;
	bool paren_done = 0;
	bool before_comma = 0;
	FILE *fp = 0;

	// open the file 
	fp = fopen(filename, "r");

	// return if there is an error
	if( fp == NULL ) 
		return NULL;

	// count the number of points (for every line)
	N = 0;
	while( fgets( line, 65535, fp ) != NULL ) 
		N++;
	fclose( fp );

	// allocate data 
	g_vec_data = (VECTYPE *)calloc(N*g_vec_dims, sizeof(VECTYPE));

	// read values into 
	fp = fopen(filename, "r");
	int line_num = 0;
	while( fgets( line, 65535, fp ) != NULL ) {

		line_done = false;
		i = 0;
		j = 0;
		while( !line_done ) {
			paren_done = false;
			if( line[i++] == '(' ) {
				while( !paren_done ) {
					if( line[i] == ',' ) {
						item[j] = '\0';
						g_vec_data[k++].index = atoi( item );
						j=0;
					}
					else if( line[i] == ')' ) {
						item[j] = '\0';
						g_vec_data[m++].value = atof( item );
						j=0;
						paren_done = true;
					}
					else if( line[i] != '(' && line[i] != ' ' ) {
							item[j++]=line[i];
					}
					i++;
				}
			}
			if( line[i] == '\0' ) {
				line_num++;
				k = g_vec_dims*line_num;
				m = g_vec_dims*line_num;
				line_done = true;
			}
		}
	}

	// permute the data using Knuth shuffle
	VECTYPE *shuffle_temp = (VECTYPE *) malloc( sizeof( VECTYPE ) * g_vec_dims ) ;
	int shuffle_idx = 0;
	for( i = 0; i < N * g_vec_dims; i+=g_vec_dims ) {

		shuffle_idx = i + ( myrand() % (N-(i/g_vec_dims)) )*g_vec_dims;
		for( j = 0; j < g_vec_dims; j++ ) {

			shuffle_temp[j].index			= g_vec_data[i+j].index;
			shuffle_temp[j].value			= g_vec_data[i+j].value;
			g_vec_data[i+j].index			= g_vec_data[shuffle_idx+j].index;
			g_vec_data[i+j].value			= g_vec_data[shuffle_idx+j].value;
			g_vec_data[shuffle_idx+j].index	= shuffle_temp[j].index;
			g_vec_data[shuffle_idx+j].value	= shuffle_temp[j].value;
		}		
	}
	free(shuffle_temp);

	return g_vec_data;
}

/*
	Load a CSV file to an array of floats
*/
float *loadCSV( const char *filename ) {

	char line[65536];	// line of input buffer
	char item[512];		// single number string
	float *data = NULL;	// output data

	// open the file 
	FILE *fp = fopen( filename, "r" );
	if( fp == NULL ) {
		printf( "ERROR cannot open %s\n", filename );
		exit( 0 );
	}

	// get dataset statistics
	N = 0;
	n_original_dims = 0;
	while( fgets( line, 65535, fp ) != NULL ) {

		// count the number of points (for every line)
		N++;

		// count the number of dimensions (once)
		if( n_original_dims == 0 && N > SKIP_LINES) {
			int i = 0;
			while( line[i] != '\0' ) {
				if( line[i] == ',' ) {
					n_original_dims++;
				}
				i++;
			}
			n_original_dims++;
		}
	}
	fclose( fp );
	N -= SKIP_LINES;

	// allocate our data buffer	
	data = (float*)malloc(sizeof(float)*N*n_original_dims);

	// read the data into the buffer
	fp = fopen(filename, "r");
	int skip = 0;
	int k = 0;
	while( fgets( line, 65535, fp ) != NULL ) {

		int done = 0;
		int i = 0;
		int j = 0;
		while( !done ) {

			// skip the introductory lines
			if( skip++ < SKIP_LINES ) {
			
				done = 1;
			}
			else {

				// parse character data
				if( line[i] == ',' ) {
			
					item[j] = '\0';
					data[k++] = (float) atof( item );
					j = 0;
				}
				else if( line[i] == '\n' || line[i] == '\0' ) {

					item[j] = '\0';
					data[k++] = (float) atof( item );
					done++;
				}
				else if( line[i] != ' ' ) {

					item[j++] = line[i];
				}
				i++;
			}
		}
	}

	//// normalize the data

	float *max_vals = (float *) malloc( sizeof( float ) * n_original_dims );
	float *min_vals = (float *) malloc( sizeof( float ) * n_original_dims );
	for( int i = 0; i < n_original_dims; i++ ) {
		max_vals[ i ] = 0.f;
		min_vals[ i ] = 10000.0f;
	}
	k = 0;
	for( int i = 0; i < N; i++ ) {		
		for( int j = 0; j < n_original_dims; j++ ) {
			if( data[i*(n_original_dims)+j] > max_vals[j] ) {
				max_vals[j] = data[i*(n_original_dims)+j];
			}
			if( data[i*(n_original_dims)+j] < min_vals[j] ) {
				min_vals[j] = data[i*(n_original_dims)+j];					
			}
		}
	}
	for( int i = 0; i < n_original_dims; i++ ) {
		max_vals[ i ] -= min_vals[ i ];
	}
	for( int i = 0; i < N; i++ ) {		
		for( int j = 0; j < n_original_dims; j++ ) {
			if( (max_vals[j] - min_vals[j]) < 0.0001f ) {
				data[i*(n_original_dims)+j] = 0.f;
			}
			else {
				data[i*(n_original_dims)+j] = 
					(data[i*(n_original_dims)+j] - min_vals[j])/max_vals[j];
				if( !_finite( data[i*(n_original_dims)+j]) ) 
					data[i*(n_original_dims)+j] = 0.f;
			}
		}
	}
	free( max_vals );
	free( min_vals );

	// permute the data using Knuth shuffle
	float *shuffle_temp = (float *) malloc( sizeof( float ) * n_original_dims ) ;
	int shuffle_idx = 0;
	for( int i = 0; i < N*n_original_dims; i+=n_original_dims ) {

		shuffle_idx = i + ( myrand() % (N-(i/n_original_dims)) )*n_original_dims;
		for( int j = 0; j < n_original_dims; j++ ) {	// swap
		
			shuffle_temp[j]=data[i+j];
			data[i+j] = data[shuffle_idx+j];
			data[shuffle_idx+j] = shuffle_temp[j];
		}		
	}
	free(shuffle_temp);

	return data;
}

/*
	distance and index comparison functions for qsort
*/
int distcomp( const void *a, const void *b ) {

	const INDEXTYPE *da = (const INDEXTYPE *)a;
	const INDEXTYPE *db = (const INDEXTYPE *)b;
	if(da->highd == db->highd)
		return 0;
	return (da->highd - db->highd)<0.f?-1:1;
}
int idxcomp( const void *a, const void *b ) {

	const INDEXTYPE *da = (const INDEXTYPE *)a;
	const INDEXTYPE *db = (const INDEXTYPE *)b;
	return (int)(da->index - db->index);
}

float max( float a, float b) {

	return (a < b)?b:a;
}
float min( float a, float b) {

	return (a < b)?a:b;
}


/*
	Sparse Stress Termination Condition
*/
int terminate( INDEXTYPE *idx_set, int size ) {

	float numer = 0.f; // sq diff of dists
	float denom = 0.f; // sq dists
	float temp  = 0.f;

	if( iteration > MAX_ITERATION ) {

		return 1;
	}

	// compute sparse stress
	for( int i = 0; i < size; i++ ) {

		for( int j = 0; j < (V_SET_SIZE+S_SET_SIZE); j++ ) {

			temp	= (idx_set[i*(V_SET_SIZE+S_SET_SIZE) + j].highd==1.000f)?0.f:(idx_set[i*(V_SET_SIZE+S_SET_SIZE) + j].highd - idx_set[i*(V_SET_SIZE+S_SET_SIZE) + j].lowd);
			numer	+= temp*temp;
			denom	+= (idx_set[i*(V_SET_SIZE+S_SET_SIZE) + j].highd==1.000f)?0.f:(idx_set[i*(V_SET_SIZE+S_SET_SIZE) + j].highd * idx_set[i*(V_SET_SIZE+S_SET_SIZE) + j].highd);
		}
	}
	sstress[ iteration ] = numer / denom;

	// convolve the signal
	float signal = 0.f;
	if( iteration - stop_iteration > COSCLEN ) {

		for( int i = 0; i < COSCLEN; i++ ) {

			signal += sstress[ (iteration - COSCLEN)+i ] * cosc[ i ];
		}

		if( fabs( signal ) < EPS ) {

			stop_iteration = iteration;
			return 1;
		}
	}

	return 0;
}

/*
	calculate the cosine distance between two points in g_vec_data
*/
float calc_cos_dist( int p1, int p2 ) {

	float dot = 0.f;

	for( int i = 0; i < g_vec_dims; i++ ) {
		for( int j = 0; j < g_vec_dims; j++ ) {

			if( g_vec_data[p1*g_vec_dims+i].index == g_vec_data[p2*g_vec_dims+j].index ) {

				dot += g_vec_data[p1*g_vec_dims+i].value * g_vec_data[p2*g_vec_dims+j].value;
			}
		}
	}

	return (1.f - dot)*(1.f - dot);
}

/*
	Compute Chalmers' an iteration of force directed simulation on subset of size 'size' holding fixedsize fixed
*/
void force_directed( int size, int fixedsize ) {

	// initialize index sets
	if( iteration == stop_iteration ) {

		for( int i = 0; i < size; i++ ) {

			for( int j = 0; j < V_SET_SIZE; j++ ) {

				g_idx[i*(V_SET_SIZE+S_SET_SIZE) + j ].index = myrand()%(g_interpolating?fixedsize:size);
			}
		}
	}

	// perform the force simulation iteration
	float *dir_vec		= (float *)malloc( sizeof(float) * n_embedding_dims );
	float *relvel_vec	= (float *)malloc( sizeof(float) * n_embedding_dims );
	float diff			= 0.f;
	float norm			= 0.f;
	float lo			= 0.f;
	float hi			= 0.f;
	
	// compute new forces for each point
	for( int i = fixedsize; i < size; i++ ) {

		for( int j = 0; j < V_SET_SIZE+S_SET_SIZE; j++ ) {

			// update the S set with random entries
			if( j >= V_SET_SIZE ) {

				g_idx[i*(V_SET_SIZE+S_SET_SIZE)+j].index = myrand()%(g_interpolating?fixedsize:size);
			}

			// calculate high dimensional distances
			int idx = g_idx[i*(V_SET_SIZE+S_SET_SIZE)+j].index;
			if( g_vec_dims ) {

				g_idx[i*(V_SET_SIZE+S_SET_SIZE)+j].highd = calc_cos_dist( idx, i );
			}
			else {
				 
				hi = 0.f;
				for( int k = 0; k < n_original_dims; k++ ) {
			
					norm = (g_data[idx*n_original_dims+k] - g_data[i*n_original_dims+k]);
					hi += norm*norm;
				}
				g_idx[i*(V_SET_SIZE+S_SET_SIZE)+j].highd=(float)sqrt(hi);
			}
		}

		// sort index set by index
		qsort(&(g_idx[i*(V_SET_SIZE+S_SET_SIZE)]), (V_SET_SIZE+S_SET_SIZE), sizeof(INDEXTYPE), idxcomp );

		// mark duplicates (with 1000)
		for( int j = 1; j < V_SET_SIZE+S_SET_SIZE; j++ ) {

			if( g_idx[i*(V_SET_SIZE+S_SET_SIZE)+j].index==g_idx[i*(V_SET_SIZE+S_SET_SIZE)+j-1].index )
				g_idx[i*(V_SET_SIZE+S_SET_SIZE)+j].highd=1000.f;
		}

		// sort index set by distance
		qsort(&(g_idx[i*(V_SET_SIZE+S_SET_SIZE)]), (V_SET_SIZE+S_SET_SIZE), sizeof(INDEXTYPE), distcomp );

		// move the point
		for( int j = 0; j < (V_SET_SIZE+S_SET_SIZE); j++ ) {

			// get a reference to the other point in the index set
			int idx = g_idx[i*(V_SET_SIZE+S_SET_SIZE)+j].index;
			
			norm = 0.f;
			for( int k = 0; k < n_embedding_dims; k++ ) {

				// calculate the direction vector
				dir_vec[k] =  g_embed[idx*n_embedding_dims+k] - g_embed[i*n_embedding_dims+k];
				norm += dir_vec[k]*dir_vec[k];
			}
			norm = sqrt( norm );
			g_idx[i*(V_SET_SIZE+S_SET_SIZE)+j].lowd = norm;
			if( norm > 1.e-6 && g_idx[i*(V_SET_SIZE+S_SET_SIZE)+j].highd!=1000.f ) {		// check for zero norm or mark

				// normalize direction vector
				for( int k = 0; k < n_embedding_dims; k++ ) {

					dir_vec[k] /= norm;
				}

				// calculate relative velocity
				for( int k = 0; k < n_embedding_dims; k++ ) {
					relvel_vec[k] = g_vel[idx*n_embedding_dims+k] - g_vel[i*n_embedding_dims+k];
				}

				// calculate difference between lo and hi distances
				lo = g_idx[i*(V_SET_SIZE+S_SET_SIZE)+j].lowd;	
				hi = g_idx[i*(V_SET_SIZE+S_SET_SIZE)+j].highd;
				diff = (lo - hi) * SPRINGFORCE;					
				
				// compute damping value
				norm = 0.f;
				for( int k = 0; k < n_embedding_dims; k++ ) {
					
					norm += dir_vec[k]*relvel_vec[k];
				}
				diff += norm*DAMPING;
				
				// accumulate the force
				for( int k = 0; k < n_embedding_dims; k++ ) {
					
					g_force[i*n_embedding_dims+k] += dir_vec[k]*diff;
				}
			}
		}

		// scale the force by the size factor
		for( int k = 0; k < n_embedding_dims; k++ ) {
			
			g_force[i*n_embedding_dims+k] *= SIZE_FACTOR;
		}
	}

	// compute new velocities for each point with Euler integration
	for( int i = fixedsize; i < size; i++ ) {

		for( int k = 0; k < n_embedding_dims; k++ ) {
		
			float foo = g_vel[i*n_embedding_dims+k];
			float bar = foo + g_force[i*n_embedding_dims+k]*DELTATIME;
			float baz = bar * FREENESS;
			g_vel[i*n_embedding_dims+k] = max( min(baz, 2.0 ), -2.0 );
		}
	}

	// compute new positions for each point with Euler integration
	for( int i = fixedsize; i < size; i++ ) {
		for( int k = 0; k < n_embedding_dims; k++ ) {
		
			g_embed[i*n_embedding_dims+k] += g_vel[i*n_embedding_dims+k]*DELTATIME;
		}
	}

	// clean up memory allocation
	free(dir_vec);
	free(relvel_vec);
}

/*
	draw points using opengl
*/
void draw_embedding( void ) {

	glClear( GL_COLOR_BUFFER_BIT );

	if( !g_chalmers ) {
		if( g_current_level < g_levels-1 ) {

			glPointSize(2);
			glColor3f(1.f,0.f,0.f);
			glBegin( GL_POINTS );
			// loop through the points from the previous level
			for( int i = 0; i < g_heir[g_current_level+1]; i++ ) {

				glVertex2f(g_embed[i*n_embedding_dims],g_embed[i*n_embedding_dims+1]);
			}
			glEnd();
		}

		// loop through the newly added points
		glPointSize(1);
		glColor3f(1.f,1.f,1.f);
		glBegin( GL_POINTS );
		for( int i = (g_current_level < g_levels-1)?g_heir[g_current_level+1]:0; i < g_heir[g_current_level]; i++ ) {

			glVertex2f(g_embed[i*n_embedding_dims],g_embed[i*n_embedding_dims+1]);
		}
		glEnd();
	}
	else {
		// loop through the points
		glPointSize(1);
		glColor3f(1.f,1.f,1.f);
		glBegin( GL_POINTS );
		for( int i = 0; i < N; i++ ) {

			glVertex2f(g_embed[i*n_embedding_dims],g_embed[i*n_embedding_dims+1]);
		}
		glEnd();
	}
	glFlush(); // flush that buffer
}


/*
	init embedding to a random initialization in (-1,1) x (-1,1)
*/
void init_embedding( float *embedding ) {
	
	for( int i = 0; i < N; i++ ) {
		for( int j = 0; j < n_embedding_dims; j++ ) {
			embedding[i*(n_embedding_dims)+j]=((float)(rand()%10000)/10000.f)-0.5f;
		}
	}
}

/*
	Just redraw the screen
*/
void idle( void ) {

	glutPostRedisplay();
}

/*
	GLUT display function 
*/
void display( void ) {

	if( ! g_chalmers ) {
		if( !g_done ) {

			// move the points
			if( g_interpolating )
				force_directed( g_heir[ g_current_level ], g_heir[ g_current_level+1 ] );
			else
				force_directed( g_heir[ g_current_level ], 0 );

			// check the termination condition

			if( terminate( g_idx, g_heir[g_current_level] ) ) {
		
				if( g_interpolating ) {

					g_interpolating = 0;
				}
				else {

					g_current_level--; // move to the next level down
					g_interpolating = 1;

					// check if the algorithm is complete (no more levels)
					if( g_current_level < 0 ) {

						g_done = 1;
					}
				}
			}

			iteration++;	// increment the current iteration count			
		}
	}
	else {
		if( g_chalmers++ < N ) {
			force_directed( N, 0 );
		}
	}

	draw_embedding();
	glutSwapBuffers();
}


/*
	GLUT keyboard function
*/
void kbfunc( unsigned char key, int x, int y ) {

	switch( key ) {
		case 'q':
		case 'Q':
		case '\27':
			exit( 0 );
			break;
		default:
			break;
	}

}

/*
	Initialize GLUT, GLEW, etc.
*/
void initGL(int argc, char **argv) {

	glutInit ( &argc, argv );
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB );
	glutInitWindowSize(500, 500);
    glutCreateWindow("Glimmer CPU");  
	glutDisplayFunc( display );
	glutIdleFunc( idle );
	glutKeyboardFunc( kbfunc );

	glClearColor( 0.f,0.f,0.f,1.f );
	glColor3f( 1.f, 1.f, 1.f );
}

/*
	computes the input level heirarchy and size
*/
int fill_level_count( int input, int *h ) {

	static int levels = 0;
	//printf("h[%d]=%d\n",levels,input);
	h[levels]=input;
	levels++;
	if( input <= MIN_SET_SIZE )
		return levels;
	return fill_level_count( input / DEC_FACTOR, h );
}

/*
	main function
*/
int main(int argc, char* argv[])
//int _tmain(int argc, _TCHAR* argv[])
{

	// check command line arguments
	if( argc < MIN_NUM_ARGS ) {
		printf("usage:  %s <inputfile> <outputfile> <type>", argv[0]);
		exit( 0 );
	}

	float *data = NULL;
	if( !strcmp( argv[3], "csv" ) ) {
		
		// load input CSV file
		data = loadCSV((char *)argv[1]);
		g_data = data;
	}
	else if( !strcmp( argv[3], "vec" ) ) {

		g_vec_dims = atoi( (char *)argv[2] );
		g_vec_data = loadVec( (char *)argv[1] );
	}
	else {

		N			= atoi(argv[3]);
		data		= (float *)malloc( sizeof(float)*8*N );
		g_data		= data;
		int width	= (int) ceil( sqrt( (float) N ) );
		float coeff = (1.0f / ( (float) width) );
		n_original_dims = 8;

		for( int i = 0; i < N; i++ ) {

			for( int j = 2; j < n_original_dims; j++ ) {
			
				data[i*n_original_dims+j] = 0.0;
			}
			data[i*n_original_dims]		= (float)(i % width) * coeff;
			data[i*n_original_dims+1]	= (float)(i / width) * coeff;
		}

		float *shuffle_temp = (float *) malloc( sizeof( float ) * n_original_dims ) ;
		int shuffle_idx = 0;
		for( int i = 0; i < N*n_original_dims; i+=n_original_dims ) {

			shuffle_idx = i + ( myrand() % (N-(i/n_original_dims)) )*n_original_dims;
			for( int j = 0; j < n_original_dims; j++ ) {	// swap
			
				shuffle_temp[j]=data[i+j];
				data[i+j] = data[shuffle_idx+j];
				data[shuffle_idx+j] = shuffle_temp[j];
			}		
		}
		free(shuffle_temp);
	}

	// initialize opengl and glut
	initGL( argc, (char **)argv );

	// begin timing -------------------------------------BEGIN TIMING
	clock_t start_time1 = clock();

	// allocate embedding and associated data structures
	g_levels = fill_level_count( N, g_heir );
	g_current_level = g_levels-1;
	g_embed	= (float *)malloc(sizeof(float)*n_embedding_dims*N);
	g_vel	= (float *)calloc(n_embedding_dims*N,sizeof(float));
	g_force	= (float *)calloc(n_embedding_dims*N,sizeof(float));
	g_idx	= (INDEXTYPE *)malloc(sizeof(INDEXTYPE)*N*(V_SET_SIZE+S_SET_SIZE));



	// initialize embedding
	init_embedding( g_embed );

	if( argc > 4 ) {
		if( !strcmp( argv[4], "chalm" ) ) {
			g_chalmers = 1;
			for( int i = 0; i < N; i++ ) {
				force_directed( N, 0 );
			}
		}
	} 
	else {

		while( !g_done ) {

			// move the points
			if( g_interpolating )
				force_directed( g_heir[ g_current_level ], g_heir[ g_current_level+1 ] );
			else
				force_directed( g_heir[ g_current_level ], 0 );

			// check the termination condition

			if( terminate( g_idx, g_heir[g_current_level] ) ) {
		
				if( g_interpolating ) {

					g_interpolating = 0;
				}
				else {

					g_current_level--; // move to the next level down
					g_interpolating = 1;

					// check if the algorithm is complete (no more levels)
					if( g_current_level < 0 ) {

						g_done = 1;
					}
				}
			}

			iteration++;	// increment the current iteration count			
		}
	}

	clock_t start_time2 = clock();

	printf("%d %d", N, (start_time2-start_time1));

	if (strcmp(argv[2],"NONE")) {
		outputCSV(argv[2],g_embed);
	}

	// begin event loop
	glutMainLoop();

	if( argc > 5 ) {

		double numer = 0.;	
		double denom = 0.;

		// calculate stress
		for( int i = 0; i < N; i++ ) {
			for( int j = i; j < N; j++ ) {
				
				// calculate d
				double d = 0.;
				double temp = 0.;
				for( int k = 0; k < n_embedding_dims; k++ ) {
					temp = g_embed[i*n_embedding_dims+k]-g_embed[j*n_embedding_dims+k];
					d += temp*temp;
				}
				d = sqrt(d);

				// calcualte delta
				float delta;
				if( g_vec_dims ) {
					delta = calc_cos_dist( i,j );
				}
				else {
					temp = 0.;
					delta = 0.;
					for( int k = 0; k < n_original_dims; k++ ) {
						temp = g_data[i*n_original_dims+k]-g_data[j*n_original_dims+k];
						delta += temp*temp;
					}
					delta = sqrt(delta);
				}

				temp	= d-delta;
				numer	+= temp*temp;
				denom	+= delta*delta;
			}
		}
		printf(" %lf", numer/denom );
	}

	printf("\n");


	// quit
	return 0;
}

