#include <iostream>
#include <string>
#include <cmath>
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
#include "sh_force.h"

float *embed_bounds = NULL;

//extern int n_points;
//extern int n_set_points;
extern int n_dims;
extern int n_embed_dims;
extern int n_smax;
extern int n_vmax;
extern int n_current_pass;
extern bool g_b_useWeighting;
extern float g_f_minWeightDist;
extern float g_f_spacing;
/*extern Texture t_idx;
extern Texture t_g;
extern Texture t_d;
extern Texture t_sum;
extern Texture t_pts;
extern Texture t_embed;
extern Texture t_force;
extern Texture t_velocity;
extern GLuint vb_setBuffer;*/
extern char g_shader_path[4092];
extern GLenum attachmentpoints[];

Shader sh_force_sum;	// sums forces based on d metric
Shader sh_force_sum_graph;	// sums forces based on d metric with weight 1/d^2
Shader sh_force_cons;	// performs force conservation
Shader sh_integrate;	// Euler integrates force into velocity
Shader sh_move;			// Actually apply the velocity to the points

Uniform u_sum_idx;
Uniform u_sum_g;
Uniform u_sum_d;
Uniform u_sum_embed;
Uniform u_sum_old;
Uniform u_sum_velocity;
Uniform u_sum_pass;
Uniform u_sum_binsperpt;
Uniform u_sum_embedwidth;
Uniform u_sum_distwidth;
Uniform u_sum_springforce;
Uniform u_sum_damping;
Uniform u_sum_finalpass;
Uniform u_sum_sizefactor;
Uniform u_sum_debug;
Uniform u_sum_weight;

Uniform u_sum_graph_idx;
Uniform u_sum_graph_g;
Uniform u_sum_graph_d;
Uniform u_sum_graph_embed;
Uniform u_sum_graph_old;
Uniform u_sum_graph_velocity;
Uniform u_sum_graph_pass;
Uniform u_sum_graph_binsperpt;
Uniform u_sum_graph_embedwidth;
Uniform u_sum_graph_distwidth;
Uniform u_sum_graph_springforce;
Uniform u_sum_graph_damping;
Uniform u_sum_graph_finalpass;
Uniform u_sum_graph_sizefactor;
Uniform u_sum_graph_debug;
Uniform u_sum_graph_weight;

Uniform u_cons_idx;
Uniform u_cons_d;
Uniform u_cons_sum;
Uniform u_cons_old;
Uniform u_cons_pass;
Uniform u_cons_binsperpt;
Uniform u_cons_embedwidth;
Uniform u_cons_distwidth;

Uniform u_integrate_force;
Uniform u_integrate_old;
Uniform u_integrate_freeness;
Uniform u_integrate_deltatime;

Uniform u_move_velocity;
Uniform u_move_old;
Uniform u_move_deltatime;

extern bool one_pass;

