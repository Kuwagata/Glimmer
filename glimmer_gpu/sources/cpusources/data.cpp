#include <iostream>
#include <string>
#include <cmath>
#include <float.h>
#include <time.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <stdio.h>
#include <cstring>
#include "data.h"
#include "params.h"
#include "mmio.h"
#include "sh_rand.h"
#include <map>
using std::string;

//extern int n_points;

extern bool g_b_percent_output;
extern string g_str_percent_filename;
float f_embed_min_x=10000.f;
float f_embed_max_x=-10000.f;
float f_embed_min_y=10000.f;
float f_embed_max_y=-10000.f;
extern int n_smax;
extern int n_vmax;
extern bool g_b_useGraph;
extern bool b_output_debug;
extern bool g_b_useDistance;
extern int g_n_vcycle_count;
extern int n_dims;
extern int n_embed_dims;
extern float *zero_data;
extern float *result;
extern clock_t g_dloadtime1;
extern clock_t g_dloadtime2;
extern GLenum attachmentpoints[];
//extern Texture t_pts;
//extern Texture t_embed;
extern Tier *g_Pyramid;

std::map<int,int> g_distmap;
//int n_hardlimit = 100000;	// max distance map entries

/*
	Output 
*/
void outputPoints ( const char *output_filename ) {

	int i,j;
	float *temp_embed = NULL;
	float *temp_temp_embed = NULL;
	float *temp_reference = NULL;
	FILE *fp = NULL;

	if( (fp = fopen( output_filename, "w" )) == NULL ) {
		printf("ERROR: Can't open points output file %s\n", output_filename );
		exit( 0 );
	}
	if( (temp_embed = (float * ) malloc( sizeof( float ) * 4 * g_Pyramid->t_embed->width * g_Pyramid->t_embed->height ) ) == NULL ) {
		printf("ERROR: cannot allocate temporary points output buffer.");
		exit( 0 );
	}
	if( (temp_temp_embed = (float * ) malloc( sizeof( float ) * 4 * g_Pyramid->t_embed->width * g_Pyramid->t_embed->height ) ) == NULL ) {
		printf("ERROR: cannot allocate temporary points output buffer.");
		exit( 0 );
	}
	if( (temp_reference = (float * ) malloc( sizeof( float ) * 4 * g_Pyramid->t_reference->width * g_Pyramid->t_reference->height ) ) == NULL ) {
		printf("ERROR: cannot allocate temporary points output buffer.");
		exit( 0 );
	}

	g_dloadtime1 = clock();

	// read embedding data

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,g_Pyramid->t_embed->frame_buffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,g_Pyramid->t_embed->texture_name[g_Pyramid->t_embed->attach_idx]);
	glReadBuffer(attachmentpoints[g_Pyramid->t_embed->attach_idx]);
	glReadPixels(0, 0, g_Pyramid->t_embed->width, g_Pyramid->t_embed->height,GL_RGBA,GL_FLOAT,temp_temp_embed);

	// read permutation data

	if( g_n_vcycle_count > 0 ) {
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,g_Pyramid->t_reference->frame_buffer);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,g_Pyramid->t_reference->texture_name[g_Pyramid->t_reference->attach_idx]);
		glReadBuffer(attachmentpoints[g_Pyramid->t_reference->attach_idx]);
		glReadPixels(0, 0, g_Pyramid->t_reference->width, g_Pyramid->t_reference->height,GL_RGBA,GL_FLOAT,temp_reference);
	}

	g_dloadtime2 = clock();

	int loopsize =	(4 * g_Pyramid->t_embed->width * g_Pyramid->t_embed->height) > (g_Pyramid->n_points*4)?
					(g_Pyramid->n_points*4):
					(4 * g_Pyramid->t_embed->width * g_Pyramid->t_embed->height);

	// rearrange input data
	for( i = 0; i < loopsize; i+= 4 ) {
		for( j = 0; j < n_embed_dims; j++ ) {
			if( g_n_vcycle_count > 0 ) 
				temp_embed[ ((int)temp_reference[i])*4 + j ] = temp_temp_embed[ i + j];
			else
				temp_embed[ i + j ] = temp_temp_embed[ i + j ];
		}
	}

	fprintf(fp,"X,Y\nDOUBLE,DOUBLE\n"); // output header

	// output data to file

	for( i = 0; i < loopsize; i+= 4 ) {
		for( j = 0; j < n_embed_dims; j++ ) {
			fprintf(fp, "%f",  temp_embed[i+j] );
			if( j < n_embed_dims-1) 
				fprintf(fp, ",",  temp_embed[i+j] );
		}
		fprintf(fp,"\n");
	}

	free( temp_reference );
	free( temp_embed );
	free( temp_temp_embed );

}

void readEmbed( Tier *tier, const char *filename ) {

	if( ! g_b_useDistance ) {
		tier->t_embed->height = tier->t_pts->height;
		tier->t_embed->width = tier->t_pts->width / (int) ceil( (double)n_dims / 4.0 );
		tier->t_embed->width *= (int) ceil( (double)n_embed_dims / 4.0 );
	}
	else {
		if( ! g_b_useGraph ) {
			tier->t_embed->width = (int) ceil( sqrt( (double) tier->n_points ) ); 
			tier->t_embed->height = (int) ceil(((double)tier->n_points) / ((double)(tier->t_embed->width)));
		}
		else {
			tier->t_embed->height = tier->t_pts->height;
			tier->t_embed->width = tier->t_pts->width / (int) ceil( (double)(n_vmax+n_smax) / 4.0 );
		}
	}

    result = plainCSVData( filename, tier->t_embed->width, tier->t_embed->height );
}

/*
	Initializes the embedding coordinates to be the first 
	n_embed_dims values of the original data.
*/
void initEmbed( Tier *tier ) {
	
	int i, j, k;
	
	if( ! g_b_useDistance ) {
		tier->t_embed->height = tier->t_pts->height;
		tier->t_embed->width = tier->t_pts->width / (int) ceil( (double)n_dims / 4.0 );
		tier->t_embed->width *= (int) ceil( (double)n_embed_dims / 4.0 );
	}
	else {
		if( ! g_b_useGraph ) {
			tier->t_embed->width = (int) ceil( sqrt( (double) tier->n_points ) ); 
			tier->t_embed->height = (int) ceil(((double)tier->n_points) / ((double)(tier->t_embed->width)));
		}
		else {
			tier->t_embed->height = tier->t_pts->height;
			tier->t_embed->width = tier->t_pts->width / (int) ceil( (double)(n_vmax+n_smax) / 4.0 );
		}
	}

	if( b_output_debug ) {
		printf("Embed = %d by %d  = %d slots\n", tier->t_embed->width, tier->t_embed->height, tier->t_embed->height*tier->t_embed->width );
	}

    result = (float*)malloc( 4*(tier->t_embed->height)*(tier->t_embed->width)*sizeof(float));
	for( i = 0, k = 0; i < tier->n_points; i++ ) {
		for( j = 0;  j < ((int)ceil(((double)n_embed_dims) / 4.0 ))*4; j++ ) {
			if( (j % 4) == 3 ) {
				result[k++] = 1.0f;
			}
			else {
				if( j < n_embed_dims )
					result[k++] = (float)((rand()%1000) / 1000.f)*((float)CAMERA_BOUND) - (0.5f*(float)CAMERA_BOUND);
				else
					result[k++] = 0.0f;
			}
		}
	}
	for( ;k < (4*(tier->t_embed->height)*(tier->t_embed->width)); k++ )
		result[k] = 0.0f;
}

