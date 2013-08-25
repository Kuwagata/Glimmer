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
#include <cstring>
#include <stdio.h>
#include "texture.h"
#include "shader.h"
#include "fbo.h"
#include "vbo.h"

//extern int n_set_points;
extern int n_dims;
extern int n_embed_dims;
extern int n_smax;
extern int n_vmax;
//extern Texture t_idx;
//extern Texture t_pts;
//extern Texture t_embed;
//extern GLuint vb_setBuffer;
extern GLenum attachmentpoints[];
extern char g_shader_path[4092];

Shader sh_odd_sort;		// sorts the textures according a key (odd pass)
Shader sh_even_sort;	// sorts the textures according a key (even pass)
Shader sh_dup;			// checks for duplicate neighbor indices

Shader sh_near_update_gen;	// generic near update sort routine
Shader sh_near_update_dup;	// index set near update sort routine
Shader sh_near_update_dst;	// high D near update sort routine

Uniform u_nug_key_tex;		// near update generic, key texture
Uniform u_nug_value_tex;	// near update generic, value texture

Uniform u_nud_dup_key;		// near update index, index texture key
Uniform u_nud_dst_key;		// near update index, high D texture key
Uniform u_nuh_dup_key;		// near update high D, index texture key
Uniform u_nuh_dst_key;		// near update high D, high D texture key

Uniform u_osort_key;
Uniform u_osort_value;
Uniform u_osort_distwidth;
Uniform u_esort_key;
Uniform u_esort_value;
Uniform u_esort_distwidth;

Uniform u_dup_key;
Uniform u_dup_value;
Uniform u_dup_distwidth;
Uniform u_dup_idxwidth;