void setup_sh_force( ) {

	char shader_path[8192];

	//setup shader sh_force_sum
	sh_force_sum.program = glCreateProgramObjectARB();
	sh_force_sum.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_force_sum.program, sh_force_sum.shader);
	if( one_pass )
		sprintf(shader_path, "%s%s", g_shader_path, "sh_force_sum_four.glsl");
	else 
		sprintf(shader_path, "%s%s", g_shader_path, "sh_force_sum.glsl");
	const GLcharARB *force_sum_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_force_sum.shader, 1, &force_sum_source, NULL);
	glCompileShaderARB(sh_force_sum.shader);
	glLinkProgramARB(sh_force_sum.program);
	printInfoLog( "sh_force_sum", sh_force_sum.shader );
	u_sum_idx.isFloat = false;
	u_sum_g.isFloat = false;
	u_sum_d.isFloat = false;
	u_sum_embed.isFloat = false;
	u_sum_old.isFloat = false;
	u_sum_velocity.isFloat = false;
	if( !one_pass )
		u_sum_pass.isFloat = true;
	u_sum_binsperpt.isFloat = true;
	u_sum_embedwidth.isFloat = true;
	u_sum_distwidth.isFloat = true;
	u_sum_springforce.isFloat = true;
	u_sum_damping.isFloat = true;
	if( !one_pass )
		u_sum_finalpass.isFloat = true;
	u_sum_sizefactor.isFloat = true;
	u_sum_debug.isFloat = true;
	u_sum_weight.isFloat = true;
	u_sum_idx.location = glGetUniformLocationARB( sh_force_sum.program, "t_idx" );
	u_sum_g.location = glGetUniformLocationARB( sh_force_sum.program, "t_g" );
	u_sum_d.location = glGetUniformLocationARB( sh_force_sum.program, "t_d" );
	u_sum_embed.location = glGetUniformLocationARB( sh_force_sum.program, "t_embed" );
	u_sum_old.location = glGetUniformLocationARB( sh_force_sum.program, "t_old" );
	u_sum_velocity.location = glGetUniformLocationARB( sh_force_sum.program, "t_velocity" );
	if( !one_pass )
		u_sum_pass.location = glGetUniformLocationARB( sh_force_sum.program, "n_pass" );
	u_sum_binsperpt.location = glGetUniformLocationARB( sh_force_sum.program, "binsperpt" );
	u_sum_embedwidth.location = glGetUniformLocationARB( sh_force_sum.program, "embedwidth" );
	u_sum_distwidth.location = glGetUniformLocationARB( sh_force_sum.program, "distwidth" );
	u_sum_springforce.location = glGetUniformLocationARB( sh_force_sum.program, "springforce" );
	u_sum_damping.location = glGetUniformLocationARB( sh_force_sum.program, "damping" );
	if( !one_pass )
		u_sum_finalpass.location = glGetUniformLocationARB( sh_force_sum.program, "finalpass" );
	u_sum_sizefactor.location = glGetUniformLocationARB( sh_force_sum.program, "sizefactor" );
	u_sum_debug.location = glGetUniformLocationARB( sh_force_sum.program, "debug" );
	u_sum_weight.location = glGetUniformLocationARB( sh_force_sum.program, "weight" );
	u_sum_distwidth.valuef = (float)(((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 )));
	u_sum_binsperpt.valuef = (float) ceil( (double)n_embed_dims / 4.0 );
	u_sum_springforce.valuef = (float)SPRING_FORCE;
	u_sum_damping.valuef = (float)DAMPING_FACTOR;
	u_sum_sizefactor.valuef = (float)DATA_SIZE_FACTOR;
	u_sum_debug.valuef = 0.0f;
	u_sum_weight.valuef = g_b_useWeighting?g_f_minWeightDist:1000.0;

	sh_force_sum_graph.program = glCreateProgramObjectARB();
	sh_force_sum_graph.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_force_sum_graph.program, sh_force_sum_graph.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_force_sum_graph.glsl");
	const GLcharARB *force_sum_graph_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_force_sum_graph.shader, 1, &force_sum_graph_source, NULL);
	glCompileShaderARB(sh_force_sum_graph.shader);
	glLinkProgramARB(sh_force_sum_graph.program);
	printInfoLog( "sh_force_sum_graph", sh_force_sum_graph.shader );
	u_sum_graph_idx.isFloat = false;
	u_sum_graph_g.isFloat = false;
	u_sum_graph_d.isFloat = false;
	u_sum_graph_embed.isFloat = false;
	u_sum_graph_old.isFloat = false;
	u_sum_graph_velocity.isFloat = false;
//	u_sum_graph_pass.isFloat = true;
	u_sum_graph_binsperpt.isFloat = true;
	u_sum_graph_embedwidth.isFloat = true;
	u_sum_graph_distwidth.isFloat = true;
	u_sum_graph_springforce.isFloat = true;
	u_sum_graph_damping.isFloat = true;
