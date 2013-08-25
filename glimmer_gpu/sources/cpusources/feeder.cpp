//#include "stdafx.h" //Breaks in Linux
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <cstring>
#include <stdio.h>
#include "feeder.h"
#include "data.h"
#include <cmath>
#include "graph.h"

extern float g_f_norm_dist;
//extern GlimmerGraph *glimmerGraph;
extern vtx_data *glimmerGraph;
Feeder g_feeder;
extern int g_n_vcycle_count;
extern int g_n_paging_iterations;
extern int g_n_framecount;
extern int n_smax;
extern int n_vmax;

/*

	Initializes feeder data structure
		1.  Reads in distance matrix
		2.  Allocates and sizes empty page buffer storage texture

*/
float *setup_feeder( Tier *tier, const char *filename, int type ) {

	int i = 0, j = 0, k = 0;
	char line[65535];
	char item[65535];
	int done = 0;
	int leftover = 0;
	g_feeder.page = NULL;
	g_feeder.type = type;

	if( type == 0 ) {

		// load distance matrix

		FILE *fp = fopen(filename, "r");

		// return if there is an error

		if( fp == NULL ) 
			return NULL;

		// get statistics
		tier->n_points = 0;
		while( fgets( line, 65535, fp ) != NULL ) {
			tier->n_points++;
		}
		fclose( fp );

		g_feeder.N4 = (int)ceil( ((double)tier->n_points) / 4.0 );

		// allocate our data buffers and size our texture
		tier->t_pts->height = (int) ceil( sqrt((double)(tier->n_points * g_n_paging_iterations)) );
		tier->t_pts->width = tier->t_pts->height;
		g_feeder.page = (float*) calloc( (tier->t_pts->height) * (tier->t_pts->width) * 4, sizeof( float ) );
		g_feeder.D = (float*) malloc( sizeof( float ) * ((tier->n_points * (tier->n_points+1)) / 2 ) );
		fp = fopen(filename, "r");
		while( fgets( line, 65535, fp ) != NULL ) {
			i=0;
			j=0;
			done = 0;
			while( !done ) {
				if( line[i] == ' ' ) {
					item[j] = '\0';
					g_feeder.D[k++] = (float) atof( item );
					j = 0;
				}
				else if( line[i] == '\n' || line[i] == '\0' ) {
					item[j] = '\0';
					if( strlen( item ) )
						g_feeder.D[k++] = (float) atof( item );
					done = 1;
				}
				else if( line[i] != ' ' ) {
					item[j++] = line[i];
				}
				i++;
			}
		}

		// normalize the distance matrix

		float max_dist = 0.f;
		for( i = 0; i < ((tier->t_pts->height) * (tier->t_pts->width) * 4); i++ ) {
			if( g_feeder.D[i] > max_dist )
				max_dist = g_feeder.D[i];
		}
		for( i = 0; i < ((tier->t_pts->height) * (tier->t_pts->width) * 4); i++ ) {
			g_feeder.D[i] /= max_dist;
		}
	}
	else if( type == 1 ) {

		// load input graph

		//glimmerGraph = load_edge_file( filename );
		glimmerGraph = load_vtx_data( filename, &(tier->n_points) );

		//for( int i = 0; i < 20; i++ ) {
		//	for( int j = 0; j < glimmerGraph->numnodes; j++ ) {
		//		printf("%d ", bfs( glimmerGraph, i, j ) );
		//		fflush(stdout);
		//		//for( int k = 0; k < glimmerGraph->numnodes; k++ ) {
		//		//	printf("[%d]",glimmerGraph->nodes[k].mark);
		//		//}
		//	}
		//	printf("\n");
		//}
		//exit( 0 );
		//printf("distance between first two nodes == %d", bfs( glimmerGraph, 0, 1 ));

		//tier->n_points = glimmerGraph->numnodes;
		
		//g_feeder.N4 = (int)ceil( ((double)tier->n_points) / 4.0 );

		//// allocate our data buffers and size our texture
		//tier->t_pts->height = (int) ceil( sqrt((double)(tier->n_points * g_n_paging_iterations)) );
		tier->t_pts->width = (int) ceil( sqrt((double)(tier->n_points * ((n_vmax+n_smax)/4) ) ) );
		tier->t_pts->height = tier->t_pts->width;
		tier->t_pts->width = (int) ceil( (double)tier->t_pts->height / (double)((n_vmax+n_smax)/4) ) * ((n_vmax+n_smax)/4);
		g_feeder.page = (float*) calloc( (tier->t_pts->height) * (tier->t_pts->width) * 4, sizeof( float ) );
		g_feeder.norm_dist = 1.0/g_f_norm_dist;
		//printf("tier->t_pts = %d x %d\n", tier->t_pts->width, tier->t_pts->height );
	}

	return g_feeder.page;
}