/*
	Compile together various near-update shaders
*/
void setup_sh_sort( ) {

	char shader_path[8192];

	//setup sh_near_update_gen
	sh_near_update_gen.program = glCreateProgramObjectARB();
	sh_near_update_gen.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_near_update_gen.program, sh_near_update_gen.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_near_update_gen.glsl");
	const GLcharARB *near_update_gen_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_near_update_gen.shader, 1, &near_update_gen_source, NULL);
	glCompileShaderARB(sh_near_update_gen.shader);
	glLinkProgramARB(sh_near_update_gen.program);
	printInfoLog( "sh_near_update_gen", sh_near_update_gen.shader );
	u_nug_key_tex.location		= glGetUniformLocationARB( sh_near_update_gen.program, "key_tex" );
	u_nug_value_tex.location	= glGetUniformLocationARB( sh_near_update_gen.program, "value_tex" );

	////setup sh_near_update_dst
	//sh_near_update_dst.program = glCreateProgramObjectARB();
	//sh_near_update_dst.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	//glAttachObjectARB (sh_near_update_dst.program, sh_near_update_dst.shader);
	//sprintf(shader_path, "%s%s", g_shader_path, "sh_near_update_dst.glsl");
	//const GLcharARB *near_update_dst_source = readShaderSource(shader_path);
	//glShaderSourceARB(sh_near_update_dst.shader, 1, &near_update_dst_source, NULL);
	//glCompileShaderARB(sh_near_update_dst.shader);
	//glLinkProgramARB(sh_near_update_dst.program);
	//printInfoLog( "sh_near_update_dst", sh_near_update_dst.shader );
	//u_nuh_dup_key.location		= glGetUniformLocationARB( sh_near_update_dst.program, "dup_key" );
	//u_nuh_dst_key.location		= glGetUniformLocationARB( sh_near_update_dst.program, "dst_key" );

	////setup sh_near_update_dup
	//sh_near_update_dup.program = glCreateProgramObjectARB();
	//sh_near_update_dup.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	//glAttachObjectARB (sh_near_update_dup.program, sh_near_update_dup.shader);
	//sprintf(shader_path, "%s%s", g_shader_path, "sh_near_update_dup.glsl");
	//const GLcharARB *near_update_dup_source = readShaderSource(shader_path);
	//glShaderSourceARB(sh_near_update_dup.shader, 1, &near_update_dup_source, NULL);
	//glCompileShaderARB(sh_near_update_dup.shader);
	//glLinkProgramARB(sh_near_update_dup.program);
	//printInfoLog( "sh_near_update_dup", sh_near_update_dup.shader );
	//u_nud_dup_key.location		= glGetUniformLocationARB( sh_near_update_dup.program, "dup_key" );
	//u_nud_dst_key.location		= glGetUniformLocationARB( sh_near_update_dup.program, "dst_key" );

	//setup sh_odd_sort
	sh_odd_sort.program = glCreateProgramObjectARB();
	sh_odd_sort.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_odd_sort.program, sh_odd_sort.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_odd_sort.glsl");
	const GLcharARB *odd_sort_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_odd_sort.shader, 1, &odd_sort_source, NULL);
	glCompileShaderARB(sh_odd_sort.shader);
	glLinkProgramARB(sh_odd_sort.program);
	printInfoLog( "sh_odd_sort", sh_odd_sort.shader );
	u_osort_key.isFloat = false;
	u_osort_value.isFloat = false;
	u_osort_distwidth.isFloat = true;
	u_osort_key.location = glGetUniformLocationARB( sh_odd_sort.program, "key" );
	u_osort_value.location = glGetUniformLocationARB( sh_odd_sort.program, "value" );
	u_osort_distwidth.location = glGetUniformLocationARB( sh_odd_sort.program, "distwidth" );
	u_osort_distwidth.valuef = (float)(((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 )));


	//setup sh_even_sort
	sh_even_sort.program = glCreateProgramObjectARB();
	sh_even_sort.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_even_sort.program, sh_even_sort.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_even_sort.glsl");
	const GLcharARB *even_sort_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_even_sort.shader, 1, &even_sort_source, NULL);
	glCompileShaderARB(sh_even_sort.shader);
	glLinkProgramARB(sh_even_sort.program);
	printInfoLog( "sh_even_sort", sh_even_sort.shader );
	u_esort_key.isFloat = false;
	u_esort_value.isFloat = false;
	u_esort_distwidth.isFloat = true;
	u_esort_key.location = glGetUniformLocationARB( sh_even_sort.program, "key" );
	u_esort_value.location = glGetUniformLocationARB( sh_even_sort.program, "value" );
	u_esort_distwidth.location = glGetUniformLocationARB( sh_even_sort.program, "distwidth" );
	u_esort_distwidth.valuef = (float)(((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 )));

	//setup shader sh_dup
	sh_dup.program = glCreateProgramObjectARB();
	sh_dup.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_dup.program, sh_dup.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_dup.glsl");
	const GLcharARB *dup_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_dup.shader, 1, &dup_source, NULL);
	glCompileShaderARB(sh_dup.shader);
	glLinkProgramARB(sh_dup.program);
	printInfoLog( "sh_dup", sh_dup.shader );
	u_dup_key.isFloat = false;
	u_dup_value.isFloat = false;
	u_dup_distwidth.isFloat = true;
	u_dup_idxwidth.isFloat = true;
	u_dup_key.location = glGetUniformLocationARB( sh_dup.program, "key" );
	u_dup_value.location = glGetUniformLocationARB( sh_dup.program, "value" );
	u_dup_distwidth.location = glGetUniformLocationARB( sh_dup.program, "distwidth" );
	u_dup_idxwidth.location = glGetUniformLocationARB( sh_dup.program, "idxwidth" );
	u_dup_distwidth.valuef = (float)(((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 )));

}