void initTier( Tier *tier ) {
	tier->level = 0;
	tier->t_d = (Texture *)malloc(sizeof(Texture));
	tier->t_g = (Texture *)malloc(sizeof(Texture));
	tier->t_idx = (Texture *)malloc(sizeof(Texture));
	tier->t_pts = (Texture *)malloc(sizeof(Texture));
	tier->t_vec_idx = (Texture *)malloc(sizeof(Texture));
	tier->t_embed = (Texture *)malloc(sizeof(Texture));
	tier->t_rand = (Texture *)malloc(sizeof(Texture));
	//tier->t_sum = (Texture *)malloc(sizeof(Texture));
	tier->t_force = (Texture *)malloc(sizeof(Texture));
	tier->t_velocity = (Texture *)malloc(sizeof(Texture));
	tier->t_diff = (Texture *)malloc(sizeof(Texture));
	tier->t_perm = (Texture *)malloc(sizeof(Texture));
	tier->t_reference = (Texture *)malloc(sizeof(Texture));
	tier->t_stress = (Texture *)malloc(sizeof(Texture));
	tier->next = NULL;
	tier->prev = NULL;
	tier->randr = NULL;
	tier->perm = NULL;
	tier->b_update_s = false;
}

void initZero( Tier *tier ) {
	int i = 0;
	if( zero_data != NULL ) 
		free( zero_data );

	if( ! g_b_useDistance ) {
		int n_set_bins = ((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 ));
		zero_data = (float*)malloc( 4*(tier->t_pts->height)*(tier->t_pts->width*n_set_bins)*sizeof(float));
		for( i = 0; i < (4*(tier->t_pts->height)*(tier->t_pts->width*n_set_bins)); i++ ) {
			zero_data[i] = 0.0f;
		}
	}
	else {
		int n_set_bins = ((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 ));
		zero_data = (float*)malloc( 4*(tier->t_embed->height)*(tier->t_embed->width*n_set_bins)*sizeof(float));
		for( i = 0; i < (4*(tier->t_embed->height)*(tier->t_embed->width*n_set_bins)); i++ ) {
			zero_data[i] = 0.0f;
		}
	}
}

/*
	Constructs the swiss roll based on the formula found in
	Bronstien and Bronstien's Multigrid MDS paper

	The formula is:

		x = THETA
		y = .51(1/(2.75*PI) + .75*PHI) * cos( 2.5*PI*PHI )
		z = .51(1/(2.75*PI) + .75*PHI) * sin( 2.5*PI*PHI )

		for (THETA,PHI) = [0,1]*[0,1]


*/
float *swissRoll( Tier *tier, int num_points ) {
	int i = 0, j = 0;
	float *f_data = NULL;
	double PI = 3.14159;

	// set statistics
	tier->n_points = num_points;
	n_dims = 3;

	int dim_bins = (int) ceil( (double) n_dims / 4.0 );
	// allocate our data buffer	
	tier->t_pts->width = (int) floor( ceil( sqrt( (double) (tier->n_points * dim_bins) ) ) / (double) dim_bins ); //(int) ceil( sqrt( (double) n_points ) ) * (int) ceil( (double) n_dims / 4.0 );
	tier->t_pts->height = (int) ceil(((double)tier->n_points) / ((double)(tier->t_pts->width)));//(int) ceil( sqrt( (double) n_points ) );
	tier->t_pts->width *= dim_bins;

	//printf("tier->t_pts = %d x %d \n", tier->t_pts->width, tier->t_pts->height);

	//t_pts.height = t_pts.width = (int) ceil( sqrt( (double) ( n_points * (int) ceil( (double)n_dims / 4.0 ) ) ) );
	f_data = (float*) malloc( sizeof( float ) * (tier->t_pts->height) * (tier->t_pts->width) * 4);


	double THETA,PHI;
	for( i = 0; i < tier->t_pts->height; i++ ) {
		for( j = 0; j < tier->t_pts->width; j++ ) {
			THETA = ((double)j)/((double)tier->t_pts->width);
			PHI = ((double)i)/((double)tier->t_pts->height);
			f_data[i*(tier->t_pts->width)*4 + j*4 + 0] = THETA;
			f_data[i*(tier->t_pts->width)*4 + j*4 + 1] = 0.51*(1/(2.75*PI)+0.75*PHI)*cos(2.5*PI*PHI);
			f_data[i*(tier->t_pts->width)*4 + j*4 + 2] = 0.51*(1/(2.75*PI)+0.75*PHI)*sin(2.5*PI*PHI);
			f_data[i*(tier->t_pts->width)*4 + j*4 + 3] = 0.f;
		}
	}
	return f_data;
}