//	u_sum_graph_finalpass.isFloat = true;
	u_sum_graph_sizefactor.isFloat = true;
	u_sum_graph_debug.isFloat = true;
	u_sum_graph_weight.isFloat = true;
	u_sum_graph_idx.location = glGetUniformLocationARB( sh_force_sum_graph.program, "t_idx" );
	u_sum_graph_g.location = glGetUniformLocationARB( sh_force_sum_graph.program, "t_g" );
	u_sum_graph_d.location = glGetUniformLocationARB( sh_force_sum_graph.program, "t_d" );
	u_sum_graph_embed.location = glGetUniformLocationARB( sh_force_sum_graph.program, "t_embed" );
	u_sum_graph_old.location = glGetUniformLocationARB( sh_force_sum_graph.program, "t_old" );
	u_sum_graph_velocity.location = glGetUniformLocationARB( sh_force_sum_graph.program, "t_velocity" );
//	u_sum_graph_pass.location = glGetUniformLocationARB( sh_force_sum_graph.program, "n_pass" );
	u_sum_graph_binsperpt.location = glGetUniformLocationARB( sh_force_sum_graph.program, "binsperpt" );
	u_sum_graph_embedwidth.location = glGetUniformLocationARB( sh_force_sum_graph.program, "embedwidth" );
	u_sum_graph_distwidth.location = glGetUniformLocationARB( sh_force_sum_graph.program, "distwidth" );
	u_sum_graph_springforce.location = glGetUniformLocationARB( sh_force_sum_graph.program, "springforce" );
	u_sum_graph_damping.location = glGetUniformLocationARB( sh_force_sum_graph.program, "damping" );