void near_update_gen( Texture *texture_key, Tier *tier ) {

	//clock_t time1;

	//time1 = clock();
	// flip the generic texture

	tier->t_g->attach_idx = tier->t_g->attach_idx?0:1;

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,tier->t_g->tier_width[tier->level],0.0,tier->t_g->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,tier->t_g->tier_width[tier->level],tier->t_g->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_g->frame_buffer);
	glUseProgramObjectARB( sh_near_update_gen.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_g->texture_name[tier->t_g->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_key->texture_name[texture_key->attach_idx]);
	glUniform1iARB(u_nug_key_tex.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_g->texture_name[tier->t_g->attach_idx?0:1]);
	glUniform1iARB(u_nug_value_tex.location, 2); 

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[tier->t_g->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	// and render quad
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); 
		glVertex2f(0.0, 0.0);
		glTexCoord2f(tier->t_g->tier_width[tier->level], 0.0); 
		glVertex2f(tier->t_g->tier_width[tier->level], 0.0);
		glTexCoord2f(tier->t_g->tier_width[tier->level], tier->t_g->tier_height[tier->level]); 
		glVertex2f(tier->t_g->tier_width[tier->level], tier->t_g->tier_height[tier->level]);
		glTexCoord2f(0.0, tier->t_g->tier_height[tier->level]); 
		glVertex2f(0.0, tier->t_g->tier_height[tier->level]);
	glEnd();
	//glFinish();
	//printf("\t\tA = %d\n", clock()-time1);
	//exit( 0 );
	//time1 = clock();
	Texture *t1 = tier->t_d;
	Texture *t2 = tier->t_idx;
	if( texture_key == tier->t_d ) {
		t1 = tier->t_idx;
		t2 = tier->t_d;
	}
	
	// flip the generic texture

	t1->attach_idx = t1->attach_idx?0:1;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0,t1->tier_width[tier->level],0.0,t1->tier_height[tier->level]);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0,0,t1->tier_width[tier->level],t1->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,t1->frame_buffer);
	glUseProgramObjectARB( sh_near_update_gen.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,t1->texture_name[t1->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,t2->texture_name[t2->attach_idx]);
	glUniform1iARB(u_nug_key_tex.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,t1->texture_name[t1->attach_idx?0:1]);
	glUniform1iARB(u_nug_value_tex.location, 2); 

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[t1->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	// and render quad
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); 
		glVertex2f(0.0, 0.0);
		glTexCoord2f(t1->tier_width[tier->level], 0.0); 
		glVertex2f(t1->tier_width[tier->level], 0.0);
		glTexCoord2f(t1->tier_width[tier->level], t1->tier_height[tier->level]); 
		glVertex2f(t1->tier_width[tier->level], t1->tier_height[tier->level]);
		glTexCoord2f(0.0, t1->tier_height[tier->level]); 
		glVertex2f(0.0, t1->tier_height[tier->level]);
	glEnd();
	//glFinish();
	//printf("\t\tB = %d\n", clock()-time1);
	//time1 = clock();
	// flip the generic texture

	t2->attach_idx = t2->attach_idx?0:1;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0,t2->tier_width[tier->level],0.0,t2->tier_height[tier->level]);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0,0,t2->tier_width[tier->level],t2->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,t2->frame_buffer);
	glUseProgramObjectARB( sh_near_update_gen.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,t2->texture_name[t2->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,t2->texture_name[t2->attach_idx?0:1]);
	glUniform1iARB(u_nug_key_tex.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,t2->texture_name[t2->attach_idx?0:1]);
	glUniform1iARB(u_nug_value_tex.location, 2); 

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[t2->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	// and render quad
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); 
		glVertex2f(0.0, 0.0);
		glTexCoord2f(t2->tier_width[tier->level], 0.0); 
		glVertex2f(t2->tier_width[tier->level], 0.0);
		glTexCoord2f(t2->tier_width[tier->level], t2->tier_height[tier->level]); 
		glVertex2f(t2->tier_width[tier->level], t2->tier_height[tier->level]);
		glTexCoord2f(0.0, t2->tier_height[tier->level]); 
		glVertex2f(0.0, t2->tier_height[tier->level]);
	glEnd();
	//glFinish();
	//printf("\t\tC = %d\n", clock()-time1);
}