/*
	Output the demo data constructed from lattice or swissroll routines
	to a CSV file for testing in other environments.
*/
void outputDemoData( Tier *tier, const float *f_data, const char *filename ) {
	FILE *fp = fopen(filename,"w");
	
	// output header

	for( int i = 0; i < n_dims; i++ ) {
		fprintf(fp, "D%d",i+1);
		if( i < n_dims-1 ) {
			fprintf(fp, ",",i+1);
		}
	}
	fprintf(fp, "\n");
	for( int i = 0; i < n_dims; i++ ) {
		fprintf(fp, "REAL");
		if( i < n_dims-1 ) {
			fprintf(fp, ",",i+1);
		}
	}
	fprintf(fp, "\n");
	
	// output data
	
	int n_dim_bins = (int)ceil( ((double)n_dims) / 4.0);
	for( int i = 0; i < (tier->n_points*(n_dim_bins*4)); i+= n_dim_bins*4 ) {
		for( int k = 0; k < n_dims; k++ ) {
			fprintf(fp, "%f", f_data[i+k]);
			if( k < n_dims-1 ) {
				fprintf(fp, ",",i+1);
			}
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}

/*
	Debug function: Builds a 2-D lattice in high d space
*/
float *demoData( Tier *tier, int num_points, double noise_magnitude, int noise_dimensions ) {
	int i = 0, j = 0;
	float *f_data = NULL;


	// set statistics
	tier->n_points = num_points;
	n_dims = 3;
	n_dims = 8;

	int dim_bins = (int) ceil( (double) n_dims / 4.0 );
	// allocate our data buffer	
	tier->t_pts->width = (int) floor( ceil( sqrt( (double) (tier->n_points * dim_bins) ) ) / (double) dim_bins ); //(int) ceil( sqrt( (double) n_points ) ) * (int) ceil( (double) n_dims / 4.0 );
	tier->t_pts->height = (int) ceil(((double)tier->n_points) / ((double)(tier->t_pts->width)));//(int) ceil( sqrt( (double) n_points ) );
	tier->t_pts->width *= dim_bins;

	//printf("tier->t_pts = %d x %d \n", tier->t_pts->width, tier->t_pts->height);

	//t_pts.height = t_pts.width = (int) ceil( sqrt( (double) ( n_points * (int) ceil( (double)n_dims / 4.0 ) ) ) );
	f_data = (float*) malloc( sizeof( float ) * (tier->t_pts->height) * (tier->t_pts->width) * 4);

	// set the point coordinates (simple grid)

	for( i = 0; i < tier->t_pts->height; i++ ) {
		for( j = 0; j < tier->t_pts->width; j++ ) {
			if( !(j % dim_bins) ) {
				f_data[i*(tier->t_pts->width)*4 + j*4 + 0] = (float)j/(float)(tier->t_pts->width);
				f_data[i*(tier->t_pts->width)*4 + j*4 + 1] = 0.f;
				f_data[i*(tier->t_pts->width)*4 + j*4 + 2] = (float)i/(float)(tier->t_pts->height);
				f_data[i*(tier->t_pts->width)*4 + j*4 + 3] = 0.f;
			}
			else {
				if( ((j%dim_bins)-1)*4+0 < noise_dimensions )
					f_data[i*(tier->t_pts->width)*4 + j*4 + 0] = noise_magnitude*((float)(rand()%(tier->t_pts->width)))/(float)(tier->t_pts->width);
				else 
					f_data[i*(tier->t_pts->width)*4 + j*4 + 0] = 0.f;
				if( ((j%dim_bins)-1)*4+1 < noise_dimensions )
					f_data[i*(tier->t_pts->width)*4 + j*4 + 1] = noise_magnitude*((float)(rand()%(tier->t_pts->width)))/(float)(tier->t_pts->width);
				else 
					f_data[i*(tier->t_pts->width)*4 + j*4 + 1] = 0.f;
				if( ((j%dim_bins)-1)*4+2 < noise_dimensions )
					f_data[i*(tier->t_pts->width)*4 + j*4 + 2] = noise_magnitude*((float)(rand()%(tier->t_pts->width)))/(float)(tier->t_pts->width);
				else 
					f_data[i*(tier->t_pts->width)*4 + j*4 + 2] = 0.f;
				if( ((j%dim_bins)-1)*4+3 < noise_dimensions )
					f_data[i*(tier->t_pts->width)*4 + j*4 + 3] = noise_magnitude*((float)(rand()%(tier->t_pts->width)))/(float)(tier->t_pts->width);
				else 
					f_data[i*(tier->t_pts->width)*4 + j*4 + 3] = 0.f;
			}
		}
	}

	return f_data;
}

int maxint( int i, int j ) {
	return ( i > j )?i:j;
}
int minint( int i, int j ) {
	return ( i > j )?j:i;
}
/*
	read a symmetric distance matrix from a text file

	Format is assumed to be lower triangular of the format

	d(1,1) 
	d(2,1) d(2,2)
	d(3,1) d(3,2) (3,3)
	<EOF>

	for example:

	0
	1 0
	1 2 0
	
*/
float *readDistances( Tier *tier, const char *name ) {
	
	char line[65535];
	char item[65535];
	int i = 0, j = 0, k = 0;
	int done = 0;
	int leftover = 0;
	float *f_data = NULL;
	float *f_data_temp = NULL;

	// open the file 
	FILE *fp = fopen(name, "r");

	// return if there is an error

	if( fp == NULL ) 
		return NULL;

	// get statistics
	tier->n_points = 0;
	while( fgets( line, 65535, fp ) != NULL ) {

		//printf("%s\n",line);
		// count the number of points (for every line)
		tier->n_points++;
	}
	fclose( fp );

	// allocate our data buffers and size our texture	
	tier->t_pts->height = (int) ceil( (double)tier->n_points / 2.0 );
	tier->t_pts->width = (int) ceil( (double)tier->n_points / 4.0 );

	//printf("allocating %d, and %d\n", (tier->t_pts->height) * (tier->t_pts->width) * 4, (tier->t_pts->height) * (tier->t_pts->width) * 4 + tier->n_points );

	f_data_temp = (float*) malloc( sizeof( float ) * (tier->t_pts->height) * (tier->t_pts->width) * 4 + tier->n_points * sizeof(float) );
	f_data = (float*) malloc( sizeof( float ) * (tier->t_pts->height) * (tier->t_pts->width) * 4);

	//printf("yo! %d pts at %d %d \n", tier->n_points, f_data_temp, f_data );

	// read the data into the buffer
	//int maxstrlen = 0;
	//int maxi =0;
	//int maxj = 0;
	fp = fopen(name, "r");
	while( fgets( line, 65535, fp ) != NULL ) {
		//maxstrlen=maxint(maxstrlen,strlen(line));
		//printf("%d.",k);
		i=0;
		j=0;
		done = 0;
		while( !done ) {
			if( line[i] == ' ' ) {
				item[j] = '\0';
				f_data_temp[k++] = (float) atof( item );
				j = 0;
			}
			else if( line[i] == '\n' || line[i] == '\0' ) {
				item[j] = '\0';
				if( strlen( item ) )
					f_data_temp[k++] = (float) atof( item );
				done = 1;
			}
			else if( line[i] != ' ' ) {
				item[j++] = line[i];
			}
			i++;
			//maxi=maxint(maxi,i);
			//maxj=maxint(maxj,j);
		}
	}


	//printf("YO! %d %d %d %d\n",k,maxstrlen,maxi,maxj);	

	// arrange the data in the input buffer

	//int maxref = 0;
	int row = 1;
	int col = 1;
	int span = 0;
	if( tier->n_points % 4)
		span = 4 - (tier->n_points % 4);

	j = 0;
	for( i = 1; i <= k; i += (row+1),row++) {
		col = 1;
		while( j < i ) {
			if( row <= tier->t_pts->height ) {
				f_data[ (row-1)*(tier->t_pts->width*4) + (col-1) ] = f_data_temp[ j ];
				//maxref = ((row-1)*(tier->t_pts->width*4) + (col-1)>maxref)?((row-1)*(tier->t_pts->width*4) + (col-1)):maxref;
			}
			else {
				//printf("%i:(%d,%d) becomes (%d,%d)\n",i,row,col,(tier->n_points-row),(tier->t_pts->width*4-col-span));
				f_data[ (tier->n_points-row)*(tier->t_pts->width*4) + (tier->t_pts->width*4-col-span) ] = f_data_temp[ j ];
				//maxref = ((tier->n_points-row)*(tier->t_pts->width*4) + (tier->t_pts->width*4-col-span)>maxref)?((tier->n_points-row)*(tier->t_pts->width*4) + (tier->t_pts->width*4-col-span)):maxref;
			}
			col++;
			j++;
		}
	}	

	//printf("YOO! %d and %d\n",maxref,j);
	//printf("\n\n");
	//for( i = 0; i < tier->t_pts->height; i++ ) {
	//	for( j = 0; j < tier->t_pts->width*4; j++ ) {
	//		printf("%0.2f ", f_data[(i*tier->t_pts->width*4)+j] );
	//	}
	//	printf("\n\n");
	//}
	//exit( 0 );

	// normalize the distance matrix

	float max_dist = 0.f;
	for( i = 0; i < ((tier->t_pts->height) * (tier->t_pts->width) * 4); i++ ) {
		if( f_data[i] > max_dist )
			max_dist = f_data[i];
	}
	for( i = 0; i < ((tier->t_pts->height) * (tier->t_pts->width) * 4); i++ ) {
		f_data[i] /= max_dist;
	}

	//printf("is it this?");
	free( f_data_temp );	// release our temporary storage
	//exit( 0 );

	return f_data;
}

float *plainCSVData( const char *name, int width, int height ) {
	char line[65536];
	char item[256];
	int i = 0, j = 0, k = 0, skip = 0, l = 0;
	int done = 0;
	int leftover = 0;
	float *f_data = NULL;

	// allocate our data buffer	
	leftover = (n_embed_dims % 4)?4 - (n_embed_dims % 4):0;
	f_data = (float*) malloc( sizeof( float ) * height * width * 4);

	// read the data into the buffer

	FILE *fp = fopen(name, "r");
	while( fgets( line, 65535, fp ) != NULL ) {
		done = 0;
		i = 0;
		j = 0;
		while( !done ) {
			if( skip < SKIP_LINES ) {
				skip++;
				done = 1;
			}

			if( line[i] == ',' ) {
				item[j] = '\0';
				f_data[k++] = (float) atof( item );
				//printf("%f,", f_data[k-1]);
				j = 0;
			}
			else if( line[i] == '\n' || line[i] == '\0' ) {
				item[j] = '\0';
				f_data[k++] = (float) atof( item );
				//printf("%f,", f_data[k-1]);
				for( l = 0; l < leftover; l++ ) {
					if( (k % 4) == 3 ) {
						f_data[k++] = 1.0f;
					}
					else {
						f_data[k++] = 0.0f;
					}
					//printf("0.0,", f_data[k-1]);
				}
				//printf("\n");
				done = 1;
			}
			else if( line[i] != ' ' ) {
				item[j++] = line[i];
			}
			i++;
		}
	}

	for( i = 0; i < k; i++ ) {
		if( !(i%4) ) {
			if( f_data[i] < f_embed_min_x ) {
				f_embed_min_x = f_data[i];
			}
			if( f_data[i] > f_embed_max_x ) {
				f_embed_max_x = f_data[i];
			}
		}
		if( i%4 == 1 ) {
			if( f_data[i] < f_embed_min_y ) {
				f_embed_min_y = f_data[i];
			}
			if( f_data[i] > f_embed_max_y ) {
				f_embed_max_y = f_data[i];
			}
		}
	}

	for( ;k < (4*height*width); k++ )
		f_data[k] = 0.0f;

	return f_data;
}

/*
	Parse a simple CSV file into a float buffer
*/
float *readCSVData( Tier *tier, const char *name, float percent ) {
	char line[65536];
	char item[256];
	int i = 0, j = 0, k = 0, skip = 0, l = 0;
	int done = 0;
	int leftover = 0;
	float *f_data = NULL;

	// open the file 
	FILE *fp = fopen(name, "r");

	// return if there is an error

	if( fp == NULL ) 
		return NULL;

	// get statistics
	tier->n_points = 0;
	n_dims = 0;
	while( fgets( line, 65535, fp ) != NULL ) {

		// count the number of points (for every line)
		tier->n_points++;

		// count the number of dimensions (once)
		if( n_dims == 0 && tier->n_points > SKIP_LINES) {
			i = 0;
			while( line[i] != '\0' ) {
				if( line[i] == ',' ) {
					n_dims++;
				}
				i++;
			}
			n_dims++;
		}
	}
	fclose( fp );
	tier->n_points -= SKIP_LINES;

	if( b_output_debug )
		printf("counted %d points with %d dimensions\n", tier->n_points, n_dims );

	// allocate our data buffer	
	int dim_bins = (int) ceil( (double) n_dims / 4.0 );
	tier->t_pts->width = (int) floor( ceil( sqrt( (double)(tier->n_points * dim_bins) ) ) / (double) dim_bins ); //(int) ceil( sqrt( (double) n_points ) ) * (int) ceil( (double) n_dims / 4.0 );
	tier->t_pts->height = (int) ceil(((double)tier->n_points) / ((double)(tier->t_pts->width)));//(int) ceil( sqrt( (double) n_points ) );
	tier->t_pts->width *= dim_bins;

	//t_pts.height = (int) ceil( sqrt( (double) ( n_points  ) ) );
	//t_pts.width = t_pts.height * (int) ceil( (double)n_dims / 4.0 );
	leftover = (n_dims % 4)?4 - (n_dims % 4):0;
	f_data = (float*) malloc( sizeof( float ) * (tier->t_pts->height) * (tier->t_pts->width) * 4);
	//printf("n_points = %d\n", n_points);
	//printf("w:%d h:%d l:%d\n", t_pts.width, t_pts.height, leftover );
	//exit( 0 );

	// read the data into the buffer

	fp = fopen(name, "r");
	while( fgets( line, 65535, fp ) != NULL ) {
		done = 0;
		i = 0;
		j = 0;
		while( !done ) {
			if( skip < SKIP_LINES ) {
				skip++;
				done = 1;
			}

			if( line[i] == ',' ) {
				item[j] = '\0';
				f_data[k++] = (float) atof( item );
				j = 0;
			}
			else if( line[i] == '\n' || line[i] == '\0' ) {
				item[j] = '\0';
				f_data[k++] = (float) atof( item );
				for( l = 0; l < leftover; l++ ) {
					f_data[k++] = 0.0f;
				}
				done = 1;
			}
			else if( line[i] != ' ' ) {
				item[j++] = line[i];
			}
			i++;
		}
	}

	if( 1.f - percent > 1e-5 ) {

		// permute the data using Knuth shuffle

		float *shuffle_temp = (float *) malloc( sizeof( float ) * dim_bins*4 ) ;
		int shuffle_idx = 0;
		for( i = 0; i < (tier->n_points*dim_bins*4); i+=(dim_bins*4) ) {
			shuffle_idx = i + ( rand() % (tier->n_points-(i/(dim_bins*4))) )*dim_bins*4;
			//printf("%d: %d\n", i, shuffle_idx/(dim_bins*4) );
			for( j = 0; j < (dim_bins*4); j++ ) {
				shuffle_temp[j]=f_data[i+j];
				f_data[i+j] = f_data[shuffle_idx+j];
				f_data[shuffle_idx+j] = shuffle_temp[j];
			}		
		}
		free(shuffle_temp);

		// gather only a percentage worth

		tier->n_points = (int) ceil( ((double)tier->n_points)*percent );
		tier->t_pts->width = (int) floor( ceil( sqrt( (double)(tier->n_points * dim_bins) ) ) / (double) dim_bins ); 
		tier->t_pts->height = (int) ceil(((double)tier->n_points) / ((double)(tier->t_pts->width)));
		tier->t_pts->width *= dim_bins;
		f_data = (float *) realloc( f_data, sizeof( float ) * (tier->t_pts->height) * (tier->t_pts->width) * 4 );

		// open a file for outputting the specified percentage (for generating test sets)
		if( g_b_percent_output ) {
			FILE *fp_percent = NULL;
			if( ( fp_percent = fopen(g_str_percent_filename.c_str(),"w") ) != NULL ) {
				for( j = 0 ; j < n_dims; j++ ) {
					if( j < n_dims-1 ) {
						fprintf(fp_percent,"D%d,",(j+1));
					}
					else{
						fprintf(fp_percent,"D%d\n",(j+1));
					}
				}
				for( j = 0 ; j < n_dims; j++ ) {
					if( j < n_dims-1 ) {
						fprintf(fp_percent,"REAL,");
					}
					else{
						fprintf(fp_percent,"REAL\n");
					}
				}
				for( i = 0; i < (tier->n_points); i++ ) {
					for( j = 0 ; j < n_dims; j++ ) {
						if( j < n_dims-1 ) {
							fprintf(fp_percent,"%f,",f_data[(i*(dim_bins*4))+j]);
						}
						else {
							fprintf(fp_percent,"%f\n",f_data[(i*(dim_bins*4))+j]);
						}
					}
				}			
				fclose( fp_percent );
			}
			exit( 0 );
		}
	}

	//// normalize the data

	float *max_vals = (float *) malloc( sizeof( float ) * n_dims );
	float *min_vals = (float *) malloc( sizeof( float ) * n_dims );
	for( i = 0; i < n_dims; i++ ) {
		max_vals[ i ] = 0.f;
		min_vals[ i ] = 10000.0f;
	}
	k = 0;
	for( i = 0; i < tier->n_points; i++ ) {		
		for( j = 0; j < dim_bins*4; j++ ) {
			if( j < n_dims ) {
				if( f_data[k] > max_vals[j] ) {
					max_vals[j] = f_data[k];
				}
				if( f_data[k] < min_vals[j] ) {
					min_vals[j] = f_data[k];					
				}
			}
			k++;
		}
	}
	for( i = 0; i < n_dims; i++ ) {
		max_vals[ i ] -= min_vals[ i ];
	}
	k = 0;
	for( i = 0; i < tier->n_points; i++ ) {		
		for( j = 0; j < dim_bins*4; j++ ) {
			if( j < n_dims ) {
				if( (max_vals[j] - min_vals[j]) < 0.0001f ) {
					f_data[k] = 0.f;
				}
				else {
					f_data[k] = (f_data[k] - min_vals[j])/max_vals[j];
					if( !finite( f_data[k]) ) 
						f_data[k] = 0.f;
				}
			}
			k++;
		}
	}
	free( max_vals );
	free( min_vals );

	//exit( 0 );
	return f_data;
}

/*
	Parse a simple CSV file into a float buffer
*/
void convert_to_mat( const char *inname, const char *outname ) {
	char line[65536];
	char item[256];
	int i = 0, j = 0, k = 0, skip = 0, l = 0;
	int done = 0;
	int leftover = 0;
	float *f_data = NULL;

	// open the file 
	FILE *fp = fopen(inname, "r");

	// return if there is an error

	if( fp == NULL ) 
		return;

	// get statistics
	int n_points = 0;
	while( fgets( line, 65535, fp ) != NULL ) {

		// count the number of points (for every line)
		n_points++;

		// count the number of dimensions (once)
		if( n_dims == 0 ) {
			i = 0;
			while( line[i] != '\0' ) {
				if( line[i] == ',' ) {
					n_dims++;
				}
				i++;
			}
			n_dims++;
		}
	}
	fclose( fp );
	n_points -= SKIP_LINES;

	// allocate our data buffer	
	int dim_bins = (int) ceil( (double) n_dims / 4.0 );
	int width = (int) floor( ceil( sqrt( (double)(n_points * dim_bins) ) ) / (double) dim_bins ); //(int) ceil( sqrt( (double) n_points ) ) * (int) ceil( (double) n_dims / 4.0 );
	int height = (int) ceil(((double)n_points) / ((double)(width)));//(int) ceil( sqrt( (double) n_points ) );
	width *= dim_bins;

	//t_pts.height = (int) ceil( sqrt( (double) ( n_points  ) ) );
	//t_pts.width = t_pts.height * (int) ceil( (double)n_dims / 4.0 );
	leftover = (n_dims % 4)?4 - (n_dims % 4):0;
	f_data = (float*) malloc( sizeof( float ) * height * width * 4);
	//printf("n_points = %d\n", n_points);
	//printf("w:%d h:%d l:%d\n", t_pts.width, t_pts.height, leftover );
	//exit( 0 );

	// read the data into the buffer

	fp = fopen(inname, "r");
	while( fgets( line, 65535, fp ) != NULL ) {
		done = 0;
		i = 0;
		j = 0;
		while( !done ) {
			if( skip < SKIP_LINES ) {
				skip++;
				done = 1;
			}

			if( line[i] == ',' ) {
				item[j] = '\0';
				f_data[k++] = (float) atof( item );
				j = 0;
			}
			else if( line[i] == '\n' || line[i] == '\0' ) {
				item[j] = '\0';
				f_data[k++] = (float) atof( item );
				for( l = 0; l < leftover; l++ ) {
					f_data[k++] = 0.0f;
				}
				done = 1;
			}
			else if( line[i] != ' ' ) {
				item[j++] = line[i];
			}
			i++;
		}
	}

	//// normalize the data

	FILE *fpout = fopen( outname, "w");
	k = 0;
	for( i = 0; i < n_points; i++ ) {		
		for( j = 0; j < dim_bins*4; j++ ) {
			if( j < n_dims ) {
				fprintf(fpout, "%f ", f_data[k] );
			}
			k++;
		}
		fprintf(fpout, "\n" );
	}
}

/*
	Loads sparse matrix data from vec file of the following format
	
	(index,value) ... \n

	into two arrays index and values.
	Must know maxsize beforehand.  Use the following perlscript
	to determine this:

	#!/usr/bin/perl
	$i=0;
	while(<>){
			chop;
			s/[\w(). ]+//g;
			if ($i < length ){
					$i = length;
			}
	}
	print "$i\n";

*/
int loadVecData( const char *name, Tier *tier, float **index, float **values, int maxsize, float percent ) {

	char line[65536];
	char item[256];
	int i = 0, j = 0, k = 0, m = 0, skip = 0, l = 0;
	bool line_done = 0;
	bool paren_done = 0;
	bool before_comma = 0;
	FILE *fp = 0;

	// open the file 
	fp = fopen(name, "r");

	// return if there is an error

	if( fp == NULL ) 
		return NULL;

	// count the number of points (for every line)
	tier->n_points = 0;
	while( fgets( line, 65535, fp ) != NULL ) 
		tier->n_points++;
	fclose( fp );

	// allocate our data buffers
	int dim_bins = (int) ceil( (double) maxsize / 4.0 );
	n_dims = maxsize;
	tier->t_vec_idx->width = (int) floor( ceil( sqrt( (double)(tier->n_points * dim_bins) ) ) / (double) dim_bins ); 
	tier->t_vec_idx->height = (int) ceil(((double)tier->n_points) / ((double)(tier->t_vec_idx->width)));
	tier->t_vec_idx->width *= dim_bins;
	tier->t_pts->width = (int) floor( ceil( sqrt( (double)(tier->n_points * dim_bins) ) ) / (double) dim_bins ); 
	tier->t_pts->height = (int) ceil(((double)tier->n_points) / ((double)(tier->t_pts->width)));
	tier->t_pts->width *= dim_bins;
	int leftover = (maxsize % 4)?4 - (maxsize % 4):0;
	//printf("%d by %d and %d by %d\n", tier->t_vec_idx->height,tier->t_vec_idx->width, tier->t_pts->height, tier->t_pts->width);
	float *f_index = (float*) malloc( sizeof( float ) * (tier->t_vec_idx->height) * (tier->t_vec_idx->width) * 4);
	float *f_values = (float*) malloc( sizeof( float ) * (tier->t_pts->height) * (tier->t_pts->width) * 4);
	//printf("get we here?");


	// find maximum index value K (to pad extra index space with 1+K)

	int padding = 0;
	float K = 0;
	fp = fopen(name, "r");
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
						float tempidx = atof( item );
						if( K < tempidx ) K = tempidx;
						j=0;
					}
					else if( line[i] == ')' ) {
						item[j] = '\0';
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
				line_done = true;
			}
		}
	}
	fclose( fp );

	// read values into buffers

	fp = fopen(name, "r");
	while( fgets( line, 65535, fp ) != NULL ) {

		line_done = false;
		i = 0;
		j = 0;
		padding = 0;
		while( !line_done ) {
			paren_done = false;
			if( line[i++] == '(' ) {
				padding++;
				while( !paren_done ) {
					if( line[i] == ',' ) {
						item[j] = '\0';
						f_index[k++] = atof( item );
						j=0;
					}
					else if( line[i] == ')' ) {
						item[j] = '\0';
						f_values[m++] = atof( item );
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
				//printf("padding = %d\n",padding);
				for( l = padding; l < maxsize; l++ ) {
					f_values[m++] = 0.0f;
					f_index[k++] = K+1.f;
				}
				for( l = 0; l < leftover; l++ ) {
					f_values[m++] = 0.0f;
					f_index[k++] = K+1.f;
				}
				line_done = true;
			}
		}
	}

	if( 1.f - percent > 1e-5 ) {

		// permute the data using Knuth shuffle

		float *shuffle_temp = (float *) malloc( sizeof( float ) * dim_bins*4 ) ;
		int shuffle_idx = 0;
		for( i = 0; i < (tier->n_points*dim_bins*4); i+=(dim_bins*4) ) {
			shuffle_idx = i + ( rand() % (tier->n_points-(i/(dim_bins*4))) )*dim_bins*4;
			//printf("%d: %d\n", i, shuffle_idx/(dim_bins*4) );
			for( j = 0; j < (dim_bins*4); j++ ) {
				shuffle_temp[j]=f_values[i+j];
				f_values[i+j] = f_values[shuffle_idx+j];
				f_values[shuffle_idx+j] = shuffle_temp[j];
			}		
			for( j = 0; j < (dim_bins*4); j++ ) {
				shuffle_temp[j]=f_index[i+j];
				f_index[i+j] = f_index[shuffle_idx+j];
				f_index[shuffle_idx+j] = shuffle_temp[j];
			}		
		}
		free(shuffle_temp);

		// gather only a percentage worth

		tier->n_points = (int) ceil( ((double)tier->n_points)*percent );
		tier->t_vec_idx->width = (int) floor( ceil( sqrt( (double)(tier->n_points * dim_bins) ) ) / (double) dim_bins ); 
		tier->t_vec_idx->height = (int) ceil(((double)tier->n_points) / ((double)(tier->t_vec_idx->width)));
		tier->t_vec_idx->width *= dim_bins;
		tier->t_pts->width = (int) floor( ceil( sqrt( (double)(tier->n_points * dim_bins) ) ) / (double) dim_bins ); 
		tier->t_pts->height = (int) ceil(((double)tier->n_points) / ((double)(tier->t_pts->width)));
		tier->t_pts->width *= dim_bins;
		f_values = (float *) realloc( f_values, sizeof( float ) * (tier->t_pts->height) * (tier->t_pts->width) * 4 );
		f_index = (float *) realloc( f_index, sizeof( float ) * (tier->t_vec_idx->height) * (tier->t_vec_idx->width) * 4 );
	
		// open a file for outputting the specified percentage (for generating test sets)
		if( g_b_percent_output ) {
			FILE *fp_percent = NULL;
			if( ( fp_percent = fopen(g_str_percent_filename.c_str(),"w") ) != NULL ) {
				for( i = 0; i < (tier->n_points); i++ ) {
					for( j = 0 ; j < (dim_bins*4); j++ ) {
						if( f_index[(i*(dim_bins*4))+j] < (K+1.) ) {
							fprintf(fp_percent,"(%d,%f) ",((int)f_index[(i*(dim_bins*4))+j]),f_values[(i*(dim_bins*4))+j]);
						}
					}
					fprintf(fp_percent, "\n");
				}			
				fclose( fp_percent );
			}
			exit( 0 );
		}

	}

	*values = f_values;
	*index = f_index;

	return 0;
}

// Dump the current opengl buffer to .ppm file
int dumpPPM(const char *outname, int width, int height)
{
    unsigned char *pixels = (unsigned char *)malloc( sizeof( unsigned char ) * 3 * width );
    FILE *fp;

    fp = fopen(outname, "wb");
    fprintf(fp, "P6 ");
    fprintf(fp, "%d %d ", width, height);
    fprintf(fp, "%d ", 255);

    for ( int y = height - 1; y >= 0; y--) {
        glReadPixels(0, y, width, 1, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *) pixels);
        fwrite(pixels, 1, 3 * width, fp);
    }

    fclose(fp);
    free(pixels);

    return 0;
}

