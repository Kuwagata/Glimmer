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
#include "params.h"
#include "data.h"

extern int n_max_sparse_entries;
extern bool b_sparse_vector_input;
extern bool b_output_debug;
extern bool g_b_useVel;
extern float *displacement_window;
extern float *displacement_bounds;
extern float *zero_data;
extern int n_dims;
extern int n_embed_dims;
extern char g_shader_path[4092];
extern GLenum attachmentpoints[];
extern char g_shader_path[4092];

Shader sh_stress_d_diff;			// computes the first phase of the stress function
Shader sh_stress_d_dot;				// computes the first phase of the stress function (if using cos distances)
Shader sh_stress_d_reduce;			// computes the second phase of the stress function
Shader sh_stress_g;					// computes the third phase of the stress function
Shader sh_stress_g_dot;				// computes the third phase of the stress function (if using cos distances)
Shader sh_stress_g_denom;			// computes the third phase of the stress function
Shader sh_stress_reduce;			// computes the final phases of the stress function
Shader sh_stress_reduce_dual;		// computes the final phases of the stress function
Shader sh_stress_bounds;			// computes the min/max of the embedding dimensions
Shader sh_sp_stress;				// computes the sparse stress using only existing distances
Shader sh_sp_stress_denom_low;		// 
Shader sh_sp_stress_denom_high;		// 

//Shader sh_stress_debug;				

Uniform u_sp_stress_d;
Uniform u_sp_stress_g;

Uniform u_sp_stress_denom_low_d;
Uniform u_sp_stress_denom_low_g;
Uniform u_sp_stress_denom_high_d;

Uniform u_stress_d_pts;
Uniform u_stress_diff_reference;
Uniform u_stress_diff_pointwidth;
Uniform u_stress_d_inputptx;
Uniform u_stress_d_inputpty;

Uniform u_stress_d_dot_pts;
Uniform u_stress_d_dot_vec_idx;
Uniform u_stress_d_dot_reference;
Uniform u_stress_d_dot_pointwidth;
Uniform u_stress_d_dot_inputptx;
Uniform u_stress_d_dot_inputpty;
Uniform u_stress_d_dot_maxnz;
Uniform u_stress_d_dot_logmaxnz;
Uniform u_stress_d_dot_halfN;

Uniform u_stress_d_old;
Uniform u_stress_d_pointwidth;
Uniform u_stress_d_chunkwidth;

Uniform u_stress_g_old;
Uniform u_stress_g_reference;
Uniform u_stress_g_embed;
Uniform u_stress_g_pointwidth;
Uniform u_stress_g_inputptx;
Uniform u_stress_g_inputpty;

Uniform u_stress_g_dot_old;
Uniform u_stress_g_dot_reference;
Uniform u_stress_g_dot_embed;
Uniform u_stress_g_dot_pointwidth;
Uniform u_stress_g_dot_inputptx;
Uniform u_stress_g_dot_inputpty;

Uniform u_stress_g_denom_embed;
Uniform u_stress_g_denom_pointwidth;
Uniform u_stress_g_denom_inputptx;
Uniform u_stress_g_denom_inputpty;
Uniform u_stress_g_denom_reference;

Uniform u_stress_reduce_dual_old;
Uniform u_stress_reduce_dual_texwidth;
Uniform u_stress_reduce_dual_texheight;
Uniform u_stress_reduce_dual_offset;

Uniform u_stress_reduce_old;
Uniform u_stress_reduce_texwidth;
Uniform u_stress_reduce_texheight;
Uniform u_stress_reduce_offset;

Uniform u_stress_bounds_input;
Uniform u_stress_bounds_texwidth;
Uniform u_stress_bounds_texheight;
Uniform u_stress_bounds_mode;
Uniform u_stress_bounds_skip;

