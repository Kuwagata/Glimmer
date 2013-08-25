#include <iostream>
#include <string>
#include <cmath>
#include <time.h>
using std::string;
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <stdio.h>
#include <cstring>
#include "texture.h"
#include "fbo.h"
#include "data.h"
#include "shader.h"
#include "graph.h"

/*
	Imported globals from gltest
*/

extern float g_f_norm_dist;
//extern GlimmerGraph *glimmerGraph;
extern vtx_data *glimmerGraph;
extern float *vec_indices;
extern float *data;
extern float *zero_data;
extern float *result;
extern int n_dims;
extern int n_vmax;
extern int n_smax;
extern GLenum attachmentpoints[];
extern int n_set_min_size;
extern bool b_output_debug;
extern bool b_sparse_vector_input;
extern char g_shader_path[4092];
extern bool g_b_useStress;
extern bool g_b_oneTimeStress;
extern bool g_b_oneTimeStressEnd;
extern bool g_b_useVel;
extern bool g_b_useDistance;
extern bool g_b_useGraph;

Shader sh_copy;
Uniform u_copy;

/*
	Texture Pyramid
*/

Pyramid pyr_center;

/*
	Construct our input, output, and intermediary textures.

	The idea is to keep halving the largest dimension of a texture
	until the number of items it contains falls beneath a threshold.

	We start with the permutation since it is the 'unit' texture and
	then use it to size the coordinates for the other textures.
*/
void sizeTextures( Tier *tier, int decimation_factor ) {

	int width = tier->t_perm->width;
	int height = tier->t_perm->height;
	int coord_count = 1;

	for( coord_count = 1; width * height > n_set_min_size; coord_count++ ) {
		if( width > height ) {
			width /= decimation_factor;
		}		
		else {
			height /= decimation_factor;
		}
	}
	coord_count++;
	tier->n_levels = coord_count-1;
	//tier->n_levels = coord_count;

	tier->t_perm->tier_width = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_perm->tier_height = (int *)malloc( sizeof( int ) * coord_count );
	if( ! g_b_useDistance ) {
		tier->t_pts->tier_width = (int *)malloc( sizeof( int ) * coord_count );
		tier->t_pts->tier_height = (int *)malloc( sizeof( int ) * coord_count );
		if( b_sparse_vector_input ) {
			tier->t_vec_idx->tier_width = (int *)malloc( sizeof( int ) * coord_count );
			tier->t_vec_idx->tier_height = (int *)malloc( sizeof( int ) * coord_count );
		}
	}
	tier->t_reference->tier_width = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_reference->tier_height = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_idx->tier_width = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_idx->tier_height = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_embed->tier_width = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_embed->tier_height = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_d->tier_width = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_d->tier_height = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_diff->tier_width = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_diff->tier_height = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_g->tier_width = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_g->tier_height = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_velocity->tier_width = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_velocity->tier_height = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_force->tier_width = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_force->tier_height = (int *)malloc( sizeof( int ) * coord_count );
	//tier->t_sum->tier_width = (int *)malloc( sizeof( int ) * coord_count );
	//tier->t_sum->tier_height = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_stress->tier_width = (int *)malloc( sizeof( int ) * coord_count );
	tier->t_stress->tier_height = (int *)malloc( sizeof( int ) * coord_count );

	width = tier->t_perm->width;
	height = tier->t_perm->height;
	for( coord_count = 0; coord_count < tier->n_levels; coord_count++ ) {
		
		tier->t_perm->tier_width[ coord_count ] = width;
		tier->t_perm->tier_height[ coord_count ] = height;
		tier->t_reference->tier_width[ coord_count ] = width;
		tier->t_reference->tier_height[ coord_count ] = height;
		if( ! g_b_useDistance ) {
			tier->t_pts->tier_width[ coord_count ] = width * (tier->t_pts->width/tier->t_perm->width);
			tier->t_pts->tier_height[ coord_count ] = height;
			if( b_sparse_vector_input ) {
				tier->t_vec_idx->tier_width[ coord_count ] = width * (tier->t_vec_idx->width/tier->t_perm->width);
				tier->t_vec_idx->tier_height[ coord_count ] = height;
			}
		}
		tier->t_idx->tier_width[ coord_count ] = width * (tier->t_idx->width/tier->t_perm->width);
		tier->t_idx->tier_height[ coord_count ] = height;
		tier->t_embed->tier_width[ coord_count ] = width * (tier->t_embed->width/tier->t_perm->width);
		tier->t_embed->tier_height[ coord_count ] = height;
		tier->t_stress->tier_width[ coord_count ] = width * (tier->t_embed->width/tier->t_perm->width);
		tier->t_stress->tier_height[ coord_count ] = height;
		tier->t_d->tier_width[ coord_count ] = width * (tier->t_d->width/tier->t_perm->width);
		tier->t_d->tier_height[ coord_count ] = height;
		tier->t_diff->tier_width[ coord_count ] = width * (tier->t_diff->width/tier->t_perm->width);
		tier->t_diff->tier_height[ coord_count ] = height;
		tier->t_g->tier_width[ coord_count ] = width * (tier->t_g->width/tier->t_perm->width);
		tier->t_g->tier_height[ coord_count ] = height;
		tier->t_velocity->tier_width[ coord_count ] = width * (tier->t_velocity->width/tier->t_perm->width);
		tier->t_velocity->tier_height[ coord_count ] = height;
		tier->t_force->tier_width[ coord_count ] = width * (tier->t_force->width/tier->t_perm->width);
		tier->t_force->tier_height[ coord_count ] = height;
		//tier->t_sum->tier_width[ coord_count ] = width * (tier->t_sum->width/tier->t_perm->width);
		//tier->t_sum->tier_height[ coord_count ] = height;

		if( width > height ) {
			width /= decimation_factor;
		}		
		else {
			height /= decimation_factor;
		}
	}

	// include top layer of pyramid which actually contains single pixel textures
	// this is used for inducing the driver to optimize the shader code

	height = width = 2;
	tier->t_perm->tier_width[ coord_count ] = width;
	tier->t_perm->tier_height[ coord_count ] = height;
	tier->t_reference->tier_width[ coord_count ] = width;
	tier->t_reference->tier_height[ coord_count ] = height;
	if( ! g_b_useDistance ) {
		tier->t_pts->tier_width[ coord_count ] = width;
		tier->t_pts->tier_height[ coord_count ] = height;
		if( b_sparse_vector_input ) {
			tier->t_vec_idx->tier_width[ coord_count ] = width;
			tier->t_vec_idx->tier_height[ coord_count ] = height;
		}
	}
	tier->t_idx->tier_width[ coord_count ] = width;
	tier->t_idx->tier_height[ coord_count ] = height;
	tier->t_embed->tier_width[ coord_count ] = width;
	tier->t_embed->tier_height[ coord_count ] = height;
	tier->t_stress->tier_width[ coord_count ] = width;
	tier->t_stress->tier_height[ coord_count ] = height;
	tier->t_d->tier_width[ coord_count ] = width;
	tier->t_d->tier_height[ coord_count ] = height;
	tier->t_diff->tier_width[ coord_count ] = width*(tier->t_diff->width/tier->t_perm->width);
	tier->t_diff->tier_height[ coord_count ] = height;
	tier->t_g->tier_width[ coord_count ] = width;
	tier->t_g->tier_height[ coord_count ] = height;
	tier->t_velocity->tier_width[ coord_count ] = width;
	tier->t_velocity->tier_height[ coord_count ] = height;
	tier->t_force->tier_width[ coord_count ] = width;
	tier->t_force->tier_height[ coord_count ] = height;
	//tier->t_sum->tier_width[ coord_count ] = width;
	//tier->t_sum->tier_height[ coord_count ] = height;
}