/*
	select the closest num_neighbors neighbors for the node at src_id
*/
void select_neighbors( GlimmerGraph *graph, int src_id, int num_neighbors, float *distances, float *indices ) {

	// do a BFS from the current pivot, filling up the pivot distances

	int depth = 0;
	int current_neighbor = 0;
	graph->nodes[src_id].mark	= 1;	// mark the source node	

	// working queue

	int queuesize	= graph->nodes[src_id].numedges;
	int *queue		= (int *) malloc( sizeof( int ) * graph->nodes[src_id].numedges );
	queue			= (int *)memcpy(queue,graph->nodes[src_id].edges,sizeof(int)*graph->nodes[src_id].numedges);

	// marking queue

	int markstorage = 0;
	int *markqueue	= NULL;
	int markptr		= 0;

	while( queuesize ) {

		depth++;	// increment the search depth

		// queue for next depth of search 

		int newqueuesize	= 0;
		int newqueuememsize = 0;
		int *newqueue		= NULL;
		
		// iterate over current depth queue
		for( int i = 0; i < queuesize; i++ ) {

			distances[ current_neighbor ]	= (float) depth;				// select neighbor
			indices[ current_neighbor ]		= (float) queue[ i ];
			if( ++current_neighbor == num_neighbors ) {
				free( queue );
				if( newqueue != NULL ) free( newqueue );
				for( int j = 0; j < markptr;j++ ) {
					graph->nodes[markqueue[j]].mark = 0;
				}
				graph->nodes[src_id].mark = 0;
				if( markqueue != NULL ) free( markqueue );
				return;
			}

			graph->nodes[queue[i]].mark = 1;								// mark the node
			if( ++markptr > markstorage ) {									// resize the marked list
				if( markstorage ){
					markqueue = (int*)realloc(markqueue,sizeof(int)*markstorage*2);
					markstorage *= 2;
				} 
				else {
					markqueue = (int *)malloc(sizeof(int));
					markstorage = 1;
				}
			}
			markqueue[markptr-1]=queue[i];						// add to the marked list for later processing
		}
		for( int i = 0; i < queuesize; i++ ) {
			// check the neighborhood of the node for unmarked nodes
			for( int j = 0; j < graph->nodes[queue[i]].numedges; j++ ) {		// for each edge
				if( ! graph->nodes[graph->nodes[queue[i]].edges[j]].mark ) {	// if it is NOT marked
					graph->nodes[graph->nodes[queue[i]].edges[j]].mark = 1;
					newqueuesize++;												// increase next depth queue
					if( newqueuesize-1 ) {
						if( newqueuememsize < newqueuesize ) {
							newqueuememsize *= 2;
							newqueue = (int*)realloc( newqueue, sizeof( int ) * newqueuememsize );
						}
					}
					else {
						newqueuememsize = 1;
						newqueue = (int*)malloc( sizeof( int ) );
					}
					//if( graph->nodes[queue[i]].edges[j] ) printf("%d.",graph->nodes[queue[i]].edges[j]);
					newqueue[newqueuesize-1]=graph->nodes[queue[i]].edges[j];	// add the node
				}
			}
		}
		free(queue);				// free the current queue
		queuesize = newqueuesize;	// set the queue to the next depth queue
		queue = newqueue;			
	}

	// reset marks
	for( int i = 0; i < markptr;i++ ) {
		graph->nodes[markqueue[i]].mark = 0;
	}
	graph->nodes[src_id].mark = 0;

	// free allocations
	if( markqueue != NULL ) free( markqueue );
	if( queue != NULL ) free( queue );		
}

