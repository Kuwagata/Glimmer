#include <iostream>
#include <string>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <cstring>
#include <stdio.h>
#include "vbo.h"
#include "texture.h"
#include "mmio.h"

/*
	External variables
*/

extern int g_n_vcycle_count;
extern int n_vmax;					
extern int n_smax;					

/* 
	table of distinguishable Tableau colors
*/
float tableau[10][3] =	{	{37,	139,	193},	// blue
							{254,	151,	41},	// orange
							{50,	171,	90},	// green
							{223,	61,		51},	// red
							{166,	127,	190},	// purple
							{157,	106,	94},	// brown
							{232,	151,	199},	// pink
							{145,	144,	144},	// grey
							{199,	197,	43},	// gold
							{5,		199,	215}	// teal
						};

/* 
	table of diverging color-brewer colors
*/
float color_brewer_diverging[8][3]=	{	{230,	97,		1},		// orange
										{94,	60,		153},	// purple
										{166,	97,		26},	// brown
										{0,		136,	55},	// green
										{208,	28,		139},	// pink
										{202,	0,		32},	// red
										{5,		113,	176},	// blue
										{64,	64,		64}		// gray
									};

/* 
	table of diverging color-brewer colors
*/
float primary_colors[7][3]=	{	{255,	0,		0},		// red
								{0,		0,		255},	// blue
								{0,		200,	30},	// green
								{255,	0,		255},	// magenta
								{0,		0,		0},		// black
								{128,	128,	128},	// grey
								{170,	150,	70},		// "brown"
							};