void setup_sh_stress( ) {

	char shader_path[8192];

	// setup sh_sp_stress_denom_high
	sh_sp_stress_denom_high.program = glCreateProgramObjectARB();
	sh_sp_stress_denom_high.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_sp_stress_denom_high.program, sh_sp_stress_denom_high.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_sp_stress_denom_high.glsl");
	const GLcharARB *sp_stress_denom_high_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_sp_stress_denom_high.shader, 1, &sp_stress_denom_high_source, NULL);
	glCompileShaderARB(sh_sp_stress_denom_high.shader);
	glLinkProgramARB(sh_sp_stress_denom_high.program);
	printInfoLog( "sh_sp_stress_denom_high", sh_sp_stress_denom_high.shader );
	u_sp_stress_denom_high_d.location = glGetUniformLocationARB( sh_sp_stress_denom_high.program, "t_d" );

	// setup sh_sp_stress_denom_low
	sh_sp_stress_denom_low.program = glCreateProgramObjectARB();
	sh_sp_stress_denom_low.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_sp_stress_denom_low.program, sh_sp_stress_denom_low.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_sp_stress_denom_low.glsl");
	const GLcharARB *sp_stress_denom_low_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_sp_stress_denom_low.shader, 1, &sp_stress_denom_low_source, NULL);
	glCompileShaderARB(sh_sp_stress_denom_low.shader);
	glLinkProgramARB(sh_sp_stress_denom_low.program);
	printInfoLog( "sh_sp_stress_denom_low", sh_sp_stress_denom_low.shader );
	u_sp_stress_denom_low_d.location = glGetUniformLocationARB( sh_sp_stress_denom_low.program, "t_d" );
	u_sp_stress_denom_low_g.location = glGetUniformLocationARB( sh_sp_stress_denom_low.program, "t_g" );

	// setup sh_sp_stress
	sh_sp_stress.program = glCreateProgramObjectARB();
	sh_sp_stress.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_sp_stress.program, sh_sp_stress.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_sp_stress.glsl");
	const GLcharARB *sp_stress_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_sp_stress.shader, 1, &sp_stress_source, NULL);
	glCompileShaderARB(sh_sp_stress.shader);
	glLinkProgramARB(sh_sp_stress.program);
	printInfoLog( "sh_sp_stress", sh_sp_stress.shader );
	u_sp_stress_d.location = glGetUniformLocationARB( sh_sp_stress.program, "t_d" );
	u_sp_stress_g.location = glGetUniformLocationARB( sh_sp_stress.program, "t_g" );

	// setup sh_stress_bounds
	sh_stress_bounds.program = glCreateProgramObjectARB();
	sh_stress_bounds.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_stress_bounds.program, sh_stress_bounds.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_stress_bounds.glsl");
	const GLcharARB *stress_bounds_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_stress_bounds.shader, 1, &stress_bounds_source, NULL);
	glCompileShaderARB(sh_stress_bounds.shader);
	glLinkProgramARB(sh_stress_bounds.program);
	printInfoLog( "sh_stress_bounds", sh_stress_bounds.shader );
	u_stress_bounds_input.location = glGetUniformLocationARB( sh_stress_bounds.program, "input" );
	u_stress_bounds_texwidth.location = glGetUniformLocationARB( sh_stress_bounds.program, "texwidth" );
	u_stress_bounds_texheight.location = glGetUniformLocationARB( sh_stress_bounds.program, "texheight" );
	u_stress_bounds_mode.location = glGetUniformLocationARB( sh_stress_bounds.program, "mode" );
	u_stress_bounds_skip.location = glGetUniformLocationARB( sh_stress_bounds.program, "skip" );

	// setup sh_stress_d_diff
	sh_stress_d_diff.program = glCreateProgramObjectARB();
	sh_stress_d_diff.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_stress_d_diff.program, sh_stress_d_diff.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_stress_d_diff.glsl");
	const GLcharARB *stress_d_diff_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_stress_d_diff.shader, 1, &stress_d_diff_source, NULL);
	glCompileShaderARB(sh_stress_d_diff.shader);
	glLinkProgramARB(sh_stress_d_diff.program);
	printInfoLog( "sh_stress_d_diff", sh_stress_d_diff.shader );
	u_stress_d_pts.isFloat = false;
	u_stress_diff_reference.isFloat = false;
	u_stress_d_inputptx.isFloat = true;	
	u_stress_d_inputpty.isFloat = true;	
	u_stress_d_pts.location = glGetUniformLocationARB( sh_stress_d_diff.program, "pts" );
	u_stress_diff_reference.location = glGetUniformLocationARB( sh_stress_d_diff.program, "reference" );
	u_stress_diff_pointwidth.location = glGetUniformLocationARB( sh_stress_d_diff.program, "pointwidth" );
	u_stress_d_inputptx.location = glGetUniformLocationARB( sh_stress_d_diff.program, "inputptx" );
	u_stress_d_inputpty.location = glGetUniformLocationARB( sh_stress_d_diff.program, "inputpty" );

	// setup sh_stress_d_dot
	sh_stress_d_dot.program = glCreateProgramObjectARB();
	sh_stress_d_dot.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_stress_d_dot.program, sh_stress_d_dot.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_stress_d_dot.glsl");
	const GLcharARB *stress_d_dot_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_stress_d_dot.shader, 1, &stress_d_dot_source, NULL);
	glCompileShaderARB(sh_stress_d_dot.shader);
	glLinkProgramARB(sh_stress_d_dot.program);
	printInfoLog( "sh_stress_d_dot", sh_stress_d_dot.shader );
	u_stress_d_dot_pts.isFloat = false;
	u_stress_d_dot_vec_idx.isFloat = false;
	u_stress_d_dot_reference.isFloat = false;
	u_stress_d_dot_inputptx.isFloat = true;	
	u_stress_d_dot_inputpty.isFloat = true;	
	u_stress_d_dot_maxnz.isFloat = true;
	u_stress_d_dot_logmaxnz.isFloat = true;
	u_stress_d_dot_halfN.isFloat = true;
	u_stress_d_dot_pts.location = glGetUniformLocationARB( sh_stress_d_dot.program, "pts" );
	u_stress_d_dot_vec_idx.location = glGetUniformLocationARB( sh_stress_d_dot.program, "vec_idx" );
	u_stress_d_dot_reference.location = glGetUniformLocationARB( sh_stress_d_dot.program, "reference" );
	u_stress_d_dot_pointwidth.location = glGetUniformLocationARB( sh_stress_d_dot.program, "pointwidth" );
	u_stress_d_dot_inputptx.location = glGetUniformLocationARB( sh_stress_d_dot.program, "inputptx" );
	u_stress_d_dot_inputpty.location = glGetUniformLocationARB( sh_stress_d_dot.program, "inputpty" );
	u_stress_d_dot_maxnz.location = glGetUniformLocationARB( sh_stress_d_dot.program, "maxnz" );
	u_stress_d_dot_logmaxnz.location = glGetUniformLocationARB( sh_stress_d_dot.program, "logmaxnz" );
	u_stress_d_dot_halfN.location = glGetUniformLocationARB( sh_stress_d_dot.program, "halfN" );

	// setup sh_stress_d_reduce
	sh_stress_d_reduce.program = glCreateProgramObjectARB();
	sh_stress_d_reduce.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_stress_d_reduce.program, sh_stress_d_reduce.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_stress_d_reduce.glsl");
	const GLcharARB *stress_d_reduce_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_stress_d_reduce.shader, 1, &stress_d_reduce_source, NULL);
	glCompileShaderARB(sh_stress_d_reduce.shader);
	glLinkProgramARB(sh_stress_d_reduce.program);
	printInfoLog( "sh_stress_d_reduce", sh_stress_d_reduce.shader );
	u_stress_d_old.isFloat = false;
	u_stress_d_pointwidth.isFloat = true;	
	u_stress_d_chunkwidth.isFloat = true;	
	u_stress_d_old.location = glGetUniformLocationARB( sh_stress_d_reduce.program, "old" );
	u_stress_d_pointwidth.location = glGetUniformLocationARB( sh_stress_d_reduce.program, "pointwidth" );
	u_stress_d_chunkwidth.location = glGetUniformLocationARB( sh_stress_d_reduce.program, "chunkwidth" );

	// setup sh_stress_g
	sh_stress_g.program = glCreateProgramObjectARB();
	sh_stress_g.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_stress_g.program, sh_stress_g.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_stress_g.glsl");
	const GLcharARB *stress_g_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_stress_g.shader, 1, &stress_g_source, NULL);
	glCompileShaderARB(sh_stress_g.shader);
	glLinkProgramARB(sh_stress_g.program);
	printInfoLog( "sh_stress_g", sh_stress_g.shader );
	u_stress_g_old.isFloat = false;
	u_stress_g_embed.isFloat = false;
	u_stress_g_reference.isFloat = false;
	u_stress_g_pointwidth.isFloat = true;	
	u_stress_g_inputptx.isFloat = true;	
	u_stress_g_inputpty.isFloat = true;	
	u_stress_g_old.location = glGetUniformLocationARB( sh_stress_g.program, "old" );
	u_stress_g_embed.location = glGetUniformLocationARB( sh_stress_g.program, "embed" );
	u_stress_g_reference.location = glGetUniformLocationARB( sh_stress_g.program, "reference" );
	u_stress_g_pointwidth.location = glGetUniformLocationARB( sh_stress_g.program, "pointwidth" );
	u_stress_g_inputptx.location = glGetUniformLocationARB( sh_stress_g.program, "inputptx" );
	u_stress_g_inputpty.location = glGetUniformLocationARB( sh_stress_g.program, "inputpty" );

	// setup sh_stress_g_dot
	sh_stress_g_dot.program = glCreateProgramObjectARB();
	sh_stress_g_dot.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_stress_g_dot.program, sh_stress_g_dot.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_stress_g_dot.glsl");
	const GLcharARB *stress_g_dot_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_stress_g_dot.shader, 1, &stress_g_dot_source, NULL);
	glCompileShaderARB(sh_stress_g_dot.shader);
	glLinkProgramARB(sh_stress_g_dot.program);
	printInfoLog( "sh_stress_g_dot", sh_stress_g_dot.shader );
	u_stress_g_dot_old.isFloat = false;
	u_stress_g_dot_embed.isFloat = false;
	u_stress_g_dot_reference.isFloat = false;
	u_stress_g_dot_pointwidth.isFloat = true;	
	u_stress_g_dot_inputptx.isFloat = true;	
	u_stress_g_dot_inputpty.isFloat = true;	
	u_stress_g_dot_old.location = glGetUniformLocationARB( sh_stress_g_dot.program, "old" );
	u_stress_g_dot_embed.location = glGetUniformLocationARB( sh_stress_g_dot.program, "embed" );
	u_stress_g_dot_reference.location = glGetUniformLocationARB( sh_stress_g_dot.program, "reference" );
	u_stress_g_dot_pointwidth.location = glGetUniformLocationARB( sh_stress_g_dot.program, "pointwidth" );
	u_stress_g_dot_inputptx.location = glGetUniformLocationARB( sh_stress_g_dot.program, "inputptx" );
	u_stress_g_dot_inputpty.location = glGetUniformLocationARB( sh_stress_g_dot.program, "inputpty" );

	// setup sh_stress_g_denom
	sh_stress_g_denom.program = glCreateProgramObjectARB();
	sh_stress_g_denom.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_stress_g_denom.program, sh_stress_g_denom.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_stress_g_denom.glsl");
	const GLcharARB *stress_g_denom_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_stress_g_denom.shader, 1, &stress_g_denom_source, NULL);
	glCompileShaderARB(sh_stress_g_denom.shader);
	glLinkProgramARB(sh_stress_g_denom.program);
	printInfoLog( "sh_stress_g_denom", sh_stress_g_denom.shader );
	u_stress_g_denom_embed.isFloat = false;
	u_stress_g_denom_reference.isFloat = false;
	u_stress_g_denom_pointwidth.isFloat = true;	
	u_stress_g_denom_inputptx.isFloat = true;	
	u_stress_g_denom_inputpty.isFloat = true;	
	u_stress_g_denom_embed.location = glGetUniformLocationARB( sh_stress_g_denom.program, "embed" );
	u_stress_g_denom_reference.location = glGetUniformLocationARB( sh_stress_g_denom.program, "reference" );
	u_stress_g_denom_pointwidth.location = glGetUniformLocationARB( sh_stress_g_denom.program, "pointwidth" );
	u_stress_g_denom_inputptx.location = glGetUniformLocationARB( sh_stress_g_denom.program, "inputptx" );
	u_stress_g_denom_inputpty.location = glGetUniformLocationARB( sh_stress_g_denom.program, "inputpty" );

	// setup sh_stress_reduce
	sh_stress_reduce.program = glCreateProgramObjectARB();
	sh_stress_reduce.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_stress_reduce.program, sh_stress_reduce.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_stress_reduce.glsl");
	const GLcharARB *stress_reduce_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_stress_reduce.shader, 1, &stress_reduce_source, NULL);
	glCompileShaderARB(sh_stress_reduce.shader);
	glLinkProgramARB(sh_stress_reduce.program);
	printInfoLog( "sh_stress_reduce", sh_stress_reduce.shader );
	u_stress_reduce_old.isFloat = false;
	u_stress_reduce_texwidth.isFloat = true;	
	u_stress_reduce_texheight.isFloat = true;	
	u_stress_reduce_offset.isFloat = true;	
	u_stress_reduce_old.location = glGetUniformLocationARB( sh_stress_reduce.program, "old" );
	u_stress_reduce_texwidth.location = glGetUniformLocationARB( sh_stress_reduce.program, "texwidth" );
	u_stress_reduce_texheight.location = glGetUniformLocationARB( sh_stress_reduce.program, "texheight" );
	u_stress_reduce_offset.location = glGetUniformLocationARB( sh_stress_reduce.program, "offset" );

	// setup sh_stress_reduce_dual
	sh_stress_reduce_dual.program = glCreateProgramObjectARB();
	sh_stress_reduce_dual.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_stress_reduce_dual.program, sh_stress_reduce_dual.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_stress_reduce_dual.glsl");
	const GLcharARB *stress_reduce_dual_source = readShaderSource(shader_path);
	glShaderSourceARB(sh_stress_reduce_dual.shader, 1, &stress_reduce_dual_source, NULL);
	glCompileShaderARB(sh_stress_reduce_dual.shader);
	glLinkProgramARB(sh_stress_reduce_dual.program);
	printInfoLog( "sh_stress_reduce_dual", sh_stress_reduce_dual.shader );
	u_stress_reduce_dual_old.isFloat = false;
	u_stress_reduce_dual_texwidth.isFloat = true;	
	u_stress_reduce_dual_texheight.isFloat = true;	
	u_stress_reduce_dual_offset.isFloat = true;	
	u_stress_reduce_dual_old.location = glGetUniformLocationARB( sh_stress_reduce_dual.program, "old" );
	u_stress_reduce_dual_texwidth.location = glGetUniformLocationARB( sh_stress_reduce_dual.program, "texwidth" );
	u_stress_reduce_dual_texheight.location = glGetUniformLocationARB( sh_stress_reduce_dual.program, "texheight" );
	u_stress_reduce_dual_offset.location = glGetUniformLocationARB( sh_stress_reduce_dual.program, "offset" );
}