/*
	helper function for select_pivots
*/
inline void update_pivot_searchinfo( int numpivots, int index, int dist, int *max_index, int *max_dist, int *n_pivots, int p, float *pivot_distances ) {
}

/*
	select pivots and return the distances for all points to those pivots
	use the technique described in the HDE paper
*/
int *select_pivots( GlimmerGraph *graph, float *pivot_distances, int numpivots ) {

	int *n_pivots = (int *)malloc( numpivots * sizeof( int ) );
	
	int max_index = 0;
	int max_dist = 0;

	int *queue		= (int *) malloc( sizeof( int ) * graph->numnodes );
	int *newqueue	= (int *) malloc( sizeof( int ) * graph->numnodes );
	int *markqueue	= (int *) malloc( sizeof( int ) * graph->numnodes );
	int *temp = NULL;
	bool b_useRandom = false;

	for( int p = 0; p < numpivots; p++ ) {

		if( b_useRandom ) {
			bool b_done = false;
			while( !b_done ) {
				n_pivots[ p ] = myrand() % graph->numnodes;
				b_done = true;
				for( int i = 0; i < p; i++ ) {
					if( n_pivots[p] == n_pivots[ i ] ) b_done = false;
				}
			}
		}
		else {
			if( ! p ) {
				n_pivots[ p ] = myrand() % graph->numnodes;
			}
			else {
				n_pivots[ p ] = max_index;
			}
		}

		max_index = 0; 
		max_dist = 0;

		// do a BFS from the current pivot, filling up the pivot distances
	
		int depth = 0;
		int src_id = n_pivots[ p ];

		graph->nodes[src_id].mark	= 1;	// mark the source node
		pivot_distances[ src_id * numpivots + p ] = (float) depth;

		// working queue

		int queuesize	= graph->nodes[src_id].numedges;
		queue			= (int *)memcpy(queue,graph->nodes[src_id].edges,sizeof(int)*graph->nodes[src_id].numedges);

		// marking queue

		int markptr		= 0;

		while( queuesize ) {

			depth++;	// increment the search depth

			// queue for next depth of search 

			int newqueuesize	= 0;
			
			// iterate over current depth queue
			for( int i = 0; i < queuesize; i++ ) {

				pivot_distances[ queue[i] * numpivots + p ] = (float) depth;	// save the distance
				if( ! b_useRandom ) {											// update pivoting information
					int dist_tally = depth;
					int j = 0;
					for( j = 0; j < p; j++ ) {
						if( queue[i] == n_pivots[ j ] ) break;
						dist_tally += pivot_distances[ queue[i] * numpivots + j ];
					}
					if( dist_tally > max_dist && ( queue[i] != n_pivots[ j ] ) ) {
						max_index = queue[i];
						max_dist = dist_tally;
					}
				}
				graph->nodes[queue[i]].mark = 1;								// mark the node
				markqueue[markptr++]=queue[i];						// add to the marked list for later processing
			}
			for( int i = 0; i < queuesize; i++ ) {
				// check the neighborhood of the node for unmarked nodes
				for( int j = 0; j < graph->nodes[queue[i]].numedges; j++ ) {		// for each edge
					if( ! graph->nodes[graph->nodes[queue[i]].edges[j]].mark ) {	// if it is NOT marked
						graph->nodes[graph->nodes[queue[i]].edges[j]].mark = 1;
						newqueue[newqueuesize++]=graph->nodes[queue[i]].edges[j];	// add the node
					}
				}
			}
			queuesize = newqueuesize;	// set the queue to the next depth queue
			temp = queue;				// swap queue storage pointers
			queue = newqueue;			
			newqueue = temp;
		}

		// reset marks
		for( int i = 0; i < markptr;i++ ) {
			graph->nodes[markqueue[i]].mark = 0;
		}
		graph->nodes[src_id].mark = 0;

	}

	// free allocations
	free( markqueue );
	free( queue );		
	free( newqueue );			

	return n_pivots;
}

