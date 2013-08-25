#include <iostream>
#include <string>
#include <cmath>
using std::string;
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <stdio.h>
#include <cstring>
#include "texture.h"
#include "shader.h"
#include "fbo.h"
#include "vbo.h"
#include "sh_rand.h"
#include "feeder.h"

extern Feeder g_feeder;
//extern int n_waffle;
//extern int n_set_points;
//extern int n_points;
extern int n_dims;
extern int n_embed_dims;
extern int n_smax;
extern int n_vmax;
//extern Texture t_rand;
//extern GLuint vb_setBuffer;
//extern float *randr;
extern GLenum attachmentpoints[];
extern bool b_output_debug;
extern bool g_b_useDistance;
extern char g_shader_path[4092];

Shader sh_rand;			// updates the random indices
Shader sh_shuffle;		// shifts texture contents according to a permutation index
Shader sh_shuffle_index;	// shifts texture refs according to a permutation index

Uniform u_P;
Uniform u_N;
Uniform u_offset;
Uniform u_idxwidth;
Uniform u_idxspan;
Uniform u_randwidth;
Uniform u_itemcount;
Uniform u_sw;
Uniform u_bw;

//Uniform u_debug;
//Uniform u_offset_x;
//Uniform u_offset_y;
//Uniform u_width;
//Uniform u_height;
//Uniform u_itemcount;
//Uniform u_sw;
//Uniform u_bw;
//Uniform randomtexture;

Uniform u_shuffle_permutation;
Uniform u_shuffle_copytexture;
Uniform u_shuffle_permwidth;
Uniform u_shuffle_inputlength;

Uniform u_shuffle_index_permutation;
Uniform u_shuffle_index_index;
Uniform u_shuffle_index_permwidth;

int myrand( ) {

	unsigned int n = (unsigned int)rand();
	unsigned int m = (unsigned int)rand();

	return ((int)((n << 16) + m));

}

/*
	Compile and Link the randomized shaders
*/
void setup_sh_rand( ) {

	char shader_path[8192];

	// setup sh_rand
	sh_rand.program = glCreateProgramObjectARB();
	sh_rand.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_rand.program, sh_rand.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_rand.glsl");
	const GLcharARB *rand_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_rand.shader, 1, &rand_source, NULL);
	glCompileShaderARB(sh_rand.shader);
	glLinkProgramARB(sh_rand.program);
	printInfoLog( "sh_rand", sh_rand.shader );
	u_P.location = glGetUniformLocationARB(sh_rand.program, "P");
	u_N.location = glGetUniformLocationARB(sh_rand.program, "N");
	u_offset.location = glGetUniformLocationARB(sh_rand.program, "offset");
	u_idxwidth.location = glGetUniformLocationARB(sh_rand.program, "idxwidth");
	u_idxspan.location = glGetUniformLocationARB(sh_rand.program, "idxspan");
	u_randwidth.location = glGetUniformLocationARB(sh_rand.program, "randwidth");
	u_itemcount.location = glGetUniformLocationARB(sh_rand.program, "itemcount");
	u_sw.location = glGetUniformLocationARB(sh_rand.program, "sw");
	u_bw.location = glGetUniformLocationARB(sh_rand.program, "bw");


	// setup sh_shuffle
	sh_shuffle.program = glCreateProgramObjectARB();
	sh_shuffle.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_shuffle.program, sh_shuffle.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_shuffle.glsl");
	const GLcharARB *shuffle_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_shuffle.shader, 1, &shuffle_source, NULL);
	glCompileShaderARB(sh_shuffle.shader);
	glLinkProgramARB(sh_shuffle.program);
	printInfoLog( "sh_shuffle", sh_shuffle.shader );
	u_shuffle_permutation.isFloat = false;
	u_shuffle_copytexture.isFloat = false;
	u_shuffle_permwidth.isFloat = true;
	u_shuffle_inputlength.isFloat = true;
	u_shuffle_permutation.location = glGetUniformLocationARB(sh_shuffle.program, "permutation");
	u_shuffle_copytexture.location = glGetUniformLocationARB(sh_shuffle.program, "copytexture");
	u_shuffle_permwidth.location = glGetUniformLocationARB(sh_shuffle.program, "permwidth");
	u_shuffle_inputlength.location = glGetUniformLocationARB(sh_shuffle.program, "inputlength");

	// setup sh_shuffle_index
	sh_shuffle_index.program = glCreateProgramObjectARB();
	sh_shuffle_index.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_shuffle_index.program, sh_shuffle_index.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_shuffle_index.glsl");
	const GLcharARB *shuffle_index_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_shuffle_index.shader, 1, &shuffle_index_source, NULL);
	glCompileShaderARB(sh_shuffle_index.shader);
	glLinkProgramARB(sh_shuffle_index.program);
	printInfoLog( "sh_shuffle_index", sh_shuffle_index.shader );
	u_shuffle_index_permutation.isFloat = false;
	u_shuffle_index_index.isFloat = false;
	u_shuffle_index_permwidth.isFloat = true;
	u_shuffle_index_permutation.location = glGetUniformLocationARB(sh_shuffle_index.program, "permutation");
	u_shuffle_index_index.location = glGetUniformLocationARB(sh_shuffle_index.program, "index");
	u_shuffle_index_permwidth.location = glGetUniformLocationARB(sh_shuffle_index.program, "permwidth");
}