/*

	Calculate the min/max x and y of the input points

	Returns a float array of size four containing - 

	{minx, miny, maxx, maxy}

	don't forget to free the memory when you're done :)

*/
int mymax( int a, int b ) {
return (a>b)?a:b;
}

float *calc_bounds( Tier *tier ) {

	// calculate / log_2( max( width, height ) ) \

	float bottom1 = floor(((double)tier->n_points)/((double)tier->t_embed->width));
	float bottom2 = ceil(((double)tier->n_points)/((double)tier->t_embed->width));
	float width1 = (float)(tier->n_points % tier->t_embed->width);

	//printf("%f %f %f\n", bottom1, bottom2, width1 ); 

	float temp = ceil( log( (double) mymax(tier->t_embed->tier_width[tier->level], 
		tier->t_embed->tier_height[tier->level]) ) / log( 2.0 ) );

	float min_dims[4];
	float max_dims[4];
	float *return_dims = (float*)malloc(sizeof(float)*4);

	// get the min values

	for( int j = 0; j < (int) temp; j++ ) {

		tier->t_stress->attach_idx = tier->t_stress->attach_idx?0:1;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0,tier->t_embed->tier_width[tier->level],0.0,tier->t_embed->tier_height[tier->level]);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glViewport(0,0,tier->t_embed->tier_width[tier->level],tier->t_embed->tier_height[tier->level]);

		// bind data structures and active texture units
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
		glUseProgramObjectARB( sh_stress_bounds.program );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

		if( j == 0 ) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_embed->texture_name[tier->t_embed->attach_idx]);
			glUniform1iARB(u_stress_bounds_input.location, 1); 
		}
		else {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx?0:1]);
			glUniform1iARB(u_stress_bounds_input.location, 1); 
		}

		glUniform1fARB(u_stress_bounds_mode.location, 1.f );
		glUniform1fARB(u_stress_bounds_skip.location, pow((float)2,(int)j) );
		glUniform1fARB(u_stress_bounds_texwidth.location, tier->t_embed->tier_width[tier->level] );
		glUniform1fARB(u_stress_bounds_texheight.location, tier->t_embed->tier_height[tier->level] );

		// make quad filled to hit every pixel/texel
		glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
		glPolygonMode(GL_FRONT,GL_FILL);

		// and render quad
		glBegin(GL_QUADS);
			
		if( tier->level == 0 ) {
			glTexCoord2f(0.0, 0.0); 
			glVertex2f(0.0, 0.0);
			glTexCoord2f(tier->t_embed->width, 0.0); 
			glVertex2f(tier->t_embed->width, 0.0);
			glTexCoord2f(tier->t_embed->width, bottom1); 
			glVertex2f(tier->t_embed->width, bottom1);
			glTexCoord2f(0.0, bottom1); 
			glVertex2f(0.0, bottom1);

			if( bottom1 < bottom2 ) {
				glTexCoord2f(0.0, bottom1); 
				glVertex2f(0.0, bottom1);
				glTexCoord2f(width1, bottom1); 
				glVertex2f(width1, bottom1);
				glTexCoord2f(width1, bottom2); 
				glVertex2f(width1, bottom2);
				glTexCoord2f(0.0, bottom2); 
				glVertex2f(0.0, bottom2);
			}
		}
		else {
			glTexCoord2f(0.0, 0.0); 
			glVertex2f(0.0, 0.0);
			glTexCoord2f(tier->t_embed->tier_width[tier->level], 0.0); 
			glVertex2f(tier->t_embed->tier_width[tier->level], 0.0);
			glTexCoord2f(tier->t_embed->tier_width[tier->level], tier->t_embed->tier_height[tier->level]); 
			glVertex2f(tier->t_embed->tier_width[tier->level], tier->t_embed->tier_height[tier->level]);
			glTexCoord2f(0.0, tier->t_embed->tier_height[tier->level]); 
			glVertex2f(0.0, tier->t_embed->tier_height[tier->level]);
		}
		glEnd();

	}

	// read the number and add it to our total
	glReadBuffer(tier->t_stress->texture_name[tier->t_stress->attach_idx]);
	glReadPixels(0,0,1,1,GL_RGBA,GL_FLOAT,min_dims);
	
	// get the max values

	for( int j = 0; j < (int) temp; j++ ) {

		tier->t_stress->attach_idx = tier->t_stress->attach_idx?0:1;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0,tier->t_embed->tier_width[tier->level],0.0,tier->t_embed->tier_height[tier->level]);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glViewport(0,0,tier->t_embed->tier_width[tier->level],tier->t_embed->tier_height[tier->level]);

		// bind data structures and active texture units
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
		glUseProgramObjectARB( sh_stress_bounds.program );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

		if( j == 0 ) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_embed->texture_name[tier->t_embed->attach_idx]);
			glUniform1iARB(u_stress_bounds_input.location, 1); 
		}
		else {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx?0:1]);
			glUniform1iARB(u_stress_bounds_input.location, 1); 
		}

		glUniform1fARB(u_stress_bounds_mode.location, 0.f );
		glUniform1fARB(u_stress_bounds_skip.location, pow((float)2,j) );
		glUniform1fARB(u_stress_bounds_texwidth.location, tier->t_embed->tier_width[tier->level] );
		glUniform1fARB(u_stress_bounds_texheight.location, tier->t_embed->tier_height[tier->level] );

		// make quad filled to hit every pixel/texel
		glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
		glPolygonMode(GL_FRONT,GL_FILL);

		// and render quad
		glBegin(GL_QUADS);
			
		if( tier->level == 0 ) {
			glTexCoord2f(0.0, 0.0); 
			glVertex2f(0.0, 0.0);
			glTexCoord2f(tier->t_embed->width, 0.0); 
			glVertex2f(tier->t_embed->width, 0.0);
			glTexCoord2f(tier->t_embed->width, bottom1); 
			glVertex2f(tier->t_embed->width, bottom1);
			glTexCoord2f(0.0, bottom1); 
			glVertex2f(0.0, bottom1);

			if( bottom1 < bottom2 ) {
				glTexCoord2f(0.0, bottom1); 
				glVertex2f(0.0, bottom1);
				glTexCoord2f(width1, bottom1); 
				glVertex2f(width1, bottom1);
				glTexCoord2f(width1, bottom2); 
				glVertex2f(width1, bottom2);
				glTexCoord2f(0.0, bottom2); 
				glVertex2f(0.0, bottom2);
			}
		}
		else {
			glTexCoord2f(0.0, 0.0); 
			glVertex2f(0.0, 0.0);
			glTexCoord2f(tier->t_embed->tier_width[tier->level], 0.0); 
			glVertex2f(tier->t_embed->tier_width[tier->level], 0.0);
			glTexCoord2f(tier->t_embed->tier_width[tier->level], tier->t_embed->tier_height[tier->level]); 
			glVertex2f(tier->t_embed->tier_width[tier->level], tier->t_embed->tier_height[tier->level]);
			glTexCoord2f(0.0, tier->t_embed->tier_height[tier->level]); 
			glVertex2f(0.0, tier->t_embed->tier_height[tier->level]);
		}
		glEnd();

	}

	// read the number and add it to our total
	glReadBuffer(tier->t_stress->texture_name[tier->t_stress->attach_idx]);
	glReadPixels(0,0,1,1,GL_RGBA,GL_FLOAT,max_dims);

	max_dims[0]-=min_dims[0];
	max_dims[1]-=min_dims[1];

	return_dims[0]=min_dims[0];
	return_dims[1]=min_dims[1];
	return_dims[2]=max_dims[0];
	return_dims[3]=max_dims[1];

	return return_dims;
}

/*

	Calculate embedding coordinate displacement on the CPU (slow and to be replaced)

*/