/*
	Setup vertex buffers
*/
void initVBO( Tier *tier, const char *colors_filename ) {
	
	// allocate and initialize CPU data

	float *vertices = (float *)malloc( sizeof(float) * (tier->t_embed->width*tier->t_embed->height) * 4);
	float *colors	= (float *)malloc( sizeof(float) * (tier->t_embed->width*tier->t_embed->height) * 4);
	float f_pt_count = (float) (tier->t_embed->width*tier->t_embed->height);
	//float *vertices = (float *)malloc( sizeof(float) * (tier->n_points) * 4);
	//float *colors	= (float *)malloc( sizeof(float) * (tier->n_points) * 4);
	tier->n_setPoints = ((tier->t_d->width) / ((n_vmax+n_smax)/4)) * 4;
	float *sset		= (float *)malloc( sizeof(float) * (tier->n_setPoints) * 2);
	float *vset		= (float *)malloc( sizeof(float) * (tier->n_setPoints) * 2);

	if( colors_filename != NULL ) {
		char line[1028];
		FILE *fp=fopen(colors_filename,"r");
		int pt = 0;
		int n_class = 0;
		while( fgets(line,1027,fp) != NULL ) {
			n_class = atoi(line)-1;
			//if( n_class > 5 ) {
			//	printf("ERROR: too many classes\n");
			//	exit( 0 );
			//}				
			colors[pt*4+0] = (1.f/255.f)*tableau[n_class][0];		
			colors[pt*4+1] = (1.f/255.f)*tableau[n_class][1];		
			colors[pt*4+2] = (1.f/255.f)*tableau[n_class][2];		
			colors[pt*4+3] = 1.f;
			pt++;
		}
		fclose(fp);

		if( g_n_vcycle_count ) {
			float *colors_temp	= (float *)malloc( sizeof(float) * (tier->t_embed->width*tier->t_embed->height) * 4);
			for( int i = 0; i < (tier->n_points); i++ ) {
				colors_temp[i*4+0] = colors[((int)tier->perm[i*4])*4+0];
				colors_temp[i*4+1] = colors[((int)tier->perm[i*4])*4+1];
				colors_temp[i*4+2] = colors[((int)tier->perm[i*4])*4+2];
				colors_temp[i*4+3] = colors[((int)tier->perm[i*4])*4+3];
			}
			free( colors );
			colors = colors_temp;
		}
	}

	for( int i = 0; i < (tier->n_points)*4; i++ ) {
		vertices[ i ] = 0.f;//((float)rand()) / ((float)RAND_MAX) - 0.5f;
		if( (i % 4) > 2 ) {
			vertices[ i ] = 1.0f;
		}
		if( colors_filename == NULL ) {
			//green
			//if( (i % 4)!=3 && (i % 4)!=1 )
			//	colors[i]=0.f;
			//else if( (i % 4)==1 || (i % 4)==3 ) 
			//	colors[i]=1.f;

			//black
			if( (i % 4)!=3 )
				colors[i]=0.f;
			else 
				colors[i]=1.f;
		}

		//printf("%f\n", vertices[ i ]);
		//if( (i % 4) > 1 ) {
		//	colors[ i ] = ((float)i/4.f)/f_pt_count;//1.0f;
		//}
		//else {
		//	if(i % 4) {
		//		colors[ i ] = 1.0;//(float) ( (i/4) / (tier->t_embed->width) );
		//	}
		//	else {
		//		colors[ i ] = 1.0;///*500.f*((float)i/(float)n_points)-1.f;*/ (float) ( (i/4) % (tier->t_embed->width) );
		//		//printf("colors[%d]=%f of %d\n",i,colors[i],n_points);
		//	}
		//}
	}

	int k = 0;
	for( int i = 0; i < tier->t_d->width; i+= ((n_vmax+n_smax)/4)) {
		sset[k++] = (float)(i+(n_vmax/4));
		sset[k++] = 0.f;
		sset[k++] = (float)(i+((n_vmax+n_smax)/4));
		sset[k++] = 0.f;
		sset[k++] = (float)(i+((n_vmax+n_smax)/4));
		sset[k++] = (float)tier->t_d->height;
		sset[k++] = (float)(i+(n_vmax/4));
		sset[k++] = (float)tier->t_d->height;
	}
	k=0;
	for( int i = 0; i < tier->t_d->width; i+= ((n_vmax+n_smax)/4)) {
		vset[k++] = 0.f;
		vset[k++] = 0.f;
		vset[k++] = (float)(i+(n_vmax/4));
		vset[k++] = 0.f;
		vset[k++] = (float)(i+(n_vmax/4));
		vset[k++] = (float)tier->t_d->height;
		vset[k++] = 0.f;
		vset[k++] = (float)tier->t_d->height;
	}

	// construct vertex buffers

	glGenBuffersARB(1, &(tier->vb_vertexBuffer));
	glGenBuffersARB(1, &(tier->vb_colorBuffer));
	glGenBuffersARB(1, &(tier->vb_ssetBuffer));
	glGenBuffersARB(1, &(tier->vb_vsetBuffer));

	// read data into GPU

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_vertexBuffer);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float) * (tier->t_embed->width*tier->t_embed->height) * 4, vertices, GL_STREAM_DRAW_ARB);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_colorBuffer);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float) * (tier->t_embed->width*tier->t_embed->height) * 4, colors, GL_STATIC_DRAW_ARB);
	//glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_vertexBuffer);
	//glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float) * (tier->n_points) * 4, vertices, GL_STREAM_DRAW_ARB);
	//glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_colorBuffer);
	//glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float) * (tier->n_points) * 4, colors, GL_STATIC_DRAW_ARB);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_ssetBuffer);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float) * (tier->n_setPoints) * 2, sset, GL_STATIC_DRAW_ARB);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_vsetBuffer);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float) * (tier->n_setPoints) * 2, vset, GL_STATIC_DRAW_ARB);

	// free CPU data

	//free( vertices );
	//free( colors );
	free( sset );
	free( vset );
}