/*
	Fill the feeder page with distances
		1.  Maps from D -> D''
		2.  Copies values for entire page
*/
void feeder_get_page( Tier *tier, int base_iteration, bool b_fix ) {

	int i,j,row,col;

	if( g_feeder.type == 0 ) {
		if( g_n_vcycle_count ) { // do we need to permute the indices
			int itemcount,sw,ssw,bw = tier->t_perm->width;
			int refitemcount;
			if( b_fix ) {
				refitemcount = tier->t_perm->tier_width[tier->level+1]*tier->t_perm->tier_height[tier->level+1];
				itemcount = (tier->level==0)?tier->n_points:(tier->t_perm->tier_width[tier->level]*tier->t_perm->tier_height[tier->level]);
				sw = tier->t_perm->tier_width[tier->level+1];
				ssw = tier->t_perm->tier_width[tier->level];
			}
			else {
				refitemcount = itemcount = (tier->level==0)?tier->n_points:(tier->t_perm->tier_width[tier->level]*tier->t_perm->tier_height[tier->level]);
				ssw = sw = tier->t_perm->tier_width[tier->level];
			}
			for( int iters = 0; iters < g_n_paging_iterations; iters++ ) {		// for each iteration in a page
				for( int x = 0; x < itemcount*4; x++ ) {						// for each element in an iteration
					//if( b_fix && !(x % 4) ) {
					//	printf("\n");
					//}
					i = x/4;													// compute the row/column of D we use
					j = g_feeder.P[ ((g_feeder.P[i]+base_iteration+iters) % g_feeder.N4)*4+(x%4) ];
					i = i % itemcount;			 // constrain indices to random subset
					i = i % ssw + (i / ssw)*bw;
					j = j % refitemcount;
					j = j % sw + (j / sw)*bw;

					//if( b_fix ) printf("[%d %d ", i, j);

					i = (int)tier->perm[ i*4 ];  // match the subset indices to the shuffle permutation
					j = (int)tier->perm[ j*4 ];
					row = maxint( i, j );
					col = minint( i, j );
					g_feeder.page[x+(iters*g_feeder.N4*4*4)]=g_feeder.D[(row*(row+1))/2+col];	// copy it into the page

					//if( b_fix ) printf("%d %d %f] ", i, j, g_feeder.D[(row*(row+1))/2+col] );

				}
				//if( b_fix ) exit( 0 );
			}
		}
		else {
			for( int iters = 0; iters < g_n_paging_iterations; iters++ ) {		// for each iteration in a page
				for( int x = 0; x < tier->n_points*4; x++ ) {						// for each element in an iteration
					i = x/4;													// compute the row/column of D we use
					j = g_feeder.P[ ((g_feeder.P[i]+base_iteration+iters) % g_feeder.N4)*4+(x%4) ];

					//if( !(x % 4) ) {
					//	printf("\n%d %d: ", i, ((g_feeder.P[i]+base_iteration+iters) % g_feeder.N4) );
					//}

					row = maxint( i, j );
					col = minint( i, j );
					g_feeder.page[x+(iters*g_feeder.N4*4*4)]=g_feeder.D[(row*(row+1))/2+col];	// copy it into the page					
					//printf("%d %f ", j, g_feeder.D[(row*(row+1))/2+col] );
				}
				//printf("\n");
				//exit( 0 );
			}
		}
	}
	else if( g_feeder.type == 1 ) {
		if( g_n_vcycle_count ) { // do we need to permute the indices
			int itemcount,sw,ssw,bw = tier->t_perm->width;
			int refitemcount;
			if( b_fix ) {
				refitemcount = tier->t_perm->tier_width[tier->level+1]*tier->t_perm->tier_height[tier->level+1];
				itemcount = (tier->level==0)?tier->n_points:(tier->t_perm->tier_width[tier->level]*tier->t_perm->tier_height[tier->level]);
				sw = tier->t_perm->tier_width[tier->level+1];
				ssw = tier->t_perm->tier_width[tier->level];
			}
			else {
				refitemcount = itemcount = (tier->level==0)?tier->n_points:(tier->t_perm->tier_width[tier->level]*tier->t_perm->tier_height[tier->level]);
				ssw = sw = tier->t_perm->tier_width[tier->level];
			}
			for( int iters = 0; iters < g_n_paging_iterations; iters++ ) {		// for each iteration in a page
				for( int x = 0; x < itemcount*4; x++ ) {						// for each element in an iteration
					i = x/4;													// compute the row/column of D we use
					j = g_feeder.P[ ((g_feeder.P[i]+base_iteration+iters) % g_feeder.N4)*4+(x%4) ];
					i = i % itemcount;			 // constrain indices to random subset
					i = i % ssw + (i / ssw)*bw;
					j = j % refitemcount;
					j = j % sw + (j / sw)*bw;

					i = (int)tier->perm[ i*4 ];  // match the subset indices to the shuffle permutation
					j = (int)tier->perm[ j*4 ];
					row = maxint( i, j );
					col = minint( i, j );
					//g_feeder.page[x+(iters*g_feeder.N4*4*4)]= g_feeder.norm_dist*bfs_glimmer( glimmerGraph, row, col );	// copy it into the page
					//printf("%f\n",g_feeder.page[x+(iters*g_feeder.N4*4*4)]);
				}
			}
		}
		else {
			for( int iters = 0; iters < g_n_paging_iterations; iters++ ) {		// for each iteration in a page
				for( int x = 0; x < tier->n_points*4; x++ ) {						// for each element in an iteration
					i = x/4;													// compute the row/column of D we use
					j = g_feeder.P[ ((g_feeder.P[i]+base_iteration+iters) % g_feeder.N4)*4+(x%4) ];

					row = maxint( i, j );
					col = minint( i, j );
					//g_feeder.page[x+(iters*g_feeder.N4*4*4)]=g_feeder.norm_dist*bfs_glimmer( glimmerGraph, row, col );	// copy it into the page					
					//printf("%f\n",g_feeder.page[x+(iters*g_feeder.N4*4*4)]);
				}
			}
		}
	}
}