float calc_displacement( Tier *tier ) {

	//return 0.;

	static int disp_ptr = 0;
	static bool b_calc_ready = false;
	double displacement = 0.;
	int normfactor = 0;

	float *bounds = calc_bounds( tier );
	displacement_bounds[ 4*disp_ptr + 0 ] = bounds[ 0 ];
	displacement_bounds[ 4*disp_ptr + 1 ] = bounds[ 1 ];
	displacement_bounds[ 4*disp_ptr + 2 ] = bounds[ 2 ];
	displacement_bounds[ 4*disp_ptr + 3 ] = bounds[ 3 ];
	free( bounds );

	//printf("{%f,%f,%f,%f}\n", bounds[0], bounds[1], bounds[2], bounds[3] );

	// grab the current coordinates

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_embed->frame_buffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_embed->texture_name[tier->t_embed->attach_idx]);
	glReadBuffer(attachmentpoints[tier->t_embed->attach_idx]);
	glReadPixels(0, 0, tier->t_embed->width, tier->t_embed->height,GL_RGBA,GL_FLOAT,displacement_window + (tier->t_embed->width*tier->t_embed->height*4*disp_ptr));

	// do we have enough windows to calculate displacment (check only needs to occur once)
	if( !((1+disp_ptr) % DISPLACEMENT_WINDOW_SIZE) )
		b_calc_ready = true;

	if( b_calc_ready ) {

		int comp_ptr = (disp_ptr + 1) % DISPLACEMENT_WINDOW_SIZE; // calculate the comparison index

		// calculate the difference in the embedding coordinates across the window

		for( int i = 0; i < tier->t_embed->width*tier->t_embed->height; i++ ) {
			float x_disp = (displacement_window[ disp_ptr*(tier->t_embed->width*tier->t_embed->height*4) + i * 4 + 0 ] - displacement_bounds[disp_ptr*4+0])/displacement_bounds[disp_ptr*4+2] -
			(displacement_window[ comp_ptr*(tier->t_embed->width*tier->t_embed->height*4) + i * 4 + 0 ] - displacement_bounds[comp_ptr*4+0])/displacement_bounds[comp_ptr*4+2];
			float y_disp = (displacement_window[ disp_ptr*(tier->t_embed->width*tier->t_embed->height*4) + i * 4 + 1 ] - displacement_bounds[disp_ptr*4+1])/displacement_bounds[disp_ptr*4+3] -
			(displacement_window[ comp_ptr*(tier->t_embed->width*tier->t_embed->height*4) + i * 4 + 1 ] - displacement_bounds[comp_ptr*4+1])/displacement_bounds[comp_ptr*4+3];
			float disp = sqrt( x_disp*x_disp + y_disp*y_disp );
			if( disp > 1.e-5 ) normfactor++;
			displacement += disp;
		}

		// normalize by the number of points that actually moved

		if( normfactor ) {
			displacement /= (double) normfactor;
		}
	}

	// increment the displacement window pointer

	disp_ptr = (++disp_ptr) % DISPLACEMENT_WINDOW_SIZE;

	// cleanup

	return displacement;
}

/*
	Calculates a denominator for a modified stress metric.  

	if b_low_dim is true 
		returns LOW_DIM ^ 2
	else
		returns HIGH_DIM ^ 2
*/
float calc_sp_stress_denom( Tier *tier, bool b_low_dim ) {

	float sp_stress = 0.0f;

	int npoints = tier->t_embed->tier_width[tier->level]*tier->t_embed->tier_height[tier->level];
	if( tier->n_points < npoints )
		npoints = tier->n_points;
	float bottom1 = floor(((double)npoints)/((double)(tier->t_embed->tier_width[tier->level])));
	float bottom2 = ceil(((double)npoints)/((double)(tier->t_embed->tier_width[tier->level])));
	float width1 = (float)(npoints % tier->t_embed->tier_width[tier->level]);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_stress->texture_name[ tier->t_stress->attach_idx?0:1 ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_stress->width,tier->t_stress->height,
                    GL_RGBA,GL_FLOAT,zero_data);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_stress->texture_name[ tier->t_stress->attach_idx ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_stress->width,tier->t_stress->height,
                    GL_RGBA,GL_FLOAT,zero_data);

	// compute ( t_d | t_g ) ^ 2

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
	glUseProgramObjectARB( b_low_dim?(sh_sp_stress_denom_low.program):(sh_sp_stress_denom_high.program) );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

	if( b_low_dim ) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);
		glUniform1iARB(u_sp_stress_denom_low_d.location, 1); 
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_g->texture_name[tier->t_g->attach_idx]);
		glUniform1iARB(u_sp_stress_denom_low_g.location, 2); 
	}
	else {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);
		glUniform1iARB(u_sp_stress_denom_high_d.location, 1); 
	}

	// and render quad

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); 
	glVertex2f(0.0, 0.0);
	glTexCoord2f(tier->t_embed->tier_width[tier->level], 0.0); 
	glVertex2f(tier->t_embed->tier_width[tier->level], 0.0);
	glTexCoord2f(tier->t_embed->tier_width[tier->level], bottom1); 
	glVertex2f(tier->t_embed->tier_width[tier->level], bottom1);
	glTexCoord2f(0.0, bottom1); 
	glVertex2f(0.0, bottom1);
	if( bottom1 < bottom2 ) {
		glTexCoord2f(0.0, bottom1); 
		glVertex2f(0.0, bottom1);
		glTexCoord2f(width1, bottom1); 
		glVertex2f(width1, bottom1);
		glTexCoord2f(width1, bottom2); 
		glVertex2f(width1, bottom2);
		glTexCoord2f(0.0, bottom2); 
		glVertex2f(0.0, bottom2);
	}
	glEnd();

	//if( npoints == tier->n_points && b_low_dim  ) {
	////	printf("npoints = %d, level=%d gheightwidth= %dx%d \nembedheight=%d embedwidth=%d \nbottom1=%f bottom2=%f width1=%f\n", npoints, tier->level, tier->t_g->width, tier->t_g->height, tier->t_embed->tier_height[tier->level], tier->t_embed->tier_width[tier->level], bottom1, bottom2, width1 );
	//	outputTexture( tier->t_stress, "stress.txt");
	////	outputTexture( tier->t_embed, "embed.txt");
	//	outputTexture( tier->t_g, "g.txt");
	//	outputTexture( tier->t_d, "d.txt");
	//	exit ( 0 );
	//}

	// sum reduce to single float

	double reduction_passes = ceil( log( (double) (tier->t_embed->tier_width[tier->level]>tier->t_embed->tier_height[tier->level]?tier->t_embed->tier_width[tier->level]:tier->t_embed->tier_height[tier->level]) ) / log( 2.0 ) );

	for( int j = 0; j < (int) reduction_passes; j++ ) {

		tier->t_stress->attach_idx = tier->t_stress->attach_idx?0:1;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

		// bind data structures and active texture units
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
		glUseProgramObjectARB( sh_stress_reduce.program );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx?0:1]);
		glUniform1iARB(u_stress_reduce_old.location, 1); 
		
		glUniform1fARB(u_stress_reduce_offset.location, pow((float)2,j) );
		glUniform1fARB(u_stress_reduce_texwidth.location,tier->t_embed->width);
		glUniform1fARB(u_stress_reduce_texheight.location,tier->t_embed->height);

		// make quad filled to hit every pixel/texel
		glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
		glPolygonMode(GL_FRONT,GL_FILL);

		// and render quad
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0); 
			glVertex2f(0.0, 0.0);
			glTexCoord2f(tier->t_embed->tier_width[tier->level], 0.0); 
			glVertex2f(tier->t_embed->tier_width[tier->level], 0.0);
			glTexCoord2f(tier->t_embed->tier_width[tier->level], bottom1); 
			glVertex2f(tier->t_embed->tier_width[tier->level], bottom1);
			glTexCoord2f(0.0, bottom1); 
			glVertex2f(0.0, bottom1);

			if( bottom1 < bottom2 ) {
				glTexCoord2f(0.0, bottom1); 
				glVertex2f(0.0, bottom1);
				glTexCoord2f(width1, bottom1); 
				glVertex2f(width1, bottom1);
				glTexCoord2f(width1, bottom2); 
				glVertex2f(width1, bottom2);
				glTexCoord2f(0.0, bottom2); 
				glVertex2f(0.0, bottom2);
			}
		glEnd();
	}

	// return raw stress sum

	float tempstress[4] = {0.0f,0.0f,0.0f,0.0f};
	glReadBuffer(tier->t_stress->texture_name[tier->t_stress->attach_idx]);
	glReadPixels(0,0,1,1,GL_RGBA,GL_FLOAT,&tempstress);

	return tempstress[0];
}