void sort_even( Texture *texture_value, Texture *texture_key, Tier *tier ) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture_value->tier_width[tier->level],0.0,texture_value->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture_value->tier_width[tier->level],texture_value->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture_value->frame_buffer);
	glUseProgramObjectARB( sh_even_sort.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_value->texture_name[texture_value->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	if( texture_key == texture_value )
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_key->texture_name[texture_key->attach_idx?0:1]);
	else 
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_key->texture_name[texture_key->attach_idx]);
	glUniform1iARB(u_esort_key.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_value->texture_name[texture_value->attach_idx?0:1]);
	glUniform1iARB(u_esort_value.location, 2); 
	glUniform1fARB(u_esort_distwidth.location, u_esort_distwidth.valuef );		

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture_value->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	// and render quad
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); 
		glVertex2f(0.0, 0.0);
		glTexCoord2f(texture_value->tier_width[tier->level], 0.0); 
		glVertex2f(texture_value->tier_width[tier->level], 0.0);
		glTexCoord2f(texture_value->tier_width[tier->level], texture_value->tier_height[tier->level]); 
		glVertex2f(texture_value->tier_width[tier->level], texture_value->tier_height[tier->level]);
		glTexCoord2f(0.0, texture_value->tier_height[tier->level]); 
		glVertex2f(0.0, texture_value->tier_height[tier->level]);
	glEnd();

}

void sort_odd( Texture *texture_value, Texture *texture_key, Tier *tier ) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture_value->tier_width[tier->level],0.0,texture_value->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture_value->tier_width[tier->level],texture_value->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture_value->frame_buffer);
	glUseProgramObjectARB( sh_odd_sort.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_value->texture_name[texture_value->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	if( texture_key == texture_value )
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_key->texture_name[texture_key->attach_idx?0:1]);
	else 
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_key->texture_name[texture_key->attach_idx]);
	glUniform1iARB(u_osort_key.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_value->texture_name[texture_value->attach_idx?0:1]);
	glUniform1iARB(u_osort_value.location, 2); 
	glUniform1fARB(u_osort_distwidth.location, u_osort_distwidth.valuef );		

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture_value->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	// and render quad
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); 
		glVertex2f(0.0, 0.0);
		glTexCoord2f(texture_value->tier_width[tier->level], 0.0); 
		glVertex2f(texture_value->tier_width[tier->level], 0.0);
		glTexCoord2f(texture_value->tier_width[tier->level], texture_value->tier_height[tier->level]); 
		glVertex2f(texture_value->tier_width[tier->level], texture_value->tier_height[tier->level]);
		glTexCoord2f(0.0, texture_value->tier_height[tier->level]); 
		glVertex2f(0.0, texture_value->tier_height[tier->level]);
	glEnd();

}

void mark_duplicates( Texture *texture_value, Texture *texture_key, Tier *tier ) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture_value->tier_width[tier->level],0.0,texture_value->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture_value->tier_width[tier->level],texture_value->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture_value->frame_buffer);
	glUseProgramObjectARB( sh_dup.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_value->texture_name[texture_value->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	if( texture_key == texture_value )
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_key->texture_name[texture_key->attach_idx?0:1]);
	else 
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_key->texture_name[texture_key->attach_idx]);
	glUniform1iARB(u_dup_key.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture_value->texture_name[texture_value->attach_idx?0:1]);
	glUniform1iARB(u_dup_value.location, 2); 
	glUniform1fARB(u_dup_distwidth.location, u_dup_distwidth.valuef );		
	glUniform1fARB(u_dup_idxwidth.location, tier->t_perm->tier_width[0] );

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture_value->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	// and render quad
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); 
		glVertex2f(0.0, 0.0);
		glTexCoord2f(texture_value->tier_width[tier->level], 0.0); 
		glVertex2f(texture_value->tier_width[tier->level], 0.0);
		glTexCoord2f(texture_value->tier_width[tier->level], texture_value->tier_height[tier->level]); 
		glVertex2f(texture_value->tier_width[tier->level], texture_value->tier_height[tier->level]);
		glTexCoord2f(0.0, texture_value->tier_height[tier->level]); 
		glVertex2f(0.0, texture_value->tier_height[tier->level]);
	glEnd();

}