/*
	Generate textures
*/
void genTextures( Tier *tier ) {

	if( b_output_debug ) {
		printf("Generating...\n");
		printf("\tSample points...");
		printf("%d by %d\n", tier->t_pts->width, tier->t_pts->height);
	}

	int n_dim_bins = (int) ceil( (double) n_dims / (double) 4 );
	int n_set_bins = ((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 ));

	// setup t_pts
	if( ! g_b_useDistance ) {
		tier->t_pts->attachments = 2;
		tier->t_pts->attach_idx = 0;
		buildFBOTexture(tier->t_pts);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_pts->texture_name[ tier->t_pts->attach_idx ]);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_pts->width,tier->t_pts->height,
						GL_RGBA,GL_FLOAT,data);
		if( b_output_debug ) {
			printf("done.\n");
		}
	}
	else if( ! g_b_useGraph ) {
		tier->t_pts->attachments = 1;
		tier->t_pts->attach_idx = 0;
		buildFBOTexture(tier->t_pts);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_pts->texture_name[ tier->t_pts->attach_idx ]);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_pts->width,tier->t_pts->height,
						GL_RGBA,GL_FLOAT,data);
		if( b_output_debug ) {
			printf("done.\n");
		}
	}

	// setup t_vec_idx
	if( b_sparse_vector_input ) {
		tier->t_vec_idx->attachments = 2;
		tier->t_vec_idx->attach_idx = 0;
		buildFBOTexture(tier->t_vec_idx);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_vec_idx->texture_name[ tier->t_vec_idx->attach_idx ]);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_vec_idx->width,tier->t_vec_idx->height,
			GL_RGBA,GL_FLOAT,vec_indices);
	}

	// setup t_embed
	if( b_output_debug ) 
		printf("\tEmbedded points...");
	tier->t_embed->attachments = 2;
	tier->t_embed->attach_idx = 0;
	buildFBOTexture(tier->t_embed);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_embed->texture_name[ tier->t_embed->attach_idx ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_embed->width,tier->t_embed->height,
                    GL_RGBA,GL_FLOAT,result);
	if( b_output_debug ) 
		printf("done.\n");

	// setup t_rand
	if( ! g_b_useGraph ) {
		if( b_output_debug ) 
			printf("\tRandom texture...");
		//tier->t_rand->width = ( tier->t_pts->width / n_dim_bins ) * n_set_bins;
		//tier->t_rand->height = tier->t_pts->height;
		tier->t_rand->attachments = 1;
		tier->t_rand->attach_idx = 0;
		buildFBOTexture(tier->t_rand);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_rand->texture_name[ tier->t_rand->attach_idx ]);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_rand->width,tier->t_rand->height,
						GL_RGBA,GL_FLOAT,tier->randr);
		if( b_output_debug ) 
			printf("done.\n");

		// setup t_perm
		if( b_output_debug ) 
			printf("\tPermutation texture...");
		tier->t_perm->attachments = 2;
		tier->t_perm->attach_idx = 0;
		buildFBOTexture(tier->t_perm);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_perm->texture_name[ tier->t_perm->attach_idx ]);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_perm->width,tier->t_perm->height,
						GL_RGBA,GL_FLOAT,tier->perm);
		if( b_output_debug ) 
			printf("done.\n");

		// setup t_reference
		if( b_output_debug ) 
			printf("\tReference texture...");
		tier->t_reference->width = tier->t_perm->width;
		tier->t_reference->height = tier->t_perm->height;
		tier->t_reference->attachments = 2;
		tier->t_reference->attach_idx = 0;
		buildFBOTexture(tier->t_reference);
		for( int i = 0; i < tier->n_points; i++ ) {
			tier->reference[i*4] = (float)i;
			tier->reference[i*4+1] = 0.f;
			tier->reference[i*4+2] = 0.f;
			tier->reference[i*4+3] = 0.f;
		}
		if( tier->n_points < tier->t_perm->width*tier->t_perm->height ) {
			int offset = tier->t_perm->width*tier->t_perm->height - tier->n_points;
			for( int i = 0; i < offset; i++ ) {
				tier->reference[ (i + tier->n_points) * 4 ] = 0.0f;
				tier->reference[ (i + tier->n_points) * 4 + 1 ] = 0.0f;
				tier->reference[ (i + tier->n_points) * 4 + 2 ] = 0.0f;
				tier->reference[ (i + tier->n_points) * 4 + 3 ] = 0.0f;
			}
		}	
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_reference->texture_name[ tier->t_reference->attach_idx ]);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_reference->width,tier->t_reference->height,
						GL_RGBA,GL_FLOAT,tier->reference);
		if( b_output_debug ) 
			printf("done.\n");
	}

	// setup t_idx
	if( b_output_debug ) 
		printf("\tIndex texture...");
	tier->t_idx->width = tier->t_embed->width * n_set_bins;
	tier->t_idx->height = tier->t_embed->height;
	tier->t_idx->attachments = 2;
	tier->t_idx->attach_idx = 0;
	if( b_output_debug ) 
		printf("%d by %d", tier->t_idx->width, tier->t_idx->height);
	buildFBOTexture(tier->t_idx);
	if( g_b_useGraph ) {	// setup initial graph indices

	}
	if( b_output_debug ) 
		printf("done.\n");

	// setup t_d
	if( b_output_debug ) 
		printf("\tTrue Distance texture...");
	tier->t_d->width = tier->t_embed->width * n_set_bins;
	tier->t_d->height = tier->t_embed->height;
	tier->t_d->attachments = 2;
	tier->t_d->attach_idx = 0;
	buildFBOTexture(tier->t_d);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_d->texture_name[ tier->t_d->attach_idx ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_d->width,tier->t_d->height,
                    GL_RGBA,GL_FLOAT,zero_data);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_d->texture_name[ tier->t_d->attach_idx?0:1 ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_d->width,tier->t_d->height,
                    GL_RGBA,GL_FLOAT,zero_data);
	if( b_output_debug ) 
		printf("done.\n");

	/////
	// if we're using a graph we can skip a lot of work by doing it ahead of time here
	// we can do this now because unlike arbitrary data, graphs give us the topology
	// and we can avoid any search
	////
	if( g_b_useGraph ) {
		
		float *tex_data = (float *)malloc( sizeof( float ) * tier->t_idx->width * tier->t_idx->height * 4 * 2 );
		int tex_offset = tier->t_idx->width * tier->t_idx->height * 4;
		extract_distances( glimmerGraph, tier->n_points, n_vmax, n_smax, tex_data, tex_offset );
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_idx->texture_name[ tier->t_idx->attach_idx ]);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_idx->width,tier->t_idx->height,
						GL_RGBA,GL_FLOAT,tex_data);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_d->texture_name[ tier->t_d->attach_idx ]);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_d->width,tier->t_d->height,
						GL_RGBA,GL_FLOAT,tex_data+tex_offset);

		//outputTexture(tier->t_idx,"idx.txt");
		//outputTexture(tier->t_d,"d.txt");
		//exit( 0 );