//	u_sum_graph_finalpass.location = glGetUniformLocationARB( sh_force_sum_graph.program, "finalpass" );
	u_sum_graph_sizefactor.location = glGetUniformLocationARB( sh_force_sum_graph.program, "sizefactor" );
	u_sum_graph_debug.location = glGetUniformLocationARB( sh_force_sum_graph.program, "debug" );
	u_sum_graph_weight.location = glGetUniformLocationARB( sh_force_sum_graph.program, "weight" );
	u_sum_graph_distwidth.valuef = (float)(((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 )));
	u_sum_graph_binsperpt.valuef = (float) ceil( (double)n_embed_dims / 4.0 );
	u_sum_graph_springforce.valuef = (float)SPRING_FORCE;
	u_sum_graph_damping.valuef = (float)DAMPING_FACTOR;
	u_sum_graph_sizefactor.valuef = (float)DATA_SIZE_FACTOR;
	u_sum_graph_debug.valuef = 0.0f;
	u_sum_graph_weight.valuef = g_b_useWeighting?g_f_minWeightDist:1000.0;

	//setup shader sh_force_cons
	sh_force_cons.program = glCreateProgramObjectARB();
	sh_force_cons.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_force_cons.program, sh_force_cons.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_force_cons.glsl");
	const GLcharARB *force_cons_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_force_cons.shader, 1, &force_cons_source, NULL);
	glCompileShaderARB(sh_force_cons.shader);
	glLinkProgramARB(sh_force_cons.program);
	printInfoLog( "sh_force_cons", sh_force_cons.shader );
	u_cons_idx.isFloat = false;
	u_cons_d.isFloat = false;
	u_cons_old.isFloat = false;
	u_cons_sum.isFloat = false;
	u_cons_pass.isFloat = true;
	u_cons_binsperpt.isFloat = true;
	u_cons_embedwidth.isFloat = true;
	u_cons_distwidth.isFloat = true;
	u_cons_idx.location = glGetUniformLocationARB( sh_force_cons.program, "t_idx" );
	u_cons_d.location = glGetUniformLocationARB( sh_force_cons.program, "t_d" );
	u_cons_old.location = glGetUniformLocationARB( sh_force_cons.program, "t_old" );
	u_cons_sum.location = glGetUniformLocationARB( sh_force_cons.program, "t_sum" );
	u_cons_pass.location = glGetUniformLocationARB( sh_force_cons.program, "n_pass" );
	u_cons_binsperpt.location = glGetUniformLocationARB( sh_force_cons.program, "binsperpt" );
	u_cons_embedwidth.location = glGetUniformLocationARB( sh_force_cons.program, "embedwidth" );
	u_cons_distwidth.location = glGetUniformLocationARB( sh_force_cons.program, "distwidth" );
	u_cons_distwidth.valuef = (float)(((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 )));
	u_cons_binsperpt.valuef = (float) ceil( (double)n_embed_dims / 4.0 );

	//setup shader sh_integrate
	sh_integrate.program = glCreateProgramObjectARB();
	sh_integrate.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_integrate.program, sh_integrate.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_integrate.glsl");
	const GLcharARB *integrate_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_integrate.shader, 1, &integrate_source, NULL);
	glCompileShaderARB(sh_integrate.shader);
	glLinkProgramARB(sh_integrate.program);
	printInfoLog( "sh_integrate", sh_integrate.shader );
	u_integrate_force.isFloat = false;
	u_integrate_old.isFloat = false;
	u_integrate_freeness.isFloat = true;
	u_integrate_deltatime.isFloat = true;
	u_integrate_force.location = glGetUniformLocationARB( sh_integrate.program, "t_force" );
	u_integrate_old.location = glGetUniformLocationARB( sh_integrate.program, "t_old" );
	u_integrate_freeness.location = glGetUniformLocationARB( sh_integrate.program, "freeness" );
	u_integrate_deltatime.location = glGetUniformLocationARB( sh_integrate.program, "deltatime" );
	u_integrate_freeness.valuef = (float)FREENESS;
	u_integrate_deltatime.valuef = (float)DELTA_TIME;

	//setup shader sh_move
	sh_move.program = glCreateProgramObjectARB();
	sh_move.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_move.program, sh_move.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_move.glsl");
	const GLcharARB *move_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_move.shader, 1, &move_source, NULL);
	glCompileShaderARB(sh_move.shader);
	glLinkProgramARB(sh_move.program);
	printInfoLog( "sh_move", sh_move.shader );
	u_move_velocity.isFloat = false;
	u_move_old.isFloat = false;
	u_move_deltatime.isFloat = true;
	u_move_velocity.location = glGetUniformLocationARB( sh_move.program, "t_velocity" );
	u_move_old.location = glGetUniformLocationARB( sh_move.program, "t_old" );
	u_move_deltatime.location = glGetUniformLocationARB( sh_move.program, "deltatime" );
	u_move_deltatime.valuef = (float)DELTA_TIME;

}