void shuffle_index( Texture *texture, Tier *tier ) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture->tier_width[tier->level],0.0,texture->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture->tier_width[tier->level],texture->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glUseProgramObjectARB( sh_shuffle_index.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx?0:1]);
	glUniform1iARB(u_shuffle_index_index.location, 1); 

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_perm->texture_name[tier->t_perm->attach_idx?0:1]);
	glUniform1iARB(u_shuffle_index_permutation.location, 2); 

	glUniform1fARB(u_shuffle_index_permwidth.location, tier->t_perm->width );

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture->attach_idx]);
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

void shuffle_texture( Texture *texture, Tier *tier ) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture->tier_width[tier->level],0.0,texture->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture->tier_width[tier->level],texture->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glUseProgramObjectARB( sh_shuffle.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx?0:1]);
	glUniform1iARB(u_shuffle_copytexture.location, 1); 

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_perm->texture_name[tier->t_perm->attach_idx?0:1]);
	glUniform1iARB(u_shuffle_permutation.location, 2); 

	glUniform1fARB(u_shuffle_inputlength.location, texture->width / tier->t_perm->width );
	glUniform1fARB(u_shuffle_permwidth.location, tier->t_perm->width );

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture->attach_idx]);
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

void random_sample( Texture *texture, Tier *tier, bool b_fix ) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture->tier_width[tier->level],0.0,texture->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture->tier_width[tier->level],texture->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glUseProgramObjectARB( sh_rand.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_rand->texture_name[tier->t_rand->attach_idx]);
	glUniform1iARB(u_P.location, 1); 

	int idxwidth = (((int)ceil(((double)n_vmax) / 4.0 )) + ((int)ceil(((double)n_smax) / 4.0 )));
	int N4 = (int)ceil(((double)tier->n_points)/4.0);
	glUniform1fARB(u_N.location, N4 );
	glUniform1fARB(u_offset.location, tier->n_waffle );
	glUniform1fARB(u_idxwidth.location, idxwidth );
	glUniform1fARB(u_idxspan.location, tier->t_idx->width / idxwidth );
	glUniform1fARB(u_randwidth.location, tier->t_rand->width );

	if( b_fix ) {
		glUniform1fARB(u_itemcount.location, tier->t_perm->tier_width[tier->level+1]*tier->t_perm->tier_height[tier->level+1] );
		glUniform1fARB(u_sw.location, tier->t_perm->tier_width[tier->level+1] );
		glUniform1fARB(u_bw.location, tier->t_perm->width );
	}
	else {
		glUniform1fARB(u_itemcount.location, (tier->level==0)?tier->n_points:(tier->t_perm->tier_width[tier->level]*tier->t_perm->tier_height[tier->level]) );
		glUniform1fARB(u_sw.location, tier->t_perm->tier_width[tier->level] );
		glUniform1fARB(u_bw.location, tier->t_perm->width );
	}

	tier->n_waffle= (tier->n_waffle+1)%tier->n_points;

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	int test = 0;
	if( tier->b_update_s ) {
		glDisableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_VERTEX_ARRAY);
		glEnable(GL_TEXTURE_COORD_ARRAY);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_ssetBuffer);
		glTexCoordPointer( 2, GL_FLOAT, 0, 0 );
		glVertexPointer( 2, GL_FLOAT, 0, 0 );
		glDrawArrays(GL_QUADS, 0, tier->n_setPoints);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, NULL);
		glDisable(GL_VERTEX_ARRAY);
		glDisable(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
	}
	else {
		glDisableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_VERTEX_ARRAY);
		glEnable(GL_TEXTURE_COORD_ARRAY);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_vsetBuffer);
		glTexCoordPointer( 2, GL_FLOAT, 0, 0 );
		glVertexPointer( 2, GL_FLOAT, 0, 0 );
		glDrawArrays(GL_QUADS, 0, tier->n_setPoints);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, NULL);
		glDisable(GL_VERTEX_ARRAY);
		glDisable(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		glUniform1fARB(u_offset.location, tier->n_waffle );
		tier->n_waffle= (tier->n_waffle+1)%tier->n_points;

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnable(GL_VERTEX_ARRAY);
		glEnable(GL_TEXTURE_COORD_ARRAY);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, tier->vb_ssetBuffer);
		glTexCoordPointer( 2, GL_FLOAT, 0, 0 );
		glVertexPointer( 2, GL_FLOAT, 0, 0 );
		glDrawArrays(GL_QUADS, 0, tier->n_setPoints);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, NULL);
		glDisable(GL_VERTEX_ARRAY);
		glDisable(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		//// and render quad
		//glBegin(GL_QUADS);
		//	glTexCoord2f(0.0, 0.0); 
		//	glVertex2f(0.0, 0.0);
		//	glTexCoord2f(texture->tier_width[tier->level], 0.0); 
		//	glVertex2f(texture->tier_width[tier->level], 0.0);
		//	glTexCoord2f(texture->tier_width[tier->level], texture->tier_height[tier->level]); 
		//	glVertex2f(texture->tier_width[tier->level], texture->tier_height[tier->level]);
		//	glTexCoord2f(0.0, texture->tier_height[tier->level]); 
		//	glVertex2f(0.0, texture->tier_height[tier->level]);
		//glEnd();
	}
}

