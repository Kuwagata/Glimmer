#ifndef DATA_MDSGPU
#define DATA_MDSGPU 0

#include "texture.h"
#include <vector>

#define TEST_POINTS		100000
#define SKIP_LINES		2
#define USE_DEMO_DATA	1

#define SPRING_FORCE		0.7
//#define DAMPING_FACTOR		0.9
#define FREENESS			0.85
//#define DELTA_TIME			0.3
#define FOOBIE	

typedef struct mynode {
	int id;
	int numstorage;
	int *edges;
	int numedges;
	int mark;
} GlimmerNode;
typedef struct mygraph {
	GlimmerNode *nodes;
	int numnodes;
} GlimmerGraph;

int maxint( int i, int j );
int minint( int i, int j );
int loadVecData( const char *name, Tier *tier, float **index, float **values, int maxsize, float percent );
int dumpPPM(const char *outname, int width, int height);
void convert_to_mat( const char *inname, const char *outname );
void initZero( Tier *tier );
float *demoData( Tier *tier, int num_points, double noise_magnitude, int noise_dimensions );
float *plainCSVData( const char *name, int width, int height );
float *readCSVData( Tier *tier, const char *name, float percent );
//float *readCSVData( Tier *tier, const char *name );
void initTier( Tier *tier );
void readEmbed( Tier *tier, const char *filename );
void initEmbed( Tier *tier );
void outputPoints ( const char *output_filename );
float *readDistances( Tier *tier, const char *name );
void outputDemoData( Tier *tier, const float *f_data, const char *filename );
float *swissRoll( Tier *tier, int num_points );
//void outputDemoData( const float *f_data, const char *filename );
void procgraph( const char *matrixname, const char *outfilename);
GlimmerGraph *load_edge_file( const char *edgefilename );
int bfs_glimmer( GlimmerGraph *graph, int src_id, int dest_id );
int *select_pivots( GlimmerGraph *graph, float *pivot_distances, int numpivots );
void select_neighbors( GlimmerGraph *graph, int src_id, int num_neighbors, float *distances, float *indices );
#endif