#ifdef OLD_GRAPH_CODE
		// allocate index buffer memory
		
		int numpivots = n_smax;
		float *index_buffer		= (float *) malloc( sizeof( float ) * tier->t_idx->width * tier->t_idx->height * 4 );
		float *dist_hi_buffer	= (float *) malloc( sizeof( float ) * tier->t_d->width * tier->t_d->height * 4 );

		// choose initial 4 landmark points
		
		float *pivot_distances = ( float * ) malloc( glimmerGraph->numnodes * sizeof( float ) * numpivots );

		// calculate distance matrix rows for initial landmark points

		//clock_t time1 = clock( );
		int *pivots = select_pivots( glimmerGraph, pivot_distances, numpivots );
		//printf("bfs took %d ms \n", clock( ) - time1 );

		// update index and dist hi buffers with landmark info

		for( int i = 0; i < glimmerGraph->numnodes; i++ ) {
			for( int p = 0; p < numpivots; p++ ) {
				index_buffer	[ i * n_set_bins * 4 + n_vmax + p ]	= (float) pivots[ p ];
				dist_hi_buffer	[ i * n_set_bins * 4 + n_vmax + p ]	= (pivots[p]==i)?1000.0f:(1.f/g_f_norm_dist) * pivot_distances[ i*numpivots + p ];
			}
		}

		// calculate everyone's 4 nearest neighbors and their distances

		//time1 = clock( );
		float *temp_distances	= (float *)malloc(sizeof(float) * n_vmax);
		float *temp_indices		= (float *)malloc(sizeof(float) * n_vmax);
		for( int i = 0; i < glimmerGraph->numnodes; i++ ) {
			select_neighbors( glimmerGraph, i, n_vmax, temp_distances, temp_indices );
			for( int p = 0; p < n_vmax; p++ ) {
				index_buffer	[ i * n_set_bins * 4 + p ]	= temp_indices[ p ];
				dist_hi_buffer	[ i * n_set_bins * 4 + p ]	= (1.f/g_f_norm_dist)*temp_distances[ p ];
				for( int q = 0; q < n_smax; q++ ) {
					if( temp_indices[ p ] == (float) pivots[ q ] )
						dist_hi_buffer	[ i * n_set_bins * 4 + p ] = 1000.0f;
				}
			}
		}
		free(temp_distances	);
		free(temp_indices	);
		//printf("nnfillup took %d ms \n", clock( ) - time1 );
		//exit( 0 );
		// copy indices and high-d data to gpu

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_idx->texture_name[ tier->t_idx->attach_idx ]);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_idx->width,tier->t_idx->height,
						GL_RGBA,GL_FLOAT,index_buffer);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_d->texture_name[ tier->t_d->attach_idx ]);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_d->width,tier->t_d->height,
						GL_RGBA,GL_FLOAT,dist_hi_buffer);

		// cleanup memory allocation

		free( index_buffer );
		free( dist_hi_buffer );
		free( pivot_distances );
		free( pivots );