/*
	Do a breadth first traversal of the entire graph, count the number of marked nodes
	and see if it is equal to the total number of graph nodes.
*/
int check_connectivity( GlimmerGraph *graph, int src_id ) {
	
	graph->nodes[src_id].mark	= 1;	// mark the source node

	// working queue

	int queuesize	= graph->nodes[src_id].numedges;
	int *queue		= (int *) malloc( sizeof( int ) * graph->nodes[src_id].numedges );
	queue			= (int *)memcpy(queue,graph->nodes[src_id].edges,sizeof(int)*graph->nodes[src_id].numedges);

	//printf("QUEUE: ");
	//for( int i = 0;  i < queuesize; i++ ) {
//		printf("%d ",queue[i]);
//	}
//	printf("\n");

	// marking queue

	int markstorage = 0;
	int *markqueue	= NULL;
	int markptr		= 0;

	while( queuesize ) {

		// queue for next depth of search 

		int newqueuesize	= 0;
		int newqueuememsize = 0;
		int *newqueue		= NULL;
		
		// iterate over current depth queue
		for( int i = 0; i < queuesize; i++ ) {

			graph->nodes[queue[i]].mark = 1;							// mark the node
			if( ++markptr > markstorage ) {								// resize the marked list
				if( markstorage ){
					markqueue = (int*)realloc(markqueue,sizeof(int)*markstorage*2);
					markstorage *= 2;
				} 
				else {
					markqueue = (int *)malloc(sizeof(int));
					markstorage = 1;
				}
			}
			markqueue[markptr-1]=queue[i];						// add to the marked list for later processing
		}
		for( int i = 0; i < queuesize; i++ ) {
			// check the neighborhood of the node for unmarked nodes
			for( int j = 0; j < graph->nodes[queue[i]].numedges; j++ ) {		// for each edge
				if( ! graph->nodes[graph->nodes[queue[i]].edges[j]].mark ) {	// if it is NOT marked
					graph->nodes[graph->nodes[queue[i]].edges[j]].mark = 1;		// mark it
					newqueuesize++;												// increase next depth queue
					if( newqueuesize-1 ) {
						if( newqueuememsize < newqueuesize ) {
							newqueuememsize *= 2;
							newqueue = (int*)realloc( newqueue, sizeof( int ) * newqueuememsize );
						}
					}
					else {
						newqueuememsize = 1;
						newqueue = (int*)malloc( sizeof( int ) );
					}
					//if( graph->nodes[queue[i]].edges[j] ) printf("%d.",graph->nodes[queue[i]].edges[j]);
					newqueue[newqueuesize-1]=graph->nodes[queue[i]].edges[j];	// add the node to the queue
				}
			}
		}
		free(queue);				// free the current queue
		queuesize = newqueuesize;	// set the queue to the next depth queue
		queue = newqueue;			
	}

	if( b_output_debug ) {
		for( int i = 0; i < graph->numnodes; i++ ) {
			if( ! graph->nodes[i].mark ) {
				printf( "%d unmarked\n", i );
			}
		}
	}

	// reset marks
	for( int i = 0; i < markptr;i++ ) {
		graph->nodes[markqueue[i]].mark = 0;
	}
	graph->nodes[src_id].mark = 0;

	// free allocations
	if( markqueue != NULL ) free( markqueue );
	if( queue != NULL ) free( queue );
	
	if( b_output_debug )
		printf("graph connectivity: %d vs %d\n", graph->numnodes, markptr+1);

	return (markptr+1==graph->numnodes);
}

