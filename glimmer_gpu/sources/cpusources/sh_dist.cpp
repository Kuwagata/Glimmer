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
#include "params.h"
#include "feeder.h"

extern Feeder g_feeder;
extern bool b_use_paging;
extern int g_n_paging_iterations;
extern bool b_output_debug;
extern unsigned int g_n_framecount;
//extern int n_set_points;
extern int n_dims;
extern int n_embed_dims;
extern int n_smax;
extern int n_vmax;
extern int n_debug;
extern int n_max_sparse_entries;
//extern Texture t_idx;
//extern Texture t_pts;
//extern Texture t_embed;
//extern GLuint vb_setBuffer;
extern GLenum attachmentpoints[];
extern char g_shader_path[4092];

Shader sh_dist_g;			// computes embedding distance (assumes embedding dimension < 5)
Shader sh_dist_d_lookup;	// reads high dimensional distance from a lookup table
Shader sh_page;				// reads high dimensional distance from a lookup table paged in bit by bit
Shader sh_dot_term;			// computes the dot product over a sparse vector using binary index searches
Shader sh_dist_d_diff;		// computes high dimensional distance (L2)
Shader sh_dist_d_reduce;	// computes high dimensional distance (L2)
Shader sh_dist_d_copy;		// copies the sum and sqrt's the sonofagun over to the d texture
Shader sh_dot_copy;			// copies the sum, subracts the value from 1 over to the d texture (for cosine dist)
Shader sh_interpolate;		// Positions a point at the weighted barycenter of the points in its near set.

Uniform u_interpolate_idx;			// index set
Uniform u_interpolate_d;			// high-d-distances
Uniform u_interpolate_embed;		// low-d-coords
Uniform u_interpolate_embedwidth;	// low-d-coord texture width
Uniform u_interpolate_distwidth;	// high-d-distance texture width
Uniform u_interpolate_binsperpt;	// 

Uniform u_page_page;			// the page snagged from D''
Uniform u_page_dwidth;			// width of a distance element
Uniform u_page_dspan;			// number of distance elements in a span
Uniform u_page_pspan;			// number of elements in a span of a page
Uniform u_page_iterationoffs;	// iteration within page

Uniform u_g_embed;
Uniform u_g_idx;
Uniform u_g_embedwidth;
Uniform u_g_distwidth;
Uniform u_g_binsperpt;

Uniform u_diff_pts;
Uniform u_diff_idx;
Uniform u_diff_pointwidth;
Uniform u_diff_diffwidth;
Uniform u_diff_idxwidth;
Uniform u_diff_chunkwidth;
Uniform u_diff_binsperpt;

Uniform u_reduce_diff;
Uniform u_reduce_oldchunkwidth;
Uniform u_reduce_newchunkwidth;
Uniform u_reduce_preselec;
Uniform u_reduce_postselec;

Uniform u_copy_diff;

Uniform u_dot_copy;

Uniform u_lookup_pts;
Uniform u_lookup_idx;
Uniform u_lookup_perm;
Uniform u_lookup_lookupheight;
Uniform u_lookup_lookupwidth;
Uniform u_lookup_diffwidth;
Uniform u_lookup_pointcount;
Uniform u_lookup_refwidth;

Uniform u_dot_term_pts;			// values texture
Uniform u_dot_term_vec_idx;		// indices texture
Uniform u_dot_term_idx;			// set index texture
Uniform u_dot_term_pointwidth;	// width of a single point
Uniform u_dot_term_diffwidth;	// width of a difference element
Uniform u_dot_term_idxwidth;	// width of a single index set
Uniform u_dot_term_chunkwidth;	// width of a reduction element
Uniform u_dot_term_binsperpt;	// typically the same as pointwidth
Uniform u_dot_term_maxnz;		// the maximum number of nonzeros - 1
Uniform u_dot_term_logmaxnz;	// ceil(log2(maxnz+1));
Uniform u_dot_term_halfN;		// pow(logmaxnz-1,2.0);