#endif
	}

	//// setup t_g
	if( b_output_debug ) 
		printf("\tEmbedded Distance texture...");
	tier->t_g->width = tier->t_embed->width * n_set_bins;
	tier->t_g->height = tier->t_embed->height;
	tier->t_g->attachments = 2;
	tier->t_g->attach_idx = 0;
	buildFBOTexture(tier->t_g);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_g->texture_name[ tier->t_g->attach_idx ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_g->width,tier->t_g->height,
                    GL_RGBA,GL_FLOAT,zero_data);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_g->texture_name[ tier->t_g->attach_idx?0:1 ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_g->width,tier->t_g->height,
                    GL_RGBA,GL_FLOAT,zero_data);
	if( b_output_debug ) 
		printf("done.\n");

	// setup t_sum
	//if( b_output_debug ) 
	//	printf("\tForce Sums...");
	//tier->t_sum->width = tier->t_embed->width;
	//tier->t_sum->height = tier->t_embed->height;
	//tier->t_sum->attachments = 2;
	//tier->t_sum->attach_idx = 0;
	//buildFBOTexture(tier->t_sum);
	//glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_sum->texture_name[ tier->t_sum->attach_idx ]);
 //   glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_sum->width,tier->t_sum->height,
 //                   GL_RGBA,GL_FLOAT,zero_data);
	//if( b_output_debug ) 
	//	printf("done.\n");

	// setup t_force
	if( b_output_debug ) 
		printf("\tForces...");
	tier->t_force->width = tier->t_embed->width;
	tier->t_force->height = tier->t_embed->height;
	tier->t_force->attachments = 2;
	tier->t_force->attach_idx = 0;
	buildFBOTexture(tier->t_force);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_force->texture_name[ tier->t_force->attach_idx ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_force->width,tier->t_force->height,
                    GL_RGBA,GL_FLOAT,zero_data);
	if( b_output_debug ) 
		printf("done.\n");

	// setup t_velocity
	if( b_output_debug ) 
		printf("\tVelocities...");
	tier->t_velocity->width = tier->t_embed->width;
	tier->t_velocity->height = tier->t_embed->height;
	tier->t_velocity->attachments = 2;
	tier->t_velocity->attach_idx = 0;
	buildFBOTexture(tier->t_velocity);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_velocity->texture_name[ tier->t_velocity->attach_idx ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_velocity->width,tier->t_velocity->height,
                    GL_RGBA,GL_FLOAT,zero_data);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_velocity->texture_name[ tier->t_velocity->attach_idx?0:1 ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_velocity->width,tier->t_velocity->height,
                    GL_RGBA,GL_FLOAT,zero_data);
	if( b_output_debug ) 
		printf("done.\n");

	// setup diff texture
	if( ! g_b_useDistance ) {
		if( b_output_debug ) 
			printf("\tDiffs...");
		tier->t_diff->width = (int)(ceil( (double)((tier->t_d->width)*ceil(((double)n_dims)/4.0))));
		tier->t_diff->height = tier->t_d->height;
		tier->t_diff->attachments = 2;
		tier->t_diff->attach_idx = 0;
		buildFBOTexture(tier->t_diff);
		if( b_output_debug ) 
			printf("done.\n");
	}

	if( (g_b_useStress || g_b_oneTimeStress ) ) { //&& ! g_b_useDistance ) {
		 //setup stress texture
		if( b_output_debug ) 
			printf("\tStress Texture...");

		tier->t_stress->width = (g_b_oneTimeStress||g_b_oneTimeStressEnd)?tier->t_pts->width:tier->t_embed->width;
		tier->t_stress->height = (g_b_oneTimeStress||g_b_oneTimeStressEnd)?tier->t_pts->height:tier->t_embed->height;

		// TODO: switch to above sizing after tests are finished.
		//tier->t_stress->width = tier->t_pts->width;
		//tier->t_stress->height = tier->t_pts->height;

		tier->t_stress->attachments = 2;
		tier->t_stress->attach_idx = 0;
		buildFBOTexture(tier->t_stress);
		if( b_output_debug ) 
			printf("done.\n");
	}
	// else if( g_b_useVel ) {
	if( 0 ) {	// replace with line above STRESSKILLME
		// setup velocity texture
		if( b_output_debug ) 
			printf("\tStress Denominator...");
		tier->t_stress->width = tier->t_embed->width;
		tier->t_stress->height = tier->t_embed->height;
		tier->t_stress->attachments = 2;
		tier->t_stress->attach_idx = 0;
		buildFBOTexture(tier->t_stress);
		if( b_output_debug ) 
			printf("done.\n");
	}

	if( b_output_debug ) 
		printf("done.\n");

}