/*
	update the distance hash
*/
inline void update_distmap( GlimmerGraph *graph, int node_a, int node_b, int dist) {
	//if( g_distmap.size( ) > n_hardlimit ) {
	//	for( int j = 0; j < 100; j++ ) {	// randomly erase our limit
	//		std::map<int,int>::iterator randiter = g_distmap.begin( );
	//		int randpos = ( myrand() % n_hardlimit );
	//		for(int k = 0; k < ( myrand() % n_hardlimit ); k++ ) randiter++;
	//		g_distmap.erase( randiter );
	//	}
	//}

	if(node_a < node_b){
		g_distmap[node_a*graph->numnodes+node_b] = dist;
	}
	else {
		g_distmap[node_b*graph->numnodes+node_a] = dist;
	}
}

/*
	Do a bfs on a glimmer graph between two nodes

	WARNING: will infinite loop if the path does not exist
*/
int bfs_glimmer( GlimmerGraph *graph, int src_id, int dest_id ) {

	// handle trivial case

	if( src_id == dest_id ) {
		//free(queue);
		return 0;
	}
	
	// have we hashed the distance previously?
	
	std::map<int,int>::iterator iter = g_distmap.find(dest_id*(graph->numnodes)+src_id);
	if( iter != g_distmap.end( ) ) return iter->second;

	//printf("BFS: %d --> %d\n", src_id, dest_id );

	bool b_notfound				= true;
	int dist					= 0;

	// working queue

	int queuesize	= graph->nodes[src_id].numedges;

	static int *queue		= NULL;
	static int *newqueue	= NULL;
	static int *markqueue	= NULL;
	static int *temp		= NULL;

	if( queue == NULL ) {
		queue		= (int*)malloc(sizeof(int)*graph->numnodes);
		newqueue	= (int*)malloc(sizeof(int)*graph->numnodes);
		markqueue	= (int*)malloc(sizeof(int)*graph->numnodes);
	}

	// setup initial queue

	queue	= (int *)memcpy(queue,graph->nodes[src_id].edges,sizeof(int)*graph->nodes[src_id].numedges);
	for( int i = 0; i < queuesize; i++ ) {
		update_distmap( graph, graph->nodes[src_id].edges[i], src_id, 1 );
	}

	// marking queue

	int markptr		= 0;

	graph->nodes[src_id].mark	= 1;	// mark thyself

	while( b_notfound ) {

		dist++; // increment depth counter

		// reset queue for next depth of search 

		int newqueuesize	= 0;
		
		// iterate over current depth queue
		for( int i = 0; i < queuesize; i++ ) {

			if( queue[i] == dest_id ) {	// if we have located our search node
				graph->nodes[queue[i]].mark = 0;							// unmark the node
				b_notfound = false;				// claim that we have found it
			}
			else {
				graph->nodes[queue[i]].mark = 1;						// mark the node
				markqueue[(++markptr)-1]=queue[i];						// add to the marked list for later processing
			}
		}
		if( b_notfound ) {
			for( int i = 0; i < queuesize; i++ ) {
				// check the neighborhood of the node for unmarked nodes
				for( int j = 0; j < graph->nodes[queue[i]].numedges; j++ ) {		// for each edge
					if( ! graph->nodes[graph->nodes[queue[i]].edges[j]].mark ) {	// if NOT marked
						update_distmap( graph, graph->nodes[queue[i]].edges[j], src_id, dist+1 );	// update the distance map
						graph->nodes[graph->nodes[queue[i]].edges[j]].mark = 1;		// mark it
						newqueuesize++;												// increase next depth queue
						newqueue[newqueuesize-1]=graph->nodes[queue[i]].edges[j];	// add node to next depth queue
					}
				}
			}
		}
		queuesize = newqueuesize;	// set the queue to the next depth queue
		temp = queue;
		queue = newqueue;
		newqueue = temp;
	}

	//printf("\tRESETTING %d MARKS...", markptr);

	// reset marks
	for( int i = 0; i < markptr;i++ ) {
		//printf("%d...", markqueue[i]);
		graph->nodes[markqueue[i]].mark = 0;
	}
	graph->nodes[src_id].mark = 0;
	//printf("\n");
	
	
	return dist;
}