/*
	Calculates a fast "sparse" raw stress metric.  

	returns SUMREDUCE( (HIGH_DIM - LOW_DIM) ^ 2 )
*/
float calc_sp_stress_raw( Tier *tier ) {

	float sp_stress = 0.0f;

	int npoints = tier->t_embed->tier_width[tier->level]*tier->t_embed->tier_height[tier->level];
	if( tier->n_points < npoints )
		npoints = tier->n_points;
	float bottom1 = floor(((double)npoints)/((double)(tier->t_embed->tier_width[tier->level])));
	float bottom2 = ceil(((double)npoints)/((double)(tier->t_embed->tier_width[tier->level])));
	float width1 = (float)(npoints % tier->t_embed->tier_width[tier->level]);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_stress->texture_name[ tier->t_stress->attach_idx?0:1 ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_stress->width,tier->t_stress->height,
                    GL_RGBA,GL_FLOAT,zero_data);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_stress->texture_name[ tier->t_stress->attach_idx ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_stress->width,tier->t_stress->height,
                    GL_RGBA,GL_FLOAT,zero_data);

	// compute ( t_d - t_g ) ^ 2

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
	glUseProgramObjectARB( sh_sp_stress.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);
	glUniform1iARB(u_sp_stress_d.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_g->texture_name[tier->t_g->attach_idx]);
	glUniform1iARB(u_sp_stress_g.location, 2); 

	// and render quad

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); 
	glVertex2f(0.0, 0.0);
	glTexCoord2f(tier->t_embed->tier_width[tier->level], 0.0); 
	glVertex2f(tier->t_embed->tier_width[tier->level], 0.0);
	glTexCoord2f(tier->t_embed->tier_width[tier->level], bottom1); 
	glVertex2f(tier->t_embed->tier_width[tier->level], bottom1);
	glTexCoord2f(0.0, bottom1); 
	glVertex2f(0.0, bottom1);
	if( bottom1 < bottom2 ) {
		glTexCoord2f(0.0, bottom1); 
		glVertex2f(0.0, bottom1);
		glTexCoord2f(width1, bottom1); 
		glVertex2f(width1, bottom1);
		glTexCoord2f(width1, bottom2); 
		glVertex2f(width1, bottom2);
		glTexCoord2f(0.0, bottom2); 
		glVertex2f(0.0, bottom2);
	}
	glEnd();

	//if( tier->level == 1 ) {
	//	printf("npoints = %d, level=%d gheightwidth= %dx%d \nembedheight=%d embedwidth=%d \nbottom1=%f bottom2=%f width1=%f\n", npoints, tier->level, tier->t_g->width, tier->t_g->height, tier->t_embed->tier_height[tier->level], tier->t_embed->tier_width[tier->level], bottom1, bottom2, width1 );
		//outputTexture( tier->t_stress, "stress.txt");
		//outputTextureLevel( tier->t_stress, 1, "stress.txt");
	//	outputTexture( tier->t_embed, "embed.txt");
	//	outputTexture( tier->t_g, "g.txt");
	//	outputTexture( tier->t_d, "d.txt");
		//exit ( 0 );
	//}

	// sum reduce to single float

	double reduction_passes = ceil( log( (double) (tier->t_embed->tier_width[tier->level]>tier->t_embed->tier_height[tier->level]?tier->t_embed->tier_width[tier->level]:tier->t_embed->tier_height[tier->level]) ) / log( 2.0 ) );

	for( int j = 0; j < (int) reduction_passes; j++ ) {

		tier->t_stress->attach_idx = tier->t_stress->attach_idx?0:1;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

		// bind data structures and active texture units
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
		glUseProgramObjectARB( sh_stress_reduce.program );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx?0:1]);
		glUniform1iARB(u_stress_reduce_old.location, 1); 
		
		glUniform1fARB(u_stress_reduce_offset.location, pow((float)2,j) );
		glUniform1fARB(u_stress_reduce_texwidth.location,tier->t_embed->tier_width[tier->level]);
		glUniform1fARB(u_stress_reduce_texheight.location,tier->t_embed->tier_height[tier->level]);

		// make quad filled to hit every pixel/texel
		glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
		glPolygonMode(GL_FRONT,GL_FILL);

		// and render quad
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0); 
			glVertex2f(0.0, 0.0);
			glTexCoord2f(tier->t_embed->tier_width[tier->level], 0.0); 
			glVertex2f(tier->t_embed->tier_width[tier->level], 0.0);
			glTexCoord2f(tier->t_embed->tier_width[tier->level], bottom1); 
			glVertex2f(tier->t_embed->tier_width[tier->level], bottom1);
			glTexCoord2f(0.0, bottom1); 
			glVertex2f(0.0, bottom1);

			if( bottom1 < bottom2 ) {
				glTexCoord2f(0.0, bottom1); 
				glVertex2f(0.0, bottom1);
				glTexCoord2f(width1, bottom1); 
				glVertex2f(width1, bottom1);
				glTexCoord2f(width1, bottom2); 
				glVertex2f(width1, bottom2);
				glTexCoord2f(0.0, bottom2); 
				glVertex2f(0.0, bottom2);
			}
		glEnd();
	}

	// return raw stress sum

	float tempstress[4] = {0.0f,0.0f,0.0f,0.0f};
	glReadBuffer(tier->t_stress->texture_name[tier->t_stress->attach_idx]);
	glReadPixels(0,0,1,1,GL_RGBA,GL_FLOAT,&tempstress);

	//if( tier->level == 1 ) {
	//printf("%f\n",tempstress[0]);
	//exit( 0 );
	//}
	return tempstress[0];//tempstress[0]/calc_sp_stress_denom(tier);//tempstress[0]>0.0001f?log(tempstress[0]):log(0.0001f);
}

/*
	Calculates a fast "sparse" stress1 metric.  
	
	SUMREDUCE( (HIGH_DIM - LOW_DIM) ^ 2 )  / LOW_DIM^2
*/
float calc_sp_stress_1( Tier *tier ) {
	return calc_sp_stress_raw( tier ) / calc_sp_stress_denom( tier, true );
}

/*
	Calculates a fast "sparse" normalized stress metric.  
	
	SUMREDUCE( (HIGH_DIM - LOW_DIM) ^ 2 )  / HIGH_DIM^2
*/
float calc_sp_stress_norm( Tier *tier ) {
	float temp1 = calc_sp_stress_raw( tier );
	float temp2 = calc_sp_stress_denom( tier, false );
	//printf("normstress = %f / %f = %f\n", temp1, temp2, temp1/temp2);
	return temp1/temp2;
}