void setup_sh_dist( ) {

	int i,j;
	char number_str[255];
	char letter_str[255];
	char shader_path[8192];

	// setup sh_interpolate
	sh_interpolate.program = glCreateProgramObjectARB();
	sh_interpolate.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_interpolate.program, sh_interpolate.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_interpolate.glsl");
	string interpolate_string = readShaderSource(shader_path);
	const GLcharARB *interpolate_source = interpolate_string.c_str();
	glShaderSourceARB(sh_interpolate.shader, 1, &interpolate_source, NULL);
	glCompileShaderARB(sh_interpolate.shader);
	glLinkProgramARB(sh_interpolate.program);
	printInfoLog( "sh_interpolate", sh_interpolate.shader );
	u_interpolate_idx.location			= glGetUniformLocationARB( sh_interpolate.program, "t_idx" );
	u_interpolate_d.location			= glGetUniformLocationARB( sh_interpolate.program, "t_d" );
	u_interpolate_embed.location		= glGetUniformLocationARB( sh_interpolate.program, "t_embed" );
	u_interpolate_embedwidth.location	= glGetUniformLocationARB( sh_interpolate.program, "embedwidth" );
	u_interpolate_distwidth.location	= glGetUniformLocationARB( sh_interpolate.program, "distwidth" );
	u_interpolate_binsperpt.location	= glGetUniformLocationARB( sh_interpolate.program, "binsperpt" );
	u_interpolate_distwidth.valuef = (float)(((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 )));
	u_interpolate_binsperpt.valuef = (float) ceil( (double)n_embed_dims / 4.0 );

	// setup sh_dot_term
	sh_dot_term.program = glCreateProgramObjectARB();
	sh_dot_term.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_dot_term.program, sh_dot_term.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_dot_term.glsl");
	string dot_term_string = readShaderSource(shader_path);
	const GLcharARB *dot_term_source = dot_term_string.c_str();
	glShaderSourceARB(sh_dot_term.shader, 1, &dot_term_source, NULL);
	glCompileShaderARB(sh_dot_term.shader);
	glLinkProgramARB(sh_dot_term.program);
	printInfoLog( "sh_dot_term", sh_dot_term.shader );
	u_dot_term_vec_idx.location		= glGetUniformLocationARB( sh_dot_term.program, "vec_idx" );
	u_dot_term_pts.location		    = glGetUniformLocationARB( sh_dot_term.program, "pts" );
	u_dot_term_idx.location			= glGetUniformLocationARB( sh_dot_term.program, "idx" );
	u_dot_term_pointwidth.location	= glGetUniformLocationARB( sh_dot_term.program, "pointwidth" );
	u_dot_term_diffwidth.location	= glGetUniformLocationARB( sh_dot_term.program, "diffwidth" );
	u_dot_term_idxwidth.location	= glGetUniformLocationARB( sh_dot_term.program, "idxwidth" );
	u_dot_term_chunkwidth.location	= glGetUniformLocationARB( sh_dot_term.program, "chunkwidth" );
	u_dot_term_binsperpt.location	= glGetUniformLocationARB( sh_dot_term.program, "binsperpt" );
	u_dot_term_maxnz.location		= glGetUniformLocationARB( sh_dot_term.program, "maxnz" );
	u_dot_term_logmaxnz.location	= glGetUniformLocationARB( sh_dot_term.program, "logmaxnz" );
	u_dot_term_halfN.location		= glGetUniformLocationARB( sh_dot_term.program, "halfN" );

	// setup sh_page
	sh_page.program = glCreateProgramObjectARB();
	sh_page.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_page.program, sh_page.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_page.glsl");
	string page_string = readShaderSource(shader_path);
	const GLcharARB *page_source = page_string.c_str();
	glShaderSourceARB(sh_page.shader, 1, &page_source, NULL);
	glCompileShaderARB(sh_page.shader);
	glLinkProgramARB(sh_page.program);
	printInfoLog( "sh_page", sh_page.shader );
	u_page_page.location			= glGetUniformLocationARB( sh_page.program, "page" );
	u_page_dwidth.location			= glGetUniformLocationARB( sh_page.program, "dwidth" );
	u_page_dspan.location			= glGetUniformLocationARB( sh_page.program, "dspan" );
	u_page_pspan.location			= glGetUniformLocationARB( sh_page.program, "pspan" );
	u_page_iterationoffs.location	= glGetUniformLocationARB( sh_page.program, "iterationoffs" );

	// setup sh_dist_d_lookup
	sh_dist_d_lookup.program = glCreateProgramObjectARB();
	sh_dist_d_lookup.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_dist_d_lookup.program, sh_dist_d_lookup.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_dist_lookup.glsl");
	string dist_lookup_string = readShaderSource(shader_path);
	const GLcharARB *dist_lookup_source = dist_lookup_string.c_str();
	glShaderSourceARB(sh_dist_d_lookup.shader, 1, &dist_lookup_source, NULL);
	glCompileShaderARB(sh_dist_d_lookup.shader);
	glLinkProgramARB(sh_dist_d_lookup.program);
	printInfoLog( "sh_dist_d_lookup", sh_dist_d_lookup.shader );
	u_lookup_pts.isFloat = false;
	u_lookup_idx.isFloat = false;
	u_lookup_perm.isFloat = false;
	u_lookup_lookupheight.isFloat = true;
	u_lookup_lookupwidth.isFloat = true;
	u_lookup_diffwidth.isFloat = true;
	u_lookup_pointcount.isFloat = true;
	u_lookup_refwidth.isFloat = true;
	u_lookup_pts.location = glGetUniformLocationARB( sh_dist_d_lookup.program, "pts" );
	u_lookup_idx.location = glGetUniformLocationARB( sh_dist_d_lookup.program, "idx" );
	u_lookup_perm.location = glGetUniformLocationARB( sh_dist_d_lookup.program, "perm" );
	u_lookup_lookupheight.location = glGetUniformLocationARB( sh_dist_d_lookup.program, "lookupheight" );
	u_lookup_lookupwidth.location = glGetUniformLocationARB( sh_dist_d_lookup.program, "lookupwidth" );
	u_lookup_diffwidth.location = glGetUniformLocationARB( sh_dist_d_lookup.program, "diffwidth" );
	u_lookup_pointcount.location = glGetUniformLocationARB( sh_dist_d_lookup.program, "pointcount" );
	u_lookup_refwidth.location = glGetUniformLocationARB( sh_dist_d_lookup.program, "refwidth" );

	// setup sh_dist_d_diff
	sh_dist_d_diff.program = glCreateProgramObjectARB();
	sh_dist_d_diff.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_dist_d_diff.program, sh_dist_d_diff.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_dist_diff.glsl");
	string dist_diff_string = readShaderSource(shader_path);
	const GLcharARB *dist_diff_source = dist_diff_string.c_str();
	glShaderSourceARB(sh_dist_d_diff.shader, 1, &dist_diff_source, NULL);
	glCompileShaderARB(sh_dist_d_diff.shader);
	glLinkProgramARB(sh_dist_d_diff.program);
	printInfoLog( "sh_dist_d_diff", sh_dist_d_diff.shader );
	u_diff_pts.isFloat = false;
	u_diff_idx.isFloat = false;
	u_diff_pointwidth.isFloat = true;	
	u_diff_diffwidth.isFloat = true;
	u_diff_idxwidth.isFloat = true;
	u_diff_chunkwidth.isFloat = true;
	u_diff_binsperpt.isFloat = true;
	u_diff_pts.location = glGetUniformLocationARB( sh_dist_d_diff.program, "pts" );
	u_diff_idx.location = glGetUniformLocationARB( sh_dist_d_diff.program, "idx" );
	u_diff_pointwidth.location = glGetUniformLocationARB( sh_dist_d_diff.program, "pointwidth" );
	u_diff_diffwidth.location = glGetUniformLocationARB( sh_dist_d_diff.program, "diffwidth" );
	u_diff_idxwidth.location = glGetUniformLocationARB( sh_dist_d_diff.program, "idxwidth" );
	u_diff_chunkwidth.location = glGetUniformLocationARB( sh_dist_d_diff.program, "chunkwidth" );
	u_diff_binsperpt.location = glGetUniformLocationARB( sh_dist_d_diff.program, "binsperpt" );
	u_diff_diffwidth.valuef = (float)(((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 )))*(float) ceil( (double)n_dims / 4.0 );
	u_diff_idxwidth.valuef = (float)(((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 )));
	u_diff_chunkwidth.valuef = (float) ceil( (double)n_dims / 4.0 );
	u_diff_binsperpt.valuef = (float) ceil( (double)n_dims / 4.0 );

	// setup sh_dist_d_reduce
	sh_dist_d_reduce.program = glCreateProgramObjectARB();
	sh_dist_d_reduce.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_dist_d_reduce.program, sh_dist_d_reduce.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_dist_reduce.glsl");
	string dist_reduce_string = readShaderSource(shader_path);
	const GLcharARB *dist_reduce_source = dist_reduce_string.c_str();
	glShaderSourceARB(sh_dist_d_reduce.shader, 1, &dist_reduce_source, NULL);
	glCompileShaderARB(sh_dist_d_reduce.shader);
	glLinkProgramARB(sh_dist_d_reduce.program);
	printInfoLog( "sh_dist_d_reduce", sh_dist_d_reduce.shader );
	u_reduce_diff.isFloat = false;
	u_reduce_oldchunkwidth.isFloat = true;
	u_reduce_newchunkwidth.isFloat = true;
	u_reduce_preselec.isFloat = true;
	u_reduce_postselec.isFloat = true;
	u_reduce_diff.location = glGetUniformLocationARB( sh_dist_d_reduce.program, "diff" );
	u_reduce_oldchunkwidth.location = glGetUniformLocationARB( sh_dist_d_reduce.program, "oldchunkwidth" );
	u_reduce_newchunkwidth.location = glGetUniformLocationARB( sh_dist_d_reduce.program, "newchunkwidth" );
	//u_reduce_preselec.location = glGetUniformLocationARB( sh_dist_d_reduce.program, "preselec" );
	//u_reduce_postselec.location = glGetUniformLocationARB( sh_dist_d_reduce.program, "postselec" );

	// setup sh_dist_d_copy
	sh_dist_d_copy.program = glCreateProgramObjectARB();
	sh_dist_d_copy.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_dist_d_copy.program, sh_dist_d_copy.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_dist_copy.glsl");
	string dist_copy_string = readShaderSource(shader_path);
	const GLcharARB *dist_copy_source = dist_copy_string.c_str();
	glShaderSourceARB(sh_dist_d_copy.shader, 1, &dist_copy_source, NULL);
	glCompileShaderARB(sh_dist_d_copy.shader);
	glLinkProgramARB(sh_dist_d_copy.program);
	printInfoLog( "sh_dist_d_copy", sh_dist_d_copy.shader );
	u_copy_diff.isFloat = false;
	u_copy_diff.location = glGetUniformLocationARB( sh_dist_d_copy.program, "diff" );

	// setup sh_dot_copy
	sh_dot_copy.program = glCreateProgramObjectARB();
	sh_dot_copy.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_dot_copy.program, sh_dot_copy.shader);
	sprintf(shader_path, "%s%s", g_shader_path, "sh_dot_copy.glsl");
	string dot_copy_string = readShaderSource(shader_path);
	const GLcharARB *dot_copy_source = dot_copy_string.c_str();
	glShaderSourceARB(sh_dot_copy.shader, 1, &dot_copy_source, NULL);
	glCompileShaderARB(sh_dot_copy.shader);
	glLinkProgramARB(sh_dot_copy.program);
	printInfoLog( "sh_dot_copy", sh_dot_copy.shader );
	u_dot_copy.isFloat = false;
	u_dot_copy.location = glGetUniformLocationARB( sh_dot_copy.program, "diff" );

	
	//setup sh_dist_g
	sh_dist_g.program = glCreateProgramObjectARB();
	sh_dist_g.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_dist_g.program, sh_dist_g.shader);
	string dist_g_string = "\
								   \nuniform sampler2DRect embed;\
								   \nuniform sampler2DRect idx;\
								   \nuniform float embedwidth;\
								   \nuniform float distwidth;\
								   \nuniform float binsperpt;\
								   void main()\n\
								   {\n\
								   vec2 self = floor( gl_TexCoord[0].xy );\n\
								   vec2 owncoord = vec2( floor( self.x / distwidth ), self.y);\n\
								   vec4 color = vec4(0.0,0.0,0.0,0.0);\n\
								   vec4 indices = texture2DRect(idx, self);\n\
								   vec4 indexx  = mod( indices, embedwidth )*binsperpt;\n\
								   vec4 indexy  = floor( indices / embedwidth );\n";
	for( j = 0; j < n_embed_dims/4 + (n_embed_dims%4?1:0); j++ ) {
		dist_g_string += "vec4 mypt";
		sprintf(number_str, "%d", j);
		dist_g_string += number_str;
		dist_g_string += " = texture2DRect(embed, owncoord+";
		sprintf(number_str, "%d.0", j);
		dist_g_string += number_str;
		dist_g_string += ");\n";
	}
	for( i = 0; i < 4; i++ ) {
		switch(i) {
			case 0:
				sprintf(letter_str, "x");
				break;
			case 1:
				sprintf(letter_str, "y");
				break;
			case 2:
				sprintf(letter_str, "z");
				break;
			case 3:
				sprintf(letter_str, "w");
				break;
		}
		for( j = 0; j < n_embed_dims/4 + (n_embed_dims%4?1:0); j++ ) {
			dist_g_string += "vec4 pt";
			sprintf(number_str, "%d_%d", i, j);
			dist_g_string += number_str;
			sprintf(number_str, "%d.0", j);
			dist_g_string += " = texture2DRect(embed, vec2(indexx.";
			dist_g_string += letter_str;
			dist_g_string += "+";
			dist_g_string += number_str;
			dist_g_string += ",indexy.";
			dist_g_string += letter_str;
			dist_g_string += "+";
			dist_g_string += number_str;
			dist_g_string += "));\n\
			color.";
			dist_g_string += letter_str;
			dist_g_string += " = color.";
			dist_g_string += letter_str;
			dist_g_string += " + distance(mypt";
			sprintf(number_str, "%d", j);
			dist_g_string += number_str;
			dist_g_string += ",pt";
			sprintf(number_str, "%d_%d", i, j);
			dist_g_string += number_str;
			dist_g_string += ");";

		}
	}
	dist_g_string += "gl_FragColor = color;\n}";
	//printf("\n\n%s\n\n", dist_g_string.c_str());
	//exit( 0 );
	const GLcharARB *dist_g_source = dist_g_string.c_str();
	glShaderSourceARB(sh_dist_g.shader, 1, &dist_g_source, NULL);
	glCompileShaderARB(sh_dist_g.shader);
	glLinkProgramARB(sh_dist_g.program);
	printInfoLog( "sh_dist_g", sh_dist_g.shader );
	u_g_embed.isFloat = false;
	u_g_idx.isFloat = false;
	u_g_embedwidth.isFloat = true;	
	u_g_distwidth.isFloat = true;
	u_g_binsperpt.isFloat = true;
	u_g_embed.location = glGetUniformLocationARB( sh_dist_g.program, "embed" );
	u_g_idx.location = glGetUniformLocationARB( sh_dist_g.program, "idx" );
	u_g_embedwidth.location = glGetUniformLocationARB( sh_dist_g.program, "embedwidth" );
	u_g_distwidth.location = glGetUniformLocationARB( sh_dist_g.program, "distwidth" );
	u_g_binsperpt.location = glGetUniformLocationARB( sh_dist_g.program, "binsperpt" );
	u_g_distwidth.valuef = (float)(((int) ceil( ((double)n_vmax)/4.0 ) + (int) ceil( ((double)n_smax)/4.0 )));
	u_g_binsperpt.valuef = (float) ceil( (double)n_embed_dims / 4.0 );
}