void updateNode( GlimmerNode *node, int end ) {

	for( int i = 0; i < node->numedges; i++ ) {
		if( node->edges[i] == end )
			return;
	}
	node->numedges++;
	if( ! (node->numedges-1) ) {
		node->edges = (int *)malloc( sizeof( int ) );
		node->numstorage = 1;
	}
	else if(node->numedges > node->numstorage) {
			node->numstorage *= 2;	// double storage
			node->edges = (int *)realloc( node->edges, sizeof(int)*node->numstorage);
	}
	node->edges[node->numedges-1] = end;
}

GlimmerGraph *load_edge_file( const char *edgefilename ) {
	
	char line[1028];
	char item[1028];
	int i,j;
	int start,end;
	int numnodes = 0;

	// first find the number of nodes ( find the largest int )

	FILE *fp = NULL;
	fp = fopen( edgefilename, "r" );
	while( fgets( line, 1027, fp ) != NULL ) {
		j=i=0;		
		while(line[i]!=' '&&line[i]!='\t')
			item[j++]=line[i++];
		item[j]='\0';
		numnodes=atoi(item)>numnodes?atoi(item):numnodes;

		i++;
		j = 0;
		while(line[i]!='\0')
			item[j++]=line[i++];
		item[j]='\0';
		numnodes=atoi(item)>numnodes?atoi(item):numnodes;
	}	
	fclose(fp);

	numnodes++;
	GlimmerGraph *graph = (GlimmerGraph *)malloc(sizeof(GlimmerGraph));
	graph->numnodes = numnodes;
	graph->nodes = (GlimmerNode *) calloc( numnodes, sizeof( GlimmerNode ) );

	// actually read in edges
	fp = fopen( edgefilename, "r" );

	while( fgets( line, 1027, fp ) != NULL ) {
		j=i=0;		
		while(line[i]!=' '&&line[i]!='\t')
			item[j++]=line[i++];
		item[j]='\0';
		//start=atoi(item)-1;
		start=atoi(item);

		i++;
		j = 0;
		while(line[i]!='\0')
			item[j++]=line[i++];
		item[j]='\0';
		//end=atoi(item)-1;
		end=atoi(item);

		graph->nodes[start].id = start;
		graph->nodes[end].id = end;
		//printf("adding %d %d and %d %d\n", start, end, end, start );
		if( start != end ) { // don't allow self edges
			updateNode( &(graph->nodes[start]), end );
			updateNode( &(graph->nodes[end]), start );
		}
	}	

	fclose(fp);

	if( ! check_connectivity( graph, 0 ) ) {
		printf("ERROR:  Graph not fully connected.");
		exit( 0 );
	}

	return graph;
}

/*
	loads a graph from a matrix market format
	computes the floyd warshall all-points-shortest-paths (perhaps do johnson's?)
	using the initial random permutation and access patterns as a guide
*/
void procgraph( const char *matrixname, const char *outfilename) {

	int *D;
	int maxdist = 0;
	int ret_code;
    MM_typecode matcode;
    FILE *f;
    int M, N, nz;   
    int i, j, k, I, J;
    double val;
	bool b_pattern = false;

	if ((f = fopen(matrixname, "r")) == NULL) {
		printf("ERROR: Cannot open matrix filename %s\n", matrixname );
		exit(1);
	}
    if (mm_read_banner(f, &matcode) != 0) {
		printf("ERROR: Could not process Matrix Market banner for filename %s.\n", matrixname );
        exit(1);
    }
	if ((ret_code = mm_read_mtx_crd_size(f, &M, &N, &nz)) !=0) {
		printf("ERROR: Could not process Matrix Market sizes for filename %s.\n", matrixname );
        exit(1);
	}
	if( mm_is_pattern( matcode ) ) {
		b_pattern = true;
	}
	
	if( b_output_debug ) {
		printf("Allocating zero'ed %d by %d distance matrix...", M, M );	
	}
	
	D = (int *)malloc( sizeof( int ) * M * M );
	D[0]=-1;	
	i=1;
	while( i*2 < M*M ) {
		memcpy(D+i,D,sizeof(int)*i);
		i=i*2;
	}
	if( i*2 > M*M ) {
		memcpy(D+i,D,sizeof(int)*(M*M-i));
	}
	for( i = 0; i < M; i++ ) D[i*M+i]=0;


	if( D == NULL ) {
		printf("ERROR: couldn't allocate %d bytes of memory for distance matrix.\n", sizeof( int ) * M * M );
		exit( 0 );
	}

	// initialize matrix with initial edge distances
	if( b_output_debug ) {
		printf("Initializing distance matrix with initial edge distances...");
	}
	/* NOTE: when reading in doubles, ANSI C requires the use of the "l"  */
    /*   specifier as in "%lg", "%lf", "%le", otherwise errors will occur */
    /*  (ANSI C X3.159-1989, Sec. 4.9.6.2, p. 136 lines 13-15)            */

    for (i=0; i<nz; i++)
    {

		if( b_pattern )
	        fscanf(f, "%d %d\n", &I, &J);
		else 
			fscanf(f, "%d %d %lg\n", &I, &J, &val);

		if( I != J ) {
			D[(I-1)*M+(J-1)] = 1;
			D[(J-1)*M+(I-1)] = 1;
		}

    }

    fclose(f);

	if( b_output_debug ) {
		printf("done.\n");
	}


	// calculates all points shortest paths

	if( b_output_debug ) {
		printf("Starting Floyd-Warshall...");
	}
	int temp;
	for( k = 0; k < M; k++ ) {
		if( b_output_debug )
			printf("%d.",k);
		for( i = 0; i < M; i++ ) {
			for( j = 0; j < M; j++ ) {
				if( D[i*M+k] >= 0 && D[k*M+j] >= 0 ) {
					temp = D[i*M+k]+D[k*M+j];
					if( D[i*M+j] < 0 || D[i*M+j] >= temp )
						D[i*M+j] = temp;				
				}
				maxdist = (D[i*M+j]>maxdist)?D[i*M+j]:maxdist;
			}
		}
	}

	if( b_output_debug )
		printf("done.\n");

	// output file
	if( b_output_debug ) {
		printf("Writing distance matrix to disk...");
	}
	FILE *fp = NULL;
	if( (fp = fopen( outfilename, "w" ) )== NULL ) {
		printf("ERROR couldn't open %s for writing.\n", outfilename );
		exit( 0 );
	}

	for( i = 0; i < M; i++ ) {
		//for( j = 0; j < M; j++ ) {
		for( j = 0; j < i+1; j++ ) {
			fprintf(fp, "%0.3f ", ((double)D[i*M+j])/((double)maxdist));
			//fprintf(fp, "%d ", D[i*M+j]);
		}
		fprintf(fp, "\n");
	}

	if( b_output_debug )
		printf("done.\n");	

	free(D);

}
