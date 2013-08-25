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

extern float *embed_bounds;
extern int n_points;
extern Pyramid pyr_center;
extern GLenum attachmentpoints[];

Shader sh_center;		// computes center of embedded points
Shader sh_app_center;		// computes center of embedded points

Uniform u_center_parent;
Uniform u_center_height;
Uniform u_center_width;

Uniform u_center_old;
Uniform u_center_div;

void setup_sh_center( ) {

	//setup sh_center
	sh_center.program = glCreateProgramObjectARB();
	sh_center.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_center.program, sh_center.shader);
	string center_string = "\
								   uniform sampler2DRect parent;\n\
								   uniform float parent_width;\n\
								   uniform float parent_height;\n\
								   void main()\n\
								   {\n\
								   vec2 self = floor( gl_TexCoord[0].xy )*2.0;\n\
								   vec4 zero = vec4(0.0,0.0,0.0,0.0);\n\
								   vec4 ne = texture2DRect(parent,vec2( self.x, self.y));\n\
								   vec4 se = (self.y + 1.0)>parent_height?zero:texture2DRect(parent,vec2( self.x, self.y + 1.0));\n\
								   vec4 nw = (self.x  + 1.0)>parent_width?zero:texture2DRect(parent,vec2( self.x + 1.0, self.y));\n\
								   vec4 sw = (self.y + 1.0)>parent_height?zero:texture2DRect(parent,vec2( self.x + 1.0, self.y + 1.0));\n\
								   sw = (self.x + 1.0)>parent_width?zero:sw;\n\
								   gl_FragColor = ne+se+nw+sw;\n\
								   }";
	const GLcharARB *center_source = center_string.c_str();
	glShaderSourceARB(sh_center.shader, 1, &center_source, NULL);
	glCompileShaderARB(sh_center.shader);
	glLinkProgramARB(sh_center.program);
	printInfoLog( "sh_center", sh_center.shader );
	printInfoLog( "sh_center", sh_center.program );
	u_center_parent.isFloat = false;
	u_center_height.isFloat = true;	
	u_center_width.isFloat = true;
	u_center_parent.location = glGetUniformLocationARB( sh_center.program, "parent" );
	u_center_height.location = glGetUniformLocationARB( sh_center.program, "parent_width" );
	u_center_width.location = glGetUniformLocationARB( sh_center.program, "parent_height" );
	
	//setup sh_center
	sh_app_center.program = glCreateProgramObjectARB();
	sh_app_center.shader  = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glAttachObjectARB (sh_app_center.program, sh_app_center.shader);
	string app_center_string = "\
								   uniform sampler2DRect old;\n\
								   uniform vec4 div;\n\
								   void main()\n\
								   {\n\
								   vec2 self = floor( gl_TexCoord[0].xy );\n\
								   vec4 temp = texture2DRect(old,self);\n\
								   gl_FragColor = temp-div;\n\
								   }";
	const GLcharARB *app_center_source = app_center_string.c_str();
	glShaderSourceARB(sh_app_center.shader, 1, &app_center_source, NULL);
	glCompileShaderARB(sh_app_center.shader);
	glLinkProgramARB(sh_app_center.program);
	printInfoLog( "sh_app_center", sh_app_center.shader );
	printInfoLog( "sh_app_center", sh_app_center.program );
	u_center_old.isFloat = false;
	u_center_div.isFloat = true;
	u_center_old.location = glGetUniformLocationARB( sh_app_center.program, "old" );
	u_center_div.location = glGetUniformLocationARB( sh_app_center.program, "div" );
}