/*
	Write texture data to an output file
*/
void outputTexture( Texture *texture, const char *filename ) {
	int i = 0;
	float *temp = NULL;
	FILE *fp = NULL;
	printf("outputting %s...%d by %d\n", filename, texture->width, texture->height );
	if( (temp = (float * ) malloc( sizeof( float ) * 4 * texture->width * texture->height ) ) == NULL ) {
		printf("ERROR: cannot allocate output buffer.");
		exit( 0 );
	}
	if( (fp = fopen(filename,"w") ) == NULL ) {
		printf("ERROR: cannot open output file %s.", filename);
		exit( 0 );
	}
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);
	glReadBuffer(attachmentpoints[texture->attach_idx]);
	glReadPixels(0, 0, texture->width, texture->height,GL_RGBA,GL_FLOAT,temp);

	for( i = 0; i < (4 * texture->width * texture->height); i+= 4 ) {
		fprintf(fp, "%f %f %f %f\n",  temp[i], temp[i+1], temp[i+2], temp[i+3] );
	}
	free( temp );
}

/*
	Write texture data to an output file
*/
void outputTextureLevel( Texture *texture, int level, const char *filename ) {
	int i = 0;
	float *temp = NULL;
	FILE *fp = NULL;
	printf("outputting %s...%d by %d\n", filename, texture->tier_width[level], texture->tier_height[level] );
	fflush(stdout);
	if( (temp = (float * ) malloc( sizeof( float ) * 4 * texture->tier_width[level] * texture->tier_height[level] ) ) == NULL ) {
		printf("ERROR: cannot allocate output buffer.");
		exit( 0 );
	}
	if( (fp = fopen(filename,"w") ) == NULL ) {
		printf("ERROR: cannot open output file %s.", filename);
		exit( 0 );
	}
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);
	glReadBuffer(attachmentpoints[texture->attach_idx]);
	glReadPixels(0, 0, texture->tier_width[level], texture->tier_height[level],GL_RGBA,GL_FLOAT,temp);

	for( i = 0; i < (4 * texture->tier_width[level] * texture->tier_height[level]); i+= 4 ) {
		fprintf(fp, "%f %f %f %f\n",  temp[i], temp[i+1], temp[i+2], temp[i+3] );
	}
	free( temp );
}

