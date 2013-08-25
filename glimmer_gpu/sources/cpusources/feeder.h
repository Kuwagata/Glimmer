#ifndef FEEDER_MDSGPU
#define FEEDER_MDSGPU 0

#include "texture.h"
#include "data.h"

/*
	Data Structures
*/

typedef struct feeder_t {
	int *P;			// permutation
	int N4;				// ceil( tier->n_points / 4 )
	float *D;			// n^2/2 lower triangle of symmetric distance matrix
	int type;			// 0 = raw file, 1 = graph file
	float *page;		// data buffer to store pages on the cpu
	float norm_dist;	// constant to normalize the distance with
} Feeder;


/*
	Function Prototypes
*/

float *setup_feeder( Tier *tier, const char *filename, int type );
void feeder_get_page( Tier *tier, int base_iteration, bool b_fix );

#endif