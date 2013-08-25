#include <iostream>
//#include <tchar.h> // Breaks in Linux
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <cstring>
#include <stdio.h>
#include "fbo.h"

/**
	Frame buffer attachment points
*/
GLenum attachmentpoints[] = { GL_COLOR_ATTACHMENT0_EXT, 
                              GL_COLOR_ATTACHMENT1_EXT 
                            };

/*
	Check the status of the frame buffer.  Useful for debugging.
*/
bool checkFramebufferStatus() {
    GLenum status;
    status=(GLenum)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            return true;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            printf("Framebuffer incomplete,incomplete attachment\n");
            return false;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            printf("Unsupported framebuffer format\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            printf("Framebuffer incomplete,missing attachment\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            printf("Framebuffer incomplete,attached images must have same dimensions\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
             printf("Framebuffer incomplete,attached images must have same format\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            printf("Framebuffer incomplete,missing draw buffer\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            printf("Framebuffer incomplete,missing read buffer\n");
            return false;
    }
	return false;
}

/*
	Create a framebuffer and attach an appropriately sized texture
*/
void buildFBOTexture ( Texture *texture ) {

	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluOrtho2D(0.0,texture->width,0.0,texture->height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glViewport(0,0,texture->width,texture->height);
	
	glGenFramebuffersEXT(1,&texture->frame_buffer); 
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,texture->frame_buffer);
	glGenTextures (texture->attachments, texture->texture_name);
	
	for( int i = 0; i < texture->attachments; i++ ) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[i]);    
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, 
						GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, 
						GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, 
						GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, 
						GL_TEXTURE_WRAP_T, GL_CLAMP);    
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_FLOAT_RGBA32_NV,
			texture->width,texture->height,0,GL_RGBA,GL_FLOAT,0);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, 
								attachmentpoints[i], 
								GL_TEXTURE_RECTANGLE_ARB,texture->texture_name[i],0);
		if( ! checkFramebufferStatus() ) {
			exit( 0 );
		}
	}
}