void center_points( Texture *texture ) {
	int i = 0;
	// perform reduction
	for( i = 0; i < pyr_center.height; i++ ) {

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0,pyr_center.t_level[i].width,0.0,pyr_center.t_level[i].height);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glViewport(0,0,pyr_center.t_level[i].width,pyr_center.t_level[i].height);

		// bind data structures and active texture units
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,pyr_center.t_level[i].frame_buffer);
		glUseProgramObjectARB( sh_center.program );
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,pyr_center.t_level[i].texture_name[pyr_center.t_level[i].attach_idx]);

		if( i == 0 ) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx?0:1]);
			glUniform1iARB(u_center_parent.location, 1); 
			
			glUniform1fARB(u_center_width.location, texture->width );
			glUniform1fARB(u_center_height.location, texture->height );
		}
		else {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_RECTANGLE_ARB,pyr_center.t_level[i-1].texture_name[pyr_center.t_level[i-1].attach_idx]);
			glUniform1iARB(u_center_parent.location, 1); 
			
			glUniform1fARB(u_center_width.location, pyr_center.t_level[i-1].width );
			glUniform1fARB(u_center_height.location, pyr_center.t_level[i-1].height );
		}

		// make quad filled to hit every pixel/texel
		glDrawBuffer (attachmentpoints[pyr_center.t_level[i].attach_idx]);
		glPolygonMode(GL_FRONT,GL_FILL);

		// and render quad
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0); 
			glVertex2f(0.0, 0.0);
			glTexCoord2f(pyr_center.t_level[i].width, 0.0); 
			glVertex2f(pyr_center.t_level[i].width, 0.0);
			glTexCoord2f(pyr_center.t_level[i].width, pyr_center.t_level[i].height); 
			glVertex2f(pyr_center.t_level[i].width, pyr_center.t_level[i].height);
			glTexCoord2f(0.0, pyr_center.t_level[i].height); 
			glVertex2f(0.0, pyr_center.t_level[i].height);
		glEnd();

		//if( i == 1) {
		//	outputTexture( &pyr_center.t_level[i], "pyr.txt");
		//	exit( 0 );
		//}
	}

	// grab the center value

	float center[4];

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,pyr_center.t_level[i-1].frame_buffer);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,pyr_center.t_level[i-1].texture_name[pyr_center.t_level[i-1].attach_idx]);
	glReadBuffer(attachmentpoints[pyr_center.t_level[i-1].attach_idx]);
	glReadPixels(0, 0, pyr_center.t_level[i-1].width, pyr_center.t_level[i-1].height,GL_RGBA,GL_FLOAT,center);

	center[0] /= (float) n_points;
	center[1] /= (float) n_points;
	center[2] /= (float) n_points;
	center[3] /= (float) n_points;

	printf("center = %f %f\n", center[0], center[1]);


	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0,texture->width,0.0,texture->height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0,0,texture->width,texture->height);

	// bind data structures and active texture units
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glUseProgramObjectARB( sh_app_center.program );
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[texture->attach_idx?0:1]);
	glUniform1iARB(u_center_old.location, 1); 	
	glUniform4fvARB(u_center_div.location, 1, center );

	// make quad filled to hit every pixel/texel
	glDrawBuffer (attachmentpoints[texture->attach_idx]);
	glPolygonMode(GL_FRONT,GL_FILL);

	// and render quad
	glBegin(GL_QUADS);
		glTexCoord2f(embed_bounds[0], embed_bounds[1]); 
		glVertex2f(embed_bounds[0], embed_bounds[1]);
		glTexCoord2f(embed_bounds[2], embed_bounds[3]); 
		glVertex2f(embed_bounds[2], embed_bounds[3]);
		glTexCoord2f(embed_bounds[4], embed_bounds[5]); 
		glVertex2f(embed_bounds[4], embed_bounds[5]);
		glTexCoord2f(embed_bounds[6], embed_bounds[7]); 
		glVertex2f(embed_bounds[6], embed_bounds[7]);
		if(n_points % texture->width) {
			glTexCoord2f(embed_bounds[8], embed_bounds[9]); 
			glVertex2f(embed_bounds[8], embed_bounds[9]);
			glTexCoord2f(embed_bounds[10], embed_bounds[11]); 
			glVertex2f(embed_bounds[10], embed_bounds[11]);
			glTexCoord2f(embed_bounds[12], embed_bounds[13]); 
			glVertex2f(embed_bounds[12], embed_bounds[13]);
			glTexCoord2f(embed_bounds[14], embed_bounds[15]); 
			glVertex2f(embed_bounds[14], embed_bounds[15]);
		}
	glEnd();

}