void interpolate_points( Texture *texture, Tier *tier ) {

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0,texture->tier_width[tier->level],0.0,texture->tier_height[tier->level]);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0,0,texture->tier_width[tier->level],texture->tier_height[tier->level]);

	// first copy over the old pixels

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glReadBuffer (attachmentpoints[texture->attach_idx?0:1]);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);
	glCopyTexSubImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, 0, 0, texture->tier_width[tier->level+1], texture->tier_height[tier->level+1]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glUseProgramObjectARB( sh_interpolate.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_idx->texture_name[tier->t_idx->attach_idx]);
	glUniform1iARB(u_interpolate_idx.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_embed->texture_name[tier->t_embed->attach_idx?0:1]);
	glUniform1iARB(u_interpolate_embed.location, 2); 
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);
	glUniform1iARB(u_interpolate_d.location, 3); 
	
	glUniform1fARB(u_interpolate_embedwidth.location, (float)(tier->t_embed->width / (int) u_interpolate_binsperpt.valuef) );
	glUniform1fARB(u_interpolate_distwidth.location, u_interpolate_distwidth.valuef );
	glUniform1fARB(u_interpolate_binsperpt.location, u_interpolate_binsperpt.valuef );

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	// and render quad
	//glBegin(GL_QUADS);
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