void sum_forces( Texture *texture, Tier *tier, bool b_fix ) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture->tier_width[tier->level],0.0,texture->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture->tier_width[tier->level],texture->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glUseProgramObjectARB( sh_force_sum.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_idx->texture_name[tier->t_idx->attach_idx]);
	glUniform1iARB(u_sum_idx.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_g->texture_name[tier->t_g->attach_idx]);
	glUniform1iARB(u_sum_g.location, 2); 
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);
	glUniform1iARB(u_sum_d.location, 3); 
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_embed->texture_name[tier->t_embed->attach_idx]);
	glUniform1iARB(u_sum_embed.location, 4); 
	glActiveTexture(GL_TEXTURE5);
	
	//glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_sum->texture_name[(tier->t_sum->attach_idx)?0:1]);
	//glUniform1iARB(u_sum_old.location, 5); 	
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_force->texture_name[(tier->t_force->attach_idx)?0:1]);
	glUniform1iARB(u_sum_old.location, 5); 

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_velocity->texture_name[tier->t_velocity->attach_idx]);
	glUniform1iARB(u_sum_velocity.location, 6); 		
	//glUniform1fARB(u_sum_embedwidth.location, (float)(tier->t_embed->tier_width[tier->level] / (int) u_sum_binsperpt.valuef) );
	glUniform1fARB(u_sum_embedwidth.location, (float)(tier->t_embed->tier_width[0] / (int) u_sum_binsperpt.valuef) );
	glUniform1fARB(u_sum_distwidth.location, u_sum_distwidth.valuef );
	glUniform1fARB(u_sum_binsperpt.location, u_sum_binsperpt.valuef );
	glUniform1fARB(u_sum_springforce.location, u_sum_springforce.valuef );
	if( !one_pass )
		glUniform1fARB(u_sum_finalpass.location, u_sum_finalpass.valuef );
	glUniform1fARB(u_sum_damping.location, u_sum_damping.valuef );
	glUniform1fARB(u_sum_sizefactor.location, u_sum_sizefactor.valuef );
	if( !one_pass )
		glUniform1fARB(u_sum_pass.location, (float)n_current_pass );
	glUniform1fARB(u_sum_debug.location, g_f_spacing);
	glUniform1fARB(u_sum_weight.location, g_b_useWeighting?g_f_minWeightDist:1000.0);

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	if( !b_fix ) {

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
	else {
		// and render quad
		glBegin(GL_QUADS);
			glTexCoord2f(texture->tier_width[tier->level+1], 0.0); 
			glVertex2f(texture->tier_width[tier->level+1], 0.0);
			glTexCoord2f(texture->tier_width[tier->level], 0.0); 
			glVertex2f(texture->tier_width[tier->level], 0.0);
			glTexCoord2f(texture->tier_width[tier->level], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level], texture->tier_height[tier->level]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]);

			glTexCoord2f(0.0, texture->tier_height[tier->level+1]); 
			glVertex2f(0.0, texture->tier_height[tier->level+1]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level+1]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level+1]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]);
			glTexCoord2f(0.0, texture->tier_height[tier->level]); 
			glVertex2f(0.0, texture->tier_height[tier->level]);
		glEnd();
	}
}

void sum_forces_graph( Texture *texture, Tier *tier, bool b_fix ) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture->tier_width[tier->level],0.0,texture->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture->tier_width[tier->level],texture->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glUseProgramObjectARB( sh_force_sum_graph.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_idx->texture_name[tier->t_idx->attach_idx]);
	glUniform1iARB(u_sum_graph_idx.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_g->texture_name[tier->t_g->attach_idx]);
	glUniform1iARB(u_sum_graph_g.location, 2); 
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);
	glUniform1iARB(u_sum_graph_d.location, 3); 
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_embed->texture_name[tier->t_embed->attach_idx]);
	glUniform1iARB(u_sum_graph_embed.location, 4); 
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_force->texture_name[(tier->t_force->attach_idx)?0:1]);
	glUniform1iARB(u_sum_graph_old.location, 5); 
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_velocity->texture_name[tier->t_velocity->attach_idx]);
	glUniform1iARB(u_sum_graph_velocity.location, 6); 
	//glUniform1fARB(u_sum_graph_embedwidth.location, (float)(tier->t_embed->tier_width[tier->level] / (int) u_sum_graph_binsperpt.valuef) );
	glUniform1fARB(u_sum_graph_embedwidth.location, (float)(tier->t_embed->tier_width[0] / (int) u_sum_graph_binsperpt.valuef) );
	glUniform1fARB(u_sum_graph_distwidth.location, u_sum_graph_distwidth.valuef );
	glUniform1fARB(u_sum_graph_binsperpt.location, u_sum_graph_binsperpt.valuef );
	glUniform1fARB(u_sum_graph_springforce.location, u_sum_graph_springforce.valuef );