float calc_sp_stress( Tier *tier ) {

	return calc_sp_stress_norm( tier );
}
//float calc_stress( Texture *stress, Texture *pts, Texture *embed ) {
float calc_stress( Tier *tier ) {

	float f_stress_num = 0.f;
	float f_stress_denom = 0.f;
	float bottom1;
	float bottom2;
	float width1;

	// loop for each of the points
	for( int i = 0; i < tier->n_points; i++ ) {

		if( b_output_debug ) {
			printf("%d.",i);
		}

		// calculate the initial difference

		if( !b_sparse_vector_input ) {

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

			// bind data structures and active texture units
			
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
			glUseProgramObjectARB( sh_stress_d_diff.program );
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_pts->texture_name[tier->t_pts->attach_idx]);
			glUniform1iARB(u_stress_d_pts.location, 1); 
			
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_reference->texture_name[tier->t_reference->attach_idx]);
			glUniform1iARB(u_stress_diff_reference.location, 2); 

			glUniform1fARB(u_stress_diff_pointwidth.location, ceil((float)n_dims/4.f) );
			glUniform1fARB(u_stress_d_inputptx.location, (float)(i % tier->t_perm->width )*ceil((float)n_dims/4.f) );
			glUniform1fARB(u_stress_d_inputpty.location, (float)(i / tier->t_perm->width ) );

			// make quad filled to hit every pixel/texel
			glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
			glPolygonMode(GL_FRONT,GL_FILL);

			// and render quad
			bottom1 = floor(((double)tier->n_points)/((double)tier->t_perm->width));
			bottom2 = ceil(((double)tier->n_points)/((double)tier->t_perm->width));
			width1 = (tier->n_points*(int)ceil((double)n_dims/4.0)) % tier->t_stress->width;
			glBegin(GL_QUADS);

				glTexCoord2f(0.0, 0.0); 
				glVertex2f(0.0, 0.0);
				glTexCoord2f(tier->t_stress->width, 0.0); 
				glVertex2f(tier->t_stress->width, 0.0);
				glTexCoord2f(tier->t_stress->width, bottom1); 
				glVertex2f(tier->t_stress->width, bottom1);
				glTexCoord2f(0.0, bottom1); 
				glVertex2f(0.0, bottom1);

				if( bottom1 < bottom2 ) {
					glTexCoord2f(0.0, bottom1); 
					glVertex2f(0.0, bottom1);
					glTexCoord2f(width1, bottom1); 
					glVertex2f(width1, bottom1);
					glTexCoord2f(width1, bottom2); 
					glVertex2f(width1, bottom2);
					glTexCoord2f(0.0, bottom2); 
					glVertex2f(0.0, bottom2);
				}
			glEnd();
		}
		else {		// handle sparse vector distances
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

			// bind data structures and active texture units
			
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
			glUseProgramObjectARB( sh_stress_d_dot.program );
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_pts->texture_name[tier->t_pts->attach_idx]);
			glUniform1iARB(u_stress_d_dot_pts.location, 1); 
			
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_reference->texture_name[tier->t_reference->attach_idx]);
			glUniform1iARB(u_stress_d_dot_reference.location, 2); 

			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_vec_idx->texture_name[tier->t_vec_idx->attach_idx]);
			glUniform1iARB(u_stress_d_dot_vec_idx.location, 3);		

			glUniform1fARB(u_stress_d_dot_pointwidth.location, ceil((float)n_dims/4.f) );
			glUniform1fARB(u_stress_d_dot_inputptx.location, (float)(i % tier->t_perm->width )*ceil((float)n_dims/4.f) );
			glUniform1fARB(u_stress_d_dot_inputpty.location, (float)(i / tier->t_perm->width ) );
			glUniform1fARB(u_stress_d_dot_maxnz.location, n_max_sparse_entries - 1 );
			glUniform1fARB(u_stress_d_dot_logmaxnz.location, (double) ceil( log( (double)n_max_sparse_entries )/log(2.0) ) );
			glUniform1fARB(u_stress_d_dot_halfN.location, pow( 2.0, (double) ceil( log( (double)n_max_sparse_entries )/log(2.0) ) - 1.0  ) );

			// make quad filled to hit every pixel/texel
			glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
			glPolygonMode(GL_FRONT,GL_FILL);

			// and render quad
			bottom1 = floor(((double)tier->n_points)/((double)tier->t_perm->width));
			bottom2 = ceil(((double)tier->n_points)/((double)tier->t_perm->width));
			width1 = (tier->n_points*(int)ceil((double)n_dims/4.0)) % tier->t_stress->width;
			glBegin(GL_QUADS);

				glTexCoord2f(0.0, 0.0); 
				glVertex2f(0.0, 0.0);
				glTexCoord2f(tier->t_stress->width, 0.0); 
				glVertex2f(tier->t_stress->width, 0.0);
				glTexCoord2f(tier->t_stress->width, bottom1); 
				glVertex2f(tier->t_stress->width, bottom1);
				glTexCoord2f(0.0, bottom1); 
				glVertex2f(0.0, bottom1);

				if( bottom1 < bottom2 ) {
					glTexCoord2f(0.0, bottom1); 
					glVertex2f(0.0, bottom1);
					glTexCoord2f(width1, bottom1); 
					glVertex2f(width1, bottom1);
					glTexCoord2f(width1, bottom2); 
					glVertex2f(width1, bottom2);
					glTexCoord2f(0.0, bottom2); 
					glVertex2f(0.0, bottom2);
				}
			glEnd();
		}


		//if( i == 2 ) {
		//	outputTexture( tier->t_reference, "ref.txt" );
			//outputTexture( tier->t_pts, "pts.txt" );
			//outputTexture( tier->t_vec_idx, "vec_idx.txt" );
			//outputTexture( tier->t_stress, "stress.txt" );
			//exit( 0 );
		//}

		// reduce the differences to a distance
		
		int chunkwidth = ceil((float)n_dims/4.f);
		while( chunkwidth > 0.0 ) {

			tier->t_stress->attach_idx = tier->t_stress->attach_idx?0:1;

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

			// bind data structures and active texture units
			
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
			glUseProgramObjectARB( sh_stress_d_reduce.program );
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx?0:1]);
			glUniform1iARB(u_stress_d_old.location, 1); 
			
			glUniform1fARB(u_stress_d_chunkwidth.location, chunkwidth);
			glUniform1fARB(u_stress_d_pointwidth.location,ceil((float)n_dims/4.f));

			// make quad filled to hit every pixel/texel
			glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
			glPolygonMode(GL_FRONT,GL_FILL);

			// and render quad
			glBegin(GL_QUADS);
				glTexCoord2f(0.0, 0.0); 
				glVertex2f(0.0, 0.0);
				glTexCoord2f(tier->t_stress->width, 0.0); 
				glVertex2f(tier->t_stress->width, 0.0);
				glTexCoord2f(tier->t_stress->width, bottom1); 
				glVertex2f(tier->t_stress->width, bottom1);
				glTexCoord2f(0.0, bottom1); 
				glVertex2f(0.0, bottom1);

				if( bottom1 < bottom2 ) {
					glTexCoord2f(0.0, bottom1); 
					glVertex2f(0.0, bottom1);
					glTexCoord2f(width1, bottom1); 
					glVertex2f(width1, bottom1);
					glTexCoord2f(width1, bottom2); 
					glVertex2f(width1, bottom2);
					glTexCoord2f(0.0, bottom2); 
					glVertex2f(0.0, bottom2);
				}
			glEnd();

			if( chunkwidth > 1 )
				chunkwidth = ceil((float)chunkwidth/4.f);
			else
				chunkwidth = 0;
		}

		//if( i == 1 )
			//outputTexture( tier->t_stress, "stress1.txt" );

		// compute and compare with the embedding distance
		if( !b_sparse_vector_input ) {

			tier->t_stress->attach_idx = tier->t_stress->attach_idx?0:1;

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

			// bind data structures and active texture units
			
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
			glUseProgramObjectARB( sh_stress_g.program );
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_embed->texture_name[tier->t_embed->attach_idx]);
			glUniform1iARB(u_stress_g_embed.location, 1); 
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx?0:1]);
			glUniform1iARB(u_stress_g_old.location, 2);
			
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_reference->texture_name[tier->t_reference->attach_idx]);
			glUniform1iARB(u_stress_g_reference.location, 3);

			glUniform1fARB(u_stress_g_inputptx.location, (float)(i % tier->t_perm->width ) );
			glUniform1fARB(u_stress_g_inputpty.location, (float)(i / tier->t_perm->width ) );
			glUniform1fARB(u_stress_g_pointwidth.location, ceil((float)n_dims/4.f) );

			// make quad filled to hit every pixel/texel
			glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
			glPolygonMode(GL_FRONT,GL_FILL);

			// and render quad
			bottom1 = floor(((double)tier->n_points)/((double)tier->t_embed->width));
			bottom2 = ceil(((double)tier->n_points)/((double)tier->t_embed->width));
			width1 = tier->n_points % tier->t_embed->width;
			glBegin(GL_QUADS);
					glTexCoord2f(0.0, 0.0); 
					glVertex2f(0.0, 0.0);
					glTexCoord2f(tier->t_embed->width, 0.0); 
					glVertex2f(tier->t_embed->width, 0.0);
					glTexCoord2f(tier->t_embed->width, bottom1); 
					glVertex2f(tier->t_embed->width, bottom1);
					glTexCoord2f(0.0, bottom1); 
					glVertex2f(0.0, bottom1);

					if( bottom1 < bottom2 ) {
						glTexCoord2f(0.0, bottom1); 
						glVertex2f(0.0, bottom1);
						glTexCoord2f(width1, bottom1); 
						glVertex2f(width1, bottom1);
						glTexCoord2f(width1, bottom2); 
						glVertex2f(width1, bottom2);
						glTexCoord2f(0.0, bottom2); 
						glVertex2f(0.0, bottom2);
					}
			glEnd();
		}
		else {	// handle cosine distances
			tier->t_stress->attach_idx = tier->t_stress->attach_idx?0:1;

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

			// bind data structures and active texture units
			
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
			glUseProgramObjectARB( sh_stress_g_dot.program );
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_embed->texture_name[tier->t_embed->attach_idx]);
			glUniform1iARB(u_stress_g_dot_embed.location, 1); 
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx?0:1]);
			glUniform1iARB(u_stress_g_dot_old.location, 2);
			
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_reference->texture_name[tier->t_reference->attach_idx]);
			glUniform1iARB(u_stress_g_dot_reference.location, 3);

			glUniform1fARB(u_stress_g_dot_inputptx.location, (float)(i % tier->t_perm->width ) );
			glUniform1fARB(u_stress_g_dot_inputpty.location, (float)(i / tier->t_perm->width ) );
			glUniform1fARB(u_stress_g_dot_pointwidth.location, ceil((float)n_dims/4.f) );

			// make quad filled to hit every pixel/texel
			glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
			glPolygonMode(GL_FRONT,GL_FILL);

			// and render quad
			bottom1 = floor(((double)tier->n_points)/((double)tier->t_embed->width));
			bottom2 = ceil(((double)tier->n_points)/((double)tier->t_embed->width));
			width1 = tier->n_points % tier->t_embed->width;
			glBegin(GL_QUADS);
					glTexCoord2f(0.0, 0.0); 
					glVertex2f(0.0, 0.0);
					glTexCoord2f(tier->t_embed->width, 0.0); 
					glVertex2f(tier->t_embed->width, 0.0);
					glTexCoord2f(tier->t_embed->width, bottom1); 
					glVertex2f(tier->t_embed->width, bottom1);
					glTexCoord2f(0.0, bottom1); 
					glVertex2f(0.0, bottom1);

					if( bottom1 < bottom2 ) {
						glTexCoord2f(0.0, bottom1); 
						glVertex2f(0.0, bottom1);
						glTexCoord2f(width1, bottom1); 
						glVertex2f(width1, bottom1);
						glTexCoord2f(width1, bottom2); 
						glVertex2f(width1, bottom2);
						glTexCoord2f(0.0, bottom2); 
						glVertex2f(0.0, bottom2);
					}
			glEnd();
		}
		//outputTexture(tier->t_stress,"stress.txt");
		//exit( 0 );

		// reduce both numerator and denominator to a single number at the origin

		double temp = ceil( log( (double) (tier->t_embed->width>tier->t_embed->height?tier->t_embed->width:tier->t_embed->height) ) / log( 2.0 ) );
		bottom1 = floor(((double)tier->n_points)/((double)tier->t_embed->width));
		bottom2 = ceil(((double)tier->n_points)/((double)tier->t_embed->width));
		//width1 = (tier->n_points*(int)ceil((double)n_embed_dims/4.0)) % tier->t_embed->width;
		for( int j = 0; j < (int) temp; j++ ) {

			tier->t_stress->attach_idx = tier->t_stress->attach_idx?0:1;

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

			// bind data structures and active texture units
			
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
			glUseProgramObjectARB( sh_stress_reduce_dual.program );
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx?0:1]);
			glUniform1iARB(u_stress_reduce_dual_old.location, 1); 
			
			glUniform1fARB(u_stress_reduce_dual_offset.location, pow((float)2,j) );
			glUniform1fARB(u_stress_reduce_dual_texwidth.location,tier->t_embed->width);
			glUniform1fARB(u_stress_reduce_dual_texheight.location,tier->t_embed->height);

			// make quad filled to hit every pixel/texel
			glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
			glPolygonMode(GL_FRONT,GL_FILL);

			// and render quad
			glBegin(GL_QUADS);
				glTexCoord2f(0.0, 0.0); 
				glVertex2f(0.0, 0.0);
				glTexCoord2f(tier->t_embed->width, 0.0); 
				glVertex2f(tier->t_embed->width, 0.0);
				glTexCoord2f(tier->t_embed->width, bottom1); 
				glVertex2f(tier->t_embed->width, bottom1);
				glTexCoord2f(0.0, bottom1); 
				glVertex2f(0.0, bottom1);

				if( bottom1 < bottom2 ) {
					glTexCoord2f(0.0, bottom1); 
					glVertex2f(0.0, bottom1);
					glTexCoord2f(width1, bottom1); 
					glVertex2f(width1, bottom1);
					glTexCoord2f(width1, bottom2); 
					glVertex2f(width1, bottom2);
					glTexCoord2f(0.0, bottom2); 
					glVertex2f(0.0, bottom2);
				}
			glEnd();
		}

		//if( i == 1 ) {
		//	outputTexture( tier->t_stress, "stress3.txt" );
		//	exit( 0 );
		//}

		// read the number and add it to our total
		glReadBuffer(tier->t_stress->texture_name[tier->t_stress->attach_idx]);
		float tempstress[4] = {0.0f,0.0f,0.0f,0.0f};
		glReadPixels(0,0,1,1,GL_RGBA,GL_FLOAT,&tempstress);
		f_stress_num += tempstress[0];
		f_stress_denom += tempstress[1];

		//// compute the denominator

		//glMatrixMode(GL_PROJECTION);
		//glLoadIdentity();
		//gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
		//glMatrixMode(GL_MODELVIEW);
		//glLoadIdentity();
		//glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

		//// bind data structures and active texture units
		//
		//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
		//glUseProgramObjectARB( sh_stress_g_denom.program );
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

		//glActiveTexture(GL_TEXTURE1);
		//glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_embed->texture_name[tier->t_embed->attach_idx]);
		//glUniform1iARB(u_stress_g_denom_embed.location, 1); 
		//
		//glActiveTexture(GL_TEXTURE2);
		//glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_reference->texture_name[tier->t_reference->attach_idx]);
		//glUniform1iARB(u_stress_g_denom_reference.location, 2); 
		//
		//glUniform1fARB(u_stress_g_denom_inputptx.location, (float)(i % tier->t_perm->width ) );
		//glUniform1fARB(u_stress_g_denom_inputpty.location, (float)(i / tier->t_perm->width ) );
		//glUniform1fARB(u_stress_g_denom_pointwidth.location, ceil((float)n_dims/4.f) );

		//// make quad filled to hit every pixel/texel
		//glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
		//glPolygonMode(GL_FRONT,GL_FILL);

		//// and render quad
		//glBegin(GL_QUADS);
		//		glTexCoord2f(0.0, 0.0); 
		//		glVertex2f(0.0, 0.0);
		//		glTexCoord2f(tier->t_embed->width, 0.0); 
		//		glVertex2f(tier->t_embed->width, 0.0);
		//		glTexCoord2f(tier->t_embed->width, bottom1); 
		//		glVertex2f(tier->t_embed->width, bottom1);
		//		glTexCoord2f(0.0, bottom1); 
		//		glVertex2f(0.0, bottom1);

		//		if( bottom1 < bottom2 ) {
		//			glTexCoord2f(0.0, bottom1); 
		//			glVertex2f(0.0, bottom1);
		//			glTexCoord2f(width1, bottom1); 
		//			glVertex2f(width1, bottom1);
		//			glTexCoord2f(width1, bottom2); 
		//			glVertex2f(width1, bottom2);
		//			glTexCoord2f(0.0, bottom2); 
		//			glVertex2f(0.0, bottom2);
		//		}
		//glEnd();

		//for( int j = 0; j < (int) temp; j++ ) {

		//	tier->t_stress->attach_idx = tier->t_stress->attach_idx?0:1;

		//	glMatrixMode(GL_PROJECTION);
		//	glLoadIdentity();
		//	gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
		//	glMatrixMode(GL_MODELVIEW);
		//	glLoadIdentity();
		//	glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

		//	// bind data structures and active texture units
		//	
		//	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
		//	glUseProgramObjectARB( sh_stress_reduce.program );
		//	glActiveTexture(GL_TEXTURE0);
		//	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

		//	glActiveTexture(GL_TEXTURE1);
		//	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx?0:1]);
		//	glUniform1iARB(u_stress_reduce_old.location, 1); 
		//	
		//	glUniform1fARB(u_stress_reduce_offset.location, pow((float)2,j) );
		//	glUniform1fARB(u_stress_reduce_texwidth.location,tier->t_embed->width);
		//	glUniform1fARB(u_stress_reduce_texheight.location,tier->t_embed->height);

		//	// make quad filled to hit every pixel/texel
		//	glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
		//	glPolygonMode(GL_FRONT,GL_FILL);

		//	// and render quad
		//	glBegin(GL_QUADS);
		//		glTexCoord2f(0.0, 0.0); 
		//		glVertex2f(0.0, 0.0);
		//		glTexCoord2f(tier->t_embed->width, 0.0); 
		//		glVertex2f(tier->t_embed->width, 0.0);
		//		glTexCoord2f(tier->t_embed->width, bottom1); 
		//		glVertex2f(tier->t_embed->width, bottom1);
		//		glTexCoord2f(0.0, bottom1); 
		//		glVertex2f(0.0, bottom1);

		//		if( bottom1 < bottom2 ) {
		//			glTexCoord2f(0.0, bottom1); 
		//			glVertex2f(0.0, bottom1);
		//			glTexCoord2f(width1, bottom1); 
		//			glVertex2f(width1, bottom1);
		//			glTexCoord2f(width1, bottom2); 
		//			glVertex2f(width1, bottom2);
		//			glTexCoord2f(0.0, bottom2); 
		//			glVertex2f(0.0, bottom2);
		//		}
		//	glEnd();
		//}

		//// read the number and add it to our total
		//glReadBuffer(tier->t_stress->texture_name[tier->t_stress->attach_idx]);
		//glReadPixels(0,0,1,1,GL_RGBA,GL_FLOAT,&tempstress);
		//f_stress_denom += tempstress[0];
	}

	return (f_stress_num / f_stress_denom);
	//return f_stress_num;

}