void calc_dist_g( Texture *texture, Tier *tier ) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture->tier_width[tier->level],0.0,texture->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture->tier_width[tier->level],texture->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glUseProgramObjectARB( sh_dist_g.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_idx->texture_name[tier->t_idx->attach_idx]);
	glUniform1iARB(u_g_idx.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_embed->texture_name[tier->t_embed->attach_idx]);
	glUniform1iARB(u_g_embed.location, 2); 
	
	glUniform1fARB(u_g_embedwidth.location, (float)(tier->t_embed->width / (int) u_g_binsperpt.valuef) );
	glUniform1fARB(u_g_distwidth.location, u_g_distwidth.valuef );
	glUniform1fARB(u_g_binsperpt.location, u_g_binsperpt.valuef );

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

}

void calc_diff( Tier *tier ) {

	//clock_t time1,time2;

	// calculate the initial differences

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,tier->t_diff->tier_width[tier->level],0.0,tier->t_diff->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,tier->t_diff->tier_width[tier->level],tier->t_diff->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_diff->frame_buffer);
	glUseProgramObjectARB( sh_dist_d_diff.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_diff->texture_name[tier->t_diff->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_idx->texture_name[tier->t_idx->attach_idx]);
	glUniform1iARB(u_diff_idx.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_pts->texture_name[tier->t_pts->attach_idx]);
	glUniform1iARB(u_diff_pts.location, 2); 
	
	//glUniform1fARB(u_diff_pointwidth.location, (float)(tier->t_pts->tier_width[tier->level] / (int) u_diff_binsperpt.valuef) );
	glUniform1fARB(u_diff_pointwidth.location, (float)(tier->t_pts->tier_width[0] / (int) u_diff_binsperpt.valuef) );
	glUniform1fARB(u_diff_diffwidth.location, u_diff_diffwidth.valuef );
	glUniform1fARB(u_diff_idxwidth.location, u_diff_idxwidth.valuef );
	glUniform1fARB(u_diff_chunkwidth.location, u_diff_chunkwidth.valuef );
	glUniform1fARB(u_diff_binsperpt.location, u_diff_binsperpt.valuef );

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[tier->t_diff->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	//time1 = clock();
	// and render quad
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); 
		glVertex2f(0.0, 0.0);
		glTexCoord2f(tier->t_diff->tier_width[tier->level], 0.0); 
		glVertex2f(tier->t_diff->tier_width[tier->level], 0.0);
		glTexCoord2f(tier->t_diff->tier_width[tier->level], tier->t_diff->tier_height[tier->level]); 
		glVertex2f(tier->t_diff->tier_width[tier->level], tier->t_diff->tier_height[tier->level]);
		glTexCoord2f(0.0, tier->t_diff->tier_height[tier->level]); 
		glVertex2f(0.0, tier->t_diff->tier_height[tier->level]);
	glEnd();



	//glFinish();
	//time2 = clock();
	//printf("\t \t A done in %d milliseconds.\n", (time2 - time1));

	// reduce to bottom of pyramid	
	int width = tier->t_diff->tier_width[tier->level];
	int chunkwidth = (int) ceil( ( (double) n_dims ) / 4.0 );
	int newchunkwidth = 0;
	int foo = 0;
	//printf("tier->level = %d and width = %d and tier->t_d->tier_width[tier->level] == %d\n", tier->level, width, tier->t_d->tier_width[tier->level]);
	while( width != tier->t_d->tier_width[tier->level] ) {

		//printf("get we here?");

		foo++;
		// adjust pyramid parameters

		tier->t_diff->attach_idx = tier->t_diff->attach_idx?0:1;
		newchunkwidth = (int) ceil( ((double)chunkwidth) / 4.0 );
		width = newchunkwidth * tier->t_d->tier_width[tier->level];

		// perform reduction sum

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0,width,0.0,tier->t_diff->tier_height[tier->level]);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glViewport(0,0,width,tier->t_diff->tier_height[tier->level]);

		// bind data structures and active texture units
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_diff->frame_buffer);
		glUseProgramObjectARB( sh_dist_d_reduce.program );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_diff->texture_name[tier->t_diff->attach_idx]);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_diff->texture_name[tier->t_diff->attach_idx?0:1]);
		glUniform1iARB(u_reduce_diff.location, 1); 
		
		glUniform1fARB(u_reduce_newchunkwidth.location, (float) newchunkwidth );
		glUniform1fARB(u_reduce_oldchunkwidth.location, (float) chunkwidth );
		//u_reduce_preselec.location = glGetUniformLocationARB( sh_dist_d_reduce.program, "preselec" );
		//u_reduce_postselec.location = glGetUniformLocationARB( sh_dist_d_reduce.program, "postselec" );
		//glUniform4fARB(u_reduce_postselec.location, 1.0f, 2.0f, 3.0f, 4.0f );
		//glUniform4fARB(u_reduce_preselec.location, 0.0f, 1.0f, 2.0f, 3.0f );
		//float f[] = {0.0f, 1.0f, 2.0f, 3.0f};
		//glUniform4fvARB(u_reduce_preselec.location, 1, f );

		// make quad filled to hit every pixel/texel
		glDrawBuffer (attachmentpoints[tier->t_diff->attach_idx]);
		glPolygonMode(GL_FRONT,GL_FILL);

		//time1 = clock();
		// and render quad
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0); 
			glVertex2f(0.0, 0.0);
			glTexCoord2f(width, 0.0); 
			glVertex2f(width, 0.0);
			glTexCoord2f(width, tier->t_diff->tier_height[tier->level]); 
			glVertex2f(width, tier->t_diff->tier_height[tier->level]);
			glTexCoord2f(0.0, tier->t_diff->tier_height[tier->level]); 
			glVertex2f(0.0, tier->t_diff->tier_height[tier->level]);
		glEnd();
		//glFinish();
		//time2 = clock();
		//printf("\t \t B done in %d milliseconds.\n", (time2 - time1));

		chunkwidth = newchunkwidth;
	}

	// reduce/(copy sqrt) to distance texture

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,tier->t_d->tier_width[tier->level],0.0,tier->t_d->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,tier->t_d->tier_width[tier->level],tier->t_d->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_d->frame_buffer);
	glUseProgramObjectARB( sh_dist_d_copy.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_diff->texture_name[tier->t_diff->attach_idx]);
	glUniform1iARB(u_copy_diff.location, 1); 

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[tier->t_d->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	//time1 = clock();
	// and render quad
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); 
		glVertex2f(0.0, 0.0);
		glTexCoord2f(tier->t_d->tier_width[tier->level], 0.0); 
		glVertex2f(tier->t_d->tier_width[tier->level], 0.0);
		glTexCoord2f(tier->t_d->tier_width[tier->level], tier->t_d->tier_height[tier->level]); 
		glVertex2f(tier->t_d->tier_width[tier->level], tier->t_d->tier_height[tier->level]);
		glTexCoord2f(0.0, tier->t_d->tier_height[tier->level]); 
		glVertex2f(0.0, tier->t_d->tier_height[tier->level]);
	glEnd();
	//glFinish();
	//time2 = clock();
	//printf("\t \t C done in %d milliseconds.\n", (time2 - time1));

}