//	glUniform1fARB(u_sum_graph_finalpass.location, u_sum_graph_finalpass.valuef );
	glUniform1fARB(u_sum_graph_damping.location, u_sum_graph_damping.valuef );
	glUniform1fARB(u_sum_graph_sizefactor.location, u_sum_graph_sizefactor.valuef );
//	glUniform1fARB(u_sum_graph_pass.location, (float)n_current_pass );
	glUniform1fARB(u_sum_graph_debug.location, g_f_spacing);
	glUniform1fARB(u_sum_graph_weight.location, g_b_useWeighting?g_f_minWeightDist:1000.0);

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	if( !b_fix ) {

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
	else {
		// and render quad
		glBegin(GL_QUADS);
			glTexCoord2f(texture->tier_width[tier->level+1], 0.0); 
			glVertex2f(texture->tier_width[tier->level+1], 0.0);
			glTexCoord2f(texture->tier_width[tier->level], 0.0); 
			glVertex2f(texture->tier_width[tier->level], 0.0);
			glTexCoord2f(texture->tier_width[tier->level], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level], texture->tier_height[tier->level]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]);

			glTexCoord2f(0.0, texture->tier_height[tier->level+1]); 
			glVertex2f(0.0, texture->tier_height[tier->level+1]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level+1]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level+1]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]);
			glTexCoord2f(0.0, texture->tier_height[tier->level]); 
			glVertex2f(0.0, texture->tier_height[tier->level]);
		glEnd();
	}
}

void apply_forces( Texture *texture, Tier *tier, bool b_fix ) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture->tier_width[tier->level],0.0,texture->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture->tier_width[tier->level],texture->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glUseProgramObjectARB( sh_force_cons.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_idx->texture_name[tier->t_idx->attach_idx]);
	glUniform1iARB(u_cons_idx.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);
	glUniform1iARB(u_cons_d.location, 2); 
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_force->texture_name[(tier->t_force->attach_idx)?0:1]);
	glUniform1iARB(u_cons_old.location, 3); 
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_force->texture_name[tier->t_force->attach_idx]);
	glUniform1iARB(u_cons_sum.location, 4); 
	//glUniform1fARB(u_cons_embedwidth.location, (float)(tier->t_embed->tier_width[tier->level] / (int) u_cons_binsperpt.valuef));
	glUniform1fARB(u_cons_embedwidth.location, (float)(tier->t_embed->tier_width[0] / (int) u_cons_binsperpt.valuef));
	glUniform1fARB(u_cons_distwidth.location, u_cons_distwidth.valuef );
	glUniform1fARB(u_cons_binsperpt.location, u_cons_binsperpt.valuef );
	glUniform1fARB(u_cons_pass.location, (float)n_current_pass );

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	if( !b_fix ) {

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
	else {
		// and render quad
		glBegin(GL_QUADS);
			glTexCoord2f(texture->tier_width[tier->level+1], 0.0); 
			glVertex2f(texture->tier_width[tier->level+1], 0.0);
			glTexCoord2f(texture->tier_width[tier->level], 0.0); 
			glVertex2f(texture->tier_width[tier->level], 0.0);
			glTexCoord2f(texture->tier_width[tier->level], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level], texture->tier_height[tier->level]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]);

			glTexCoord2f(0.0, texture->tier_height[tier->level+1]); 
			glVertex2f(0.0, texture->tier_height[tier->level+1]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level+1]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level+1]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]);
			glTexCoord2f(0.0, texture->tier_height[tier->level]); 
			glVertex2f(0.0, texture->tier_height[tier->level]);
		glEnd();
	}

}