void initEdgesAdj( Tier *tier, vtx_data *graph, int n ) {
	int i,j,k;
	int numedges = 0;

	for( i = 0; i < n; i++ ) {
		numedges += graph[i].nedges;
	}

	tier->n_edgecount=numedges;
	float *edges = (float *)calloc( numedges * 4 * 2, sizeof(float) );

	k = 0;
	for( int i = 0; i < n; i++ ) {
		for( j = 0; j < graph[i].nedges; j++ ) {
			edges[ k*4*2+0 ] = (float)(i%tier->t_embed->width);
			edges[ k*4*2+1 ] = (float)(i/tier->t_embed->width);
			edges[ k*4*2+4 ] = (float)((graph[i].edges[j])%tier->t_embed->width);
			edges[ k*4*2+5 ] = (float)((graph[i].edges[j])/tier->t_embed->width);
			k++;
		}
	}

	// upload data to GPU

	glGenBuffersARB(1, &(tier->vb_edgeBuffer));
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_edgeBuffer);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float) * numedges * 2 * 4, edges, GL_STREAM_DRAW_ARB);

	free(edges);
}

/*
	load edges from a graph adjacency file
*/
void initEdgesAdj( Tier *tier, const char *adjfilename ) {
	char line[1028];
	char item[1028];
	int i,j;
	int start,end;
	int numedges = 0;

	// first find the number of nodes ( find the largest int )

	FILE *fp = NULL;
	fp = fopen( adjfilename, "r" );
	while( fgets( line, 1027, fp ) != NULL ) {
		numedges++;
	}	
	fclose(fp);

	tier->n_edgecount=numedges;
	float *edges = (float *)calloc( numedges * 4 * 2, sizeof(float) );

	// actually read in edges
	fp = fopen( adjfilename, "r" );

	numedges = 0;
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

		edges[ numedges*4*2+0 ] = (float)(start%tier->t_embed->width);
		edges[ numedges*4*2+1 ] = (float)(start/tier->t_embed->width);
		edges[ numedges*4*2+4 ] = (float)(end%tier->t_embed->width);
		edges[ numedges*4*2+5 ] = (float)(end/tier->t_embed->width);
		
		numedges++;
	}	

	fclose(fp);

	// upload data to GPU

	glGenBuffersARB(1, &(tier->vb_edgeBuffer));
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_edgeBuffer);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float) * numedges * 2 * 4, edges, GL_STREAM_DRAW_ARB);

	free(edges);
}

void initEdges( Tier *tier, const char *matrixname ) {
	int ret_code;
    MM_typecode matcode;
    FILE *f;
    int M, N, nz;   
    int i,I, J;
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

	float *edges = (float *)calloc( nz * 4 * 2, sizeof(float) );


	if( edges == NULL ) {
		printf("ERROR: couldn't allocate %d bytes of memory for distance matrix.\n", sizeof( float ) * nz * 4 * 2 );
		exit( 0 );
	}

	/* NOTE: when reading in doubles, ANSI C requires the use of the "l"  */
    /*   specifier as in "%lg", "%lf", "%le", otherwise errors will occur */
    /*  (ANSI C X3.159-1989, Sec. 4.9.6.2, p. 136 lines 13-15)            */

	tier->n_edgecount=nz;
    for (i=0; i<nz; i++)
    {

		if( b_pattern )
	        fscanf(f, "%d %d\n", &I, &J);
		else 
			fscanf(f, "%d %d %lg\n", &I, &J, &val);

		if( I != J ) {
			edges[ i*4*2+0 ] = (float)((I-1)%tier->t_embed->width);
			edges[ i*4*2+1 ] = (float)((I-1)/tier->t_embed->width);
			edges[ i*4*2+4 ] = (float)((J-1)%tier->t_embed->width);
			edges[ i*4*2+5 ] = (float)((J-1)/tier->t_embed->width);
		}

    }

    fclose(f);

	// upload data to GPU

	glGenBuffersARB(1, &(tier->vb_edgeBuffer));
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_edgeBuffer);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float) * nz * 2 * 4, edges, GL_STREAM_DRAW_ARB);

	free(edges);
}