float sum_velocity( Tier *tier, bool b_smartnorm ) {
	
	if( ! g_b_useVel )
		return 0.;

	float f_velocity_sum = 0.f;

	double temp = ceil( log( (double) (tier->t_velocity->width>tier->t_velocity->height?tier->t_velocity->width:tier->t_velocity->height) ) / log( 2.0 ) );
	
	float bottom1 = floor(((double)tier->n_points)/((double)tier->t_embed->width));
	float bottom2 = ceil(((double)tier->n_points)/((double)tier->t_embed->width));
	float width1 = (float)(tier->n_points % tier->t_embed->width);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_stress->texture_name[ tier->t_stress->attach_idx?0:1 ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_stress->width,tier->t_stress->height,
                    GL_RGBA,GL_FLOAT,zero_data);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_stress->texture_name[ tier->t_stress->attach_idx ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_stress->width,tier->t_stress->height,
                    GL_RGBA,GL_FLOAT,zero_data);

	for( int j = 0; j < (int) temp; j++ ) {

		tier->t_stress->attach_idx = tier->t_stress->attach_idx?0:1;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

		// bind data structures and active texture units
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
		glUseProgramObjectARB( sh_stress_reduce.program );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

		if( j == 0 ) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_velocity->texture_name[tier->t_velocity->attach_idx]);
			glUniform1iARB(u_stress_reduce_old.location, 1); 
		}
		else {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx?0:1]);
			glUniform1iARB(u_stress_reduce_old.location, 1); 
		}
		
		glUniform1fARB(u_stress_reduce_offset.location, pow((float)2,j) );
		glUniform1fARB(u_stress_reduce_texwidth.location,tier->t_embed->width);
		glUniform1fARB(u_stress_reduce_texheight.location,tier->t_embed->height);

		// make quad filled to hit every pixel/texel
		glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
		glPolygonMode(GL_FRONT,GL_FILL);

		// and render quad
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0); 
			glVertex2f(0.0, 0.0);
			glTexCoord2f(tier->t_velocity->width, 0.0); 
			glVertex2f(tier->t_velocity->width, 0.0);
			glTexCoord2f(tier->t_velocity->width, bottom1); 
			glVertex2f(tier->t_velocity->width, bottom1);
			glTexCoord2f(0.0, bottom1); 
			glVertex2f(0.0, bottom1);

			if( bottom1 < bottom2 ) {
				glTexCoord2f(0.0, bottom1); 
				glVertex2f(0.0, bottom1);
				glTexCoord2f(width1, bottom1); 
				glVertex2f(width1, bottom1);
				glTexCoord2f(width1, bottom2); 
				glVertex2f(width1, bottom2);
				glTexCoord2f(0.0, bottom2); 
				glVertex2f(0.0, bottom2);
			}
		glEnd();
	}

	// read the number and add it to our total
	float tempvelocity[4] = {0.0f,0.0f,0.0f,0.0f};
	glReadBuffer(tier->t_stress->texture_name[tier->t_stress->attach_idx]);
	glReadPixels(0,0,1,1,GL_RGBA,GL_FLOAT,&tempvelocity);
	f_velocity_sum = sqrt( tempvelocity[0]*tempvelocity[0] +  tempvelocity[1]*tempvelocity[1] );

	// calculate the denominator
	if( 1 ) {

		if( b_smartnorm ) {
			f_velocity_sum /= ((tier->t_perm->tier_width[tier->level]*tier->t_perm->tier_width[tier->level])>tier->n_points)?
								tier->n_points:
								(tier->t_perm->tier_width[tier->level]*tier->t_perm->tier_width[tier->level]);	
		}
		else 
			f_velocity_sum /= tier->n_points;
	}
	else {
		int nz_vel_points = 0;
		float *temp_buf = NULL;
		if( (temp_buf = (float * ) malloc( sizeof( float ) * 4 * tier->t_velocity->width * tier->t_velocity->height ) ) == NULL ) {
			printf("ERROR: cannot allocate output buffer.");
			exit( 0 );
		}
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_velocity->frame_buffer);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_velocity->texture_name[tier->t_velocity->attach_idx]);
		glReadBuffer(attachmentpoints[tier->t_velocity->attach_idx]);
		glReadPixels(0, 0, tier->t_velocity->width, tier->t_velocity->height,GL_RGBA,GL_FLOAT,temp_buf);

		for( int i = 0; i < (4 * tier->t_velocity->width * tier->t_velocity->height); i+= 4 ) {
			if( fabs(temp_buf[i]) > 1e-6 || fabs(temp_buf[i+1]) > 1e-6 )
				nz_vel_points++;
		}
		free( temp_buf );
		f_velocity_sum /= nz_vel_points;
	}



	
	/*f_velocity_sum /= ((tier->t_perm->tier_width[tier->level]*tier->t_perm->tier_width[tier->level])>tier->n_points)?
						tier->n_points:
						(tier->t_perm->tier_width[tier->level]*tier->t_perm->tier_width[tier->level]);*/

	return f_velocity_sum;
}

