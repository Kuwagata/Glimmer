#include <iostream>
//#include <tchar.h> //Breaks in Linux
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <cstring>
#include <stdio.h>
#include "shader.h"

extern bool b_output_debug;

char *readShaderSource( const char *filename ) {

	FILE *fp;
	if( (fp = fopen( filename, "r" )) == NULL ) {
		printf( " ERROR: cannot open shader source file \"%s\".  Quitting.\n", filename );
	}

	// get the size of the file

	int stringsize = 0;
	while( fgetc( fp ) != EOF )
		stringsize++;
	fclose( fp );

	// allocate memory
	char *source = (char *) malloc( sizeof( char ) * (stringsize + 1) );

	// reopen and read in the file
	if( (fp = fopen( filename, "r" )) == NULL ) {
		printf( " ERROR: cannot open shader source file \"%s\".  Quitting.\n", filename );
	}
	int i = 0;
	for( i = 0; i < stringsize; i++ ) {
		source[i]=fgetc(fp);
	}
	source[i]='\0';

	return source;

}

/*
	copied from 
	http://www.lighthouse3d.com/opengl/glsl/index.php?oglinfo
 */
void printInfoLog(char *name, GLhandleARB obj) {
	//return;
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;
    glGetObjectParameterivARB(obj, 
                              GL_OBJECT_INFO_LOG_LENGTH_ARB, 
                              &infologLength);
    if (infologLength > 1) {
        infoLog = (char *)malloc(infologLength);
        glGetInfoLogARB(obj, infologLength, 
                        &charsWritten, infoLog);
		if( b_output_debug ) {
			printf( "%s:\n", name );
			printf(infoLog);
		    printf("\n");
		}
        free(infoLog);
    }
}

