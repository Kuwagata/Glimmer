//#include "stdafx.h"
#include <time.h>
#include <string>
#include <cmath>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

extern int n_points;
extern int n_dims;
extern int n_embed_dims;
extern int n_vmax;
extern int n_smax;
extern float *randr_killme;
extern float *randr;
extern float *data;
extern float *result;

int compare (const void * a, const void * b)
{
  return ( *(float*)a - *(float*)b );
}

/*
	Run an iteration of Chalmers on the CPU for profiling purposes
*/
void cpuProfile ( int iters ) {
	clock_t time1,time2;
	time1 = clock();
	int totalk = (n_points*(((int)ceil(((double)n_vmax) / 4.0 )) + ((int)ceil(((double)n_smax) / 4.0 ))));
	int binsper = (((int)ceil(((double)n_vmax) / 4.0 )) + ((int)ceil(((double)n_smax) / 4.0 )));
	float *d_dists_killme = (float *) malloc( sizeof(float) * totalk );
	float *g_dists_killme = (float *) malloc( sizeof(float) * totalk );
	for( int foo = 0; foo < iters; foo++ ) {
		int baz = rand() % totalk;
		int chuckle=0;

		// get some random samples
		for( int giggles = baz; giggles < totalk; giggles++) 
			randr_killme[chuckle++] = randr[giggles];
		for( int giggles = 0; giggles < baz; giggles++) 
			randr_killme[chuckle++] = randr[giggles];

		// calculate interpoint distances
		for( int i = 0; i < totalk; i++ ) {
			float sum = 0.f;
			for( int j = 0; j < n_dims; j++ ) {
				sum += (float)pow((float)(data[ (int)randr_killme[ i ] * n_dims + j]-data[ (i/binsper) * n_dims + j]),(int)2);			
			}
			d_dists_killme[ i ] = sum;
		}
		for( int i = 0; i < totalk; i++ ) {
			float sum = 0.f;
			for( int j = 0; j < n_embed_dims; j++ ) {
				sum += (float)pow((float)(result[ (int)randr_killme[ i ] * n_embed_dims + j]-result[ (i/binsper) * n_embed_dims + j]),(int)2);			
			}
			g_dists_killme[ i ] = sum;
		}
		for( int i = 0; i < totalk; i+=binsper )  {
			qsort( &(d_dists_killme[i]), binsper, sizeof(float), compare);
			qsort( &(g_dists_killme[i]), binsper, sizeof(float), compare);
		}
	}
	time2 = clock();
	printf("done in %d milliseconds.", (time2 - time1));
	free( g_dists_killme );
	free( d_dists_killme );
}