/*
	Compute the cosine distance between two sparse vectors
*/
void calc_dot_product( Tier *tier ) {

	// calculate the initial products

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,tier->t_diff->tier_width[tier->level],0.0,tier->t_diff->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,tier->t_diff->tier_width[tier->level],tier->t_diff->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_diff->frame_buffer);
	glUseProgramObjectARB( sh_dot_term.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_diff->texture_name[tier->t_diff->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_idx->texture_name[tier->t_idx->attach_idx]);
	glUniform1iARB(u_dot_term_idx.location, 1); 
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_pts->texture_name[tier->t_pts->attach_idx]);
	glUniform1iARB(u_dot_term_pts.location, 2); 
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_vec_idx->texture_name[tier->t_vec_idx->attach_idx]);
	glUniform1iARB(u_dot_term_vec_idx.location, 3); 
	
	glUniform1fARB(u_dot_term_pointwidth.location, (float)(tier->t_pts->tier_width[0] / (int) u_diff_binsperpt.valuef) );
	glUniform1fARB(u_dot_term_diffwidth.location, u_diff_diffwidth.valuef );
	glUniform1fARB(u_dot_term_idxwidth.location, u_diff_idxwidth.valuef );
	glUniform1fARB(u_dot_term_chunkwidth.location, u_diff_chunkwidth.valuef );
	glUniform1fARB(u_dot_term_binsperpt.location, u_diff_binsperpt.valuef );
	glUniform1fARB(u_dot_term_maxnz.location, n_max_sparse_entries - 1 );
	glUniform1fARB(u_dot_term_logmaxnz.location, (double) ceil( log( (double)n_max_sparse_entries )/log(2.0) ) );
	glUniform1fARB(u_dot_term_halfN.location, pow( 2.0, (double) ceil( log( (double)n_max_sparse_entries )/log(2.0) ) - 1.0  ) );

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[tier->t_diff->attach_idx]);

	glPolygonMode(GL_FRONT,GL_FILL);

	// and render quad
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); 
		glVertex2f(0.0, 0.0);
		glTexCoord2f(tier->t_diff->tier_width[tier->level], 0.0); 
		glVertex2f(tier->t_diff->tier_width[tier->level], 0.0);
		glTexCoord2f(tier->t_diff->tier_width[tier->level], tier->t_diff->tier_height[tier->level]); 
		glVertex2f(tier->t_diff->tier_width[tier->level], tier->t_diff->tier_height[tier->level]);
		glTexCoord2f(0.0, tier->t_diff->tier_height[tier->level]); 
		glVertex2f(0.0, tier->t_diff->tier_height[tier->level]);
	glEnd();

	//outputTexture( tier->t_diff, "diff1.txt");
	//exit( 0 );

	//time2 = clock();
	//printf("\t \t A done in %d milliseconds.\n", (time2 - time1));

	// reduce to bottom of pyramid	
	int width = tier->t_diff->tier_width[tier->level];
	int chunkwidth = (int) ceil( ( (double) n_dims ) / 4.0 );
	int newchunkwidth = 0;
	int foo = 0;
	while( width != tier->t_d->tier_width[tier->level] ) {

		foo++;
		// adjust pyramid parameters

		tier->t_diff->attach_idx = tier->t_diff->attach_idx?0:1;
		newchunkwidth = (int) ceil( ((double)chunkwidth) / 4.0 );
		width = newchunkwidth * tier->t_d->tier_width[tier->level];

		// perform reduction sum

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0,width,0.0,tier->t_diff->tier_height[tier->level]);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glViewport(0,0,width,tier->t_diff->tier_height[tier->level]);

		// bind data structures and active texture units
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_diff->frame_buffer);
		glUseProgramObjectARB( sh_dist_d_reduce.program );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_diff->texture_name[tier->t_diff->attach_idx]);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_diff->texture_name[tier->t_diff->attach_idx?0:1]);
		glUniform1iARB(u_reduce_diff.location, 1); 
		
		glUniform1fARB(u_reduce_newchunkwidth.location, (float) newchunkwidth );
		glUniform1fARB(u_reduce_oldchunkwidth.location, (float) chunkwidth );
		//u_reduce_preselec.location = glGetUniformLocationARB( sh_dist_d_reduce.program, "preselec" );
		//u_reduce_postselec.location = glGetUniformLocationARB( sh_dist_d_reduce.program, "postselec" );
		//glUniform4fARB(u_reduce_postselec.location, 1.0f, 2.0f, 3.0f, 4.0f );
		//glUniform4fARB(u_reduce_preselec.location, 0.0f, 1.0f, 2.0f, 3.0f );
		//float f[] = {0.0f, 1.0f, 2.0f, 3.0f};
		//glUniform4fvARB(u_reduce_preselec.location, 1, f );

		// make quad filled to hit every pixel/texel
		glDrawBuffer (attachmentpoints[tier->t_diff->attach_idx]);
		glPolygonMode(GL_FRONT,GL_FILL);

		//time1 = clock();
		// and render quad
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0); 
			glVertex2f(0.0, 0.0);
			glTexCoord2f(width, 0.0); 
			glVertex2f(width, 0.0);
			glTexCoord2f(width, tier->t_diff->tier_height[tier->level]); 
			glVertex2f(width, tier->t_diff->tier_height[tier->level]);
			glTexCoord2f(0.0, tier->t_diff->tier_height[tier->level]); 
			glVertex2f(0.0, tier->t_diff->tier_height[tier->level]);
		glEnd();
		//time2 = clock();
		//printf("\t \t B done in %d milliseconds.\n", (time2 - time1));

		chunkwidth = newchunkwidth;
	}

	// reduce/(copy sqrt) to distance texture

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,tier->t_d->tier_width[tier->level],0.0,tier->t_d->tier_height[tier->level]);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,tier->t_d->tier_width[tier->level],tier->t_d->tier_height[tier->level]);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_d->frame_buffer);
	glUseProgramObjectARB( sh_dot_copy.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_diff->texture_name[tier->t_diff->attach_idx]);
	glUniform1iARB(u_dot_copy.location, 1); 

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[tier->t_d->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	//time1 = clock();
	// and render quad
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); 
		glVertex2f(0.0, 0.0);
		glTexCoord2f(tier->t_d->tier_width[tier->level], 0.0); 
		glVertex2f(tier->t_d->tier_width[tier->level], 0.0);
		glTexCoord2f(tier->t_d->tier_width[tier->level], tier->t_d->tier_height[tier->level]); 
		glVertex2f(tier->t_d->tier_width[tier->level], tier->t_d->tier_height[tier->level]);
		glTexCoord2f(0.0, tier->t_d->tier_height[tier->level]); 
		glVertex2f(0.0, tier->t_d->tier_height[tier->level]);
	glEnd();
	//time2 = clock();
	//printf("\t \t C done in %d milliseconds.\n", (time2 - time1));
}