void setup_sh_copy( void ) {

	char shader_path[8192];

	// setup sh_copy

	sh_copy.program = glCreateProgramObjectARB();
	sh_copy.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_copy.program, sh_copy.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_copy.glsl");
	string copy_string = readShaderSource(shader_path);
	const GLcharARB *copy_source = copy_string.c_str();
	glShaderSourceARB(sh_copy.shader, 1, &copy_source, NULL);
	glCompileShaderARB(sh_copy.shader);
	glLinkProgramARB(sh_copy.program);
	printInfoLog( "sh_copy.shader", sh_copy.shader );
	u_copy.isFloat = false;
	u_copy.location = glGetUniformLocationARB( sh_copy.program, "t_copy" );

}

/*
	Copy the contents of the source texture to the destination texture for a given level
	of a textur hierarchy.  Simply renders the contents of the texture onto the other side
	without any shading program.
*/
void copyFlip( Texture *texture, int src, int dest, Tier *tier ) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture->tier_width[tier->level],0.0,texture->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture->tier_width[tier->level],texture->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glUseProgramObjectARB( sh_copy.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[ src ]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[ src ]);
	glUniform1iARB(u_copy.location, 1); 

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[dest]);
	glPolygonMode(GL_FRONT,GL_FILL);

	// and render quad
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); 
		glVertex2f(0.0, 0.0);
		glTexCoord2f(texture->tier_width[tier->level], 0.0); 
		glVertex2f(texture->tier_width[tier->level], 0.0);
		glTexCoord2f(texture->tier_width[tier->level], texture->tier_height[tier->level]); 
		glVertex2f(texture->tier_width[tier->level], texture->tier_height[tier->level]);
		glTexCoord2f(0.0, texture->tier_height[tier->level]); 
		glVertex2f(0.0, texture->tier_height[tier->level]);
	glEnd();

}