float sum_all_forces( Tier *tier, bool b_smartnorm ) {

	float f_force_sum = 0.f;

	double temp = ceil( log( (double) (tier->t_force->width>tier->t_force->height?tier->t_force->width:tier->t_force->height) ) / log( 2.0 ) );
	
	float bottom1 = floor(((double)tier->n_points)/((double)tier->t_embed->width));
	float bottom2 = ceil(((double)tier->n_points)/((double)tier->t_embed->width));
	float width1 = (float)(tier->n_points % tier->t_embed->width);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_stress->texture_name[ tier->t_stress->attach_idx?0:1 ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_stress->width,tier->t_stress->height,
                    GL_RGBA,GL_FLOAT,zero_data);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_stress->texture_name[ tier->t_stress->attach_idx ]);
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_stress->width,tier->t_stress->height,
                    GL_RGBA,GL_FLOAT,zero_data);

	for( int j = 0; j < (int) temp; j++ ) {

		tier->t_stress->attach_idx = tier->t_stress->attach_idx?0:1;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0,tier->t_stress->width,0.0,tier->t_stress->height);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glViewport(0,0,tier->t_stress->width,tier->t_stress->height);

		// bind data structures and active texture units
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_stress->frame_buffer);
		glUseProgramObjectARB( sh_stress_reduce.program );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx]);

		if( j == 0 ) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_force->texture_name[tier->t_force->attach_idx]);
			glUniform1iARB(u_stress_reduce_old.location, 1); 
		}
		else {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_stress->texture_name[tier->t_stress->attach_idx?0:1]);
			glUniform1iARB(u_stress_reduce_old.location, 1); 
		}
		
		glUniform1fARB(u_stress_reduce_offset.location, pow((float)2,j) );
		glUniform1fARB(u_stress_reduce_texwidth.location,tier->t_embed->width);
		glUniform1fARB(u_stress_reduce_texheight.location,tier->t_embed->height);

		// make quad filled to hit every pixel/texel
		glDrawBuffer (attachmentpoints[tier->t_stress->attach_idx]);
		glPolygonMode(GL_FRONT,GL_FILL);

		// and render quad
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0); 
			glVertex2f(0.0, 0.0);
			glTexCoord2f(tier->t_force->width, 0.0); 
			glVertex2f(tier->t_force->width, 0.0);
			glTexCoord2f(tier->t_force->width, bottom1); 
			glVertex2f(tier->t_force->width, bottom1);
			glTexCoord2f(0.0, bottom1); 
			glVertex2f(0.0, bottom1);

			if( bottom1 < bottom2 ) {
				glTexCoord2f(0.0, bottom1); 
				glVertex2f(0.0, bottom1);
				glTexCoord2f(width1, bottom1); 
				glVertex2f(width1, bottom1);
				glTexCoord2f(width1, bottom2); 
				glVertex2f(width1, bottom2);
				glTexCoord2f(0.0, bottom2); 
				glVertex2f(0.0, bottom2);
			}
		glEnd();
	}

	// read the number and add it to our total
	float tempforce[4] = {0.0f,0.0f,0.0f,0.0f};
	glReadBuffer(tier->t_stress->texture_name[tier->t_stress->attach_idx]);
	glReadPixels(0,0,1,1,GL_RGBA,GL_FLOAT,&tempforce);
	f_force_sum = sqrt( tempforce[0]*tempforce[0] +  tempforce[1]*tempforce[1] );

	// calculate the denominator
	if( 1 ) {

		if( b_smartnorm ) {
			f_force_sum /= ((tier->t_perm->tier_width[tier->level]*tier->t_perm->tier_width[tier->level])>tier->n_points)?
								tier->n_points:
								(tier->t_perm->tier_width[tier->level]*tier->t_perm->tier_width[tier->level]);	
		}
		else {
			f_force_sum /= tier->n_points;
		}
	}
	else {
		int nz_vel_points = 0;
		float *temp_buf = NULL;
		if( (temp_buf = (float * ) malloc( sizeof( float ) * 4 * tier->t_force->width * tier->t_force->height ) ) == NULL ) {
			printf("ERROR: cannot allocate output buffer.");
			exit( 0 );
		}
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_force->frame_buffer);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_force->texture_name[tier->t_force->attach_idx]);
		glReadBuffer(attachmentpoints[tier->t_force->attach_idx]);
		glReadPixels(0, 0, tier->t_force->width, tier->t_force->height,GL_RGBA,GL_FLOAT,temp_buf);

		for( int i = 0; i < (4 * tier->t_force->width * tier->t_force->height); i+= 4 ) {
			if( fabs(temp_buf[i]) > 1e-6 || fabs(temp_buf[i+1]) > 1e-6 )
				nz_vel_points++;
		}
		free( temp_buf );
		f_force_sum /= nz_vel_points;
	}

	return f_force_sum;
}