void swap_helper( float *resource, int i, int j ) {
	float foo = resource[i];
	resource[i] = resource[j];
	resource[j] = foo;
}

void swap_helper( int *resource, int i, int j ) {
	int foo = resource[i];
	resource[i] = resource[j];
	resource[j] = foo;
}

/*
	Generate a random permutation of the numbers between 1 and N
*/
void initPerm( Tier *tier ) {
	
	int i = 0;
	int n_dim_bins = (int) ceil( (double) n_dims / (double) 4 );
	if( ! g_b_useDistance ) {
		tier->t_perm->width = ( tier->t_pts->width / n_dim_bins );
		tier->t_perm->height = tier->t_pts->height;
	}
	else {
		tier->t_perm->width = tier->t_embed->width;
		tier->t_perm->height = tier->t_embed->height;
	}
	// generate the actual data

	//printf("\n\nperm size = %d\n\n", (tier->t_perm->width * tier->t_perm->height  * 4) );
	//printf("\n\npoint count = %d\n\n", tier->n_points );

	tier->perm = (float *) malloc( sizeof( float ) * tier->t_perm->width * tier->t_perm->height  * 4 );	
	tier->reference = (float *) malloc( sizeof( float ) * tier->t_perm->width * tier->t_perm->height  * 4 );	

	for( i = 0; i < tier->n_points; i++ ) {
		tier->perm[i*4] = (float)i;
		tier->perm[i*4+1] = 0.f;
		tier->perm[i*4+2] = 0.f;
		tier->perm[i*4+3] = 0.f;
	}
	if( tier->n_points < tier->t_perm->width*tier->t_perm->height ) {
		int offset = tier->t_perm->width*tier->t_perm->height - tier->n_points;
		for( i = 0; i < offset; i++ ) {
			tier->perm[ (i + tier->n_points) * 4 ] = 0.0f;
			tier->perm[ (i + tier->n_points) * 4 + 1 ] = 0.0f;
			tier->perm[ (i + tier->n_points) * 4 + 2 ] = 0.0f;
			tier->perm[ (i + tier->n_points) * 4 + 3 ] = 0.0f;
		}
	}
	// permute it around 

	for( i = 0; i < tier->n_points; i++ ) {
		int a = i*4;
		int b = i*4+((myrand() %(tier->n_points-i))*4);
		//printf("swap %d, %d\n", a, b );
		swap_helper( tier->perm, a, b );
	}
}

/*
	Constructs a randomly sampled index buffer using each index exactly
	v_max+s_max times
*/
void initRand( Tier *tier )  {

	int i;	 

	// construct the initial random permutation of size N using a KNUTH SHUFFLE

	int Plimit = (tier->n_points / n_smax + ((tier->n_points%n_smax)?1:0) )*n_smax;
	g_feeder.P = (int *)malloc( sizeof( int ) * Plimit);
	for( i=0; i < tier->n_points; i++ ) {
		g_feeder.P[i] = i;
	}
	for( i = 0; i < tier->n_points-1; i++ ) {
		//swap_helper( g_feeder.P, i, myrand() % tier->n_points );	// incorrect old way
		swap_helper( g_feeder.P, i, i+(myrand() % (tier->n_points-i)) );	// correct new way
	}
	for( i = tier->n_points; i < Plimit; i++ ) {
		g_feeder.P[i] = g_feeder.P[ i % tier->n_points ];
	}

	 // properly size the random texture

	tier->t_rand->width  = (int) ceil(sqrt((double)(Plimit / 4)));
	tier->t_rand->height = (int) ceil(sqrt((double)(Plimit / 4)));
	tier->n_waffle = 0;

	// copy it into a buffer consecutively |S| times

	tier->randr = (float *)malloc( sizeof(float) * 4 * tier->t_rand->width * tier->t_rand->height );
	for( i = 0; i < tier->t_rand->width*tier->t_rand->height*4; i++ ) {
		tier->randr[i] = (float) g_feeder.P[i%Plimit];
	}
}