void integrate_forces( Texture *texture, Tier *tier, bool b_fix ) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture->tier_width[tier->level],0.0,texture->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture->tier_width[tier->level],texture->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glUseProgramObjectARB( sh_integrate.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_velocity->texture_name[tier->t_velocity->attach_idx?0:1]);
	glUniform1iARB(u_integrate_old.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_force->texture_name[tier->t_force->attach_idx]);
	glUniform1iARB(u_integrate_force.location, 2); 
	glUniform1fARB(u_integrate_freeness.location, u_integrate_freeness.valuef );
	glUniform1fARB(u_integrate_deltatime.location, u_integrate_deltatime.valuef );

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	if( !b_fix ) {

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
	else {
		// and render quad
		glBegin(GL_QUADS);
			glTexCoord2f(texture->tier_width[tier->level+1], 0.0); 
			glVertex2f(texture->tier_width[tier->level+1], 0.0);
			glTexCoord2f(texture->tier_width[tier->level], 0.0); 
			glVertex2f(texture->tier_width[tier->level], 0.0);
			glTexCoord2f(texture->tier_width[tier->level], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level], texture->tier_height[tier->level]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]);

			glTexCoord2f(0.0, texture->tier_height[tier->level+1]); 
			glVertex2f(0.0, texture->tier_height[tier->level+1]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level+1]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level+1]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]);
			glTexCoord2f(0.0, texture->tier_height[tier->level]); 
			glVertex2f(0.0, texture->tier_height[tier->level]);
		glEnd();
	}

}

void apply_velocity( Texture *texture, Tier *tier, bool b_fix ) {

	if( embed_bounds == NULL ) {
		embed_bounds = ( float * ) malloc( sizeof(float) * 16 );
		// first square
		embed_bounds[0] = 0.0f;
		embed_bounds[1] = 0.0f;
		embed_bounds[2] = (float) texture->width;
		embed_bounds[3] = 0.0f;
		embed_bounds[4] = (float) texture->width;
		embed_bounds[5] = (float) (tier->n_points / texture->width);
		embed_bounds[6] = 0.0f;
		embed_bounds[7] = (float) (tier->n_points / texture->width);
		if(tier->n_points % texture->width) {
			// 2nd square
			embed_bounds[8] = 0.0f;
			embed_bounds[9] = (float) (tier->n_points / texture->width);
			embed_bounds[10] = (float) (tier->n_points % texture->width);
			embed_bounds[11] = (float) (tier->n_points / texture->width);
			embed_bounds[12] = (float) (tier->n_points % texture->width);
			embed_bounds[13] = (float) (tier->n_points / texture->width) + 1.0f;
			embed_bounds[14] = 0.0f;
			embed_bounds[15] = (float) (tier->n_points / texture->width) + 1.0f;
		}
	}

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture->tier_width[tier->level],0.0,texture->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture->tier_width[tier->level],texture->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glUseProgramObjectARB( sh_move.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_velocity->texture_name[tier->t_velocity->attach_idx]);
	glUniform1iARB(u_move_velocity.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_embed->texture_name[(tier->t_embed->attach_idx)?0:1]);
	glUniform1iARB(u_move_old.location, 2); 
	glUniform1fARB(u_move_deltatime.location, u_move_deltatime.valuef );

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	if( !b_fix ) {

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
	else {
		// and render quad
		glBegin(GL_QUADS);
			glTexCoord2f(texture->tier_width[tier->level+1], 0.0); 
			glVertex2f(texture->tier_width[tier->level+1], 0.0);
			glTexCoord2f(texture->tier_width[tier->level], 0.0); 
			glVertex2f(texture->tier_width[tier->level], 0.0);
			glTexCoord2f(texture->tier_width[tier->level], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level], texture->tier_height[tier->level]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]);

			glTexCoord2f(0.0, texture->tier_height[tier->level+1]); 
			glVertex2f(0.0, texture->tier_height[tier->level+1]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level+1]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level+1]);
			glTexCoord2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]); 
			glVertex2f(texture->tier_width[tier->level+1], texture->tier_height[tier->level]);
			glTexCoord2f(0.0, texture->tier_height[tier->level]); 
			glVertex2f(0.0, texture->tier_height[tier->level]);
		glEnd();
	}

}