/*
	Copy values from the distance lookup table to the appropriate index for a point
*/
void lookup_dist(  Tier *tier, bool b_fix  ) {
	static int spacer = 0;

	// have we stuffed the entire distance matrix into the pts texture?
	if( ! b_use_paging ) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0,tier->t_d->tier_width[tier->level],0.0,tier->t_d->tier_height[tier->level]);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glViewport(0,0,tier->t_d->tier_width[tier->level],tier->t_d->tier_height[tier->level]);

		// bind data structures and active texture units
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_d->frame_buffer);
		glUseProgramObjectARB( sh_dist_d_lookup.program );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_pts->texture_name[tier->t_pts->attach_idx]);
		glUniform1iARB(u_lookup_pts.location, 1); 
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_idx->texture_name[tier->t_idx->attach_idx]);
		glUniform1iARB(u_lookup_idx.location, 2); 
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_reference->texture_name[tier->t_reference->attach_idx]);
		//glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_perm->texture_name[tier->t_perm->attach_idx?0:1]);
		glUniform1iARB(u_lookup_perm.location, 3); 
		
		glUniform1fARB(u_lookup_refwidth.location, (float)tier->t_reference->width );
		glUniform1fARB(u_lookup_pointcount.location, (float)tier->n_points );
		glUniform1fARB(u_lookup_diffwidth.location, (float)(tier->t_d->width / tier->t_perm->width ) );
		glUniform1fARB(u_lookup_lookupheight.location, tier->t_pts->height );
		if( tier->n_points%4 ) {
			glUniform1fARB(u_lookup_lookupwidth.location, tier->t_pts->width*4 - (4 - tier->n_points%4) );
		}
		else
			glUniform1fARB(u_lookup_lookupwidth.location, tier->t_pts->width*4 );

		// make quad filled to hit every pixel/texel
		glDrawBuffer (attachmentpoints[tier->t_d->attach_idx]);
		glPolygonMode(GL_FRONT,GL_FILL);

		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0); 
			glVertex2f(0.0, 0.0);
			glTexCoord2f(tier->t_d->tier_width[tier->level], 0.0); 
			glVertex2f(tier->t_d->tier_width[tier->level], 0.0);
			glTexCoord2f(tier->t_d->tier_width[tier->level], tier->t_d->tier_height[tier->level]); 
			glVertex2f(tier->t_d->tier_width[tier->level], tier->t_d->tier_height[tier->level]);
			glTexCoord2f(0.0, tier->t_d->tier_height[tier->level]); 
			glVertex2f(0.0, tier->t_d->tier_height[tier->level]);
		glEnd();

		//outputTexture( tier->t_d, "d.txt" );
		//outputTexture( tier->t_idx, "idx.txt" );
		//exit( 0 );
	}
	else {  // are we paging distance from/memory

		// our first iteration requires special processing
		if( g_n_framecount == 0 ) {

			// grab pages from the feeder
			feeder_get_page( tier, 0, b_fix );
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_pts->texture_name[ tier->t_pts->attach_idx ]);
			glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_pts->width,tier->t_pts->height,
				GL_RGBA,GL_FLOAT,g_feeder.page);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0.0,tier->t_d->tier_width[tier->level],0.0,tier->t_d->tier_height[tier->level]);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glViewport(0,0,tier->t_d->tier_width[tier->level],tier->t_d->tier_height[tier->level]);

			// bind data structures and active texture units
			
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_d->frame_buffer);
			glUseProgramObjectARB( sh_page.program );
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_pts->texture_name[tier->t_pts->attach_idx]);
			glUniform1iARB(u_page_page.location, 1); 
			glUniform1fARB(u_page_dwidth.location, (float)tier->t_d->width/tier->t_embed->width );
			glUniform1fARB(u_page_dspan.location, (float)tier->t_embed->tier_width[tier->level] );
			glUniform1fARB(u_page_pspan.location, (float)tier->t_pts->width );
			glUniform1fARB(u_page_iterationoffs.location, 0.f );

			// make quad filled to hit every pixel/texel
			glDrawBuffer (attachmentpoints[tier->t_d->attach_idx]);
			glPolygonMode(GL_FRONT,GL_FILL);

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

			if( g_n_paging_iterations == 1 ) {
				spacer++;
				feeder_get_page( tier, 1, b_fix );
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_pts->texture_name[ tier->t_pts->attach_idx ]);
				glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_pts->width,tier->t_pts->height,
					GL_RGBA,GL_FLOAT,g_feeder.page);

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
		}

		// have we run out of pages?
		else {

			if( !tier->b_update_s ) {

				if( !( (g_n_framecount+spacer) % g_n_paging_iterations) ) {

					// grab pages from the feeder
					feeder_get_page( tier, (g_n_framecount + spacer)%tier->n_points, b_fix );

					// stick them on the gpu
					glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_pts->texture_name[ tier->t_pts->attach_idx ]);
					glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_pts->width,tier->t_pts->height,
						GL_RGBA,GL_FLOAT,g_feeder.page);
				}

				// run the paging lookup shader
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				gluOrtho2D(0.0,tier->t_d->tier_width[tier->level],0.0,tier->t_d->tier_height[tier->level]);
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				glViewport(0,0,tier->t_d->tier_width[tier->level],tier->t_d->tier_height[tier->level]);

				// bind data structures and active texture units
				
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_d->frame_buffer);
				glUseProgramObjectARB( sh_page.program );
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_pts->texture_name[tier->t_pts->attach_idx]);
				glUniform1iARB(u_page_page.location, 1); 
				glUniform1fARB(u_page_dwidth.location, (float)tier->t_d->width/tier->t_embed->width );
				glUniform1fARB(u_page_dspan.location, (float)tier->t_embed->tier_width[tier->level] );
				glUniform1fARB(u_page_pspan.location, (float)tier->t_pts->width );
				glUniform1fARB(u_page_iterationoffs.location, ((g_n_framecount+spacer) % g_n_paging_iterations )*tier->n_points );

				// make quad filled to hit every pixel/texel
				glDrawBuffer (attachmentpoints[tier->t_d->attach_idx]);
				glPolygonMode(GL_FRONT,GL_FILL);

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

				spacer++;
			}
			if( !( (g_n_framecount+spacer) % g_n_paging_iterations) ) {

				// grab pages from the feeder
				feeder_get_page( tier, (g_n_framecount + spacer)%tier->n_points, b_fix );

				// stick them on the gpu
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tier->t_pts->texture_name[ tier->t_pts->attach_idx ]);
				glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,0,0,0,tier->t_pts->width,tier->t_pts->height,
					GL_RGBA,GL_FLOAT,g_feeder.page);
			}

			// run the paging lookup shader
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0.0,tier->t_d->tier_width[tier->level],0.0,tier->t_d->tier_height[tier->level]);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glViewport(0,0,tier->t_d->tier_width[tier->level],tier->t_d->tier_height[tier->level]);

			// bind data structures and active texture units
			
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,tier->t_d->frame_buffer);
			glUseProgramObjectARB( sh_page.program );
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_d->texture_name[tier->t_d->attach_idx]);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,tier->t_pts->texture_name[tier->t_pts->attach_idx]);
			glUniform1iARB(u_page_page.location, 1); 
			glUniform1fARB(u_page_dwidth.location, (float)tier->t_d->width/tier->t_embed->width );
			glUniform1fARB(u_page_dspan.location, (float)tier->t_embed->tier_width[tier->level] );
			glUniform1fARB(u_page_pspan.location, (float)tier->t_pts->width );
			glUniform1fARB(u_page_iterationoffs.location, ((g_n_framecount+spacer) % g_n_paging_iterations )*tier->n_points );

			// make quad filled to hit every pixel/texel
			glDrawBuffer (attachmentpoints[tier->t_d->attach_idx]);
			glPolygonMode(GL_FRONT,GL_FILL);

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
			//outputTexture( tier->t_pts, "page.txt" );
			//outputTexture( tier->t_d, "d_page.txt" );
			//outputTexture( tier->t_idx, "idx.txt" );
			//exit( 0 );
		}
	}
}
