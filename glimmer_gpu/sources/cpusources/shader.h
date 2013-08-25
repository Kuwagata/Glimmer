#ifndef SHADER_MDSGPU
#define SHADER_MDSGPU 0

/*
	Data Structures
*/
typedef struct shader_t {
	GLhandleARB program;
	GLhandleARB shader;
} Shader;

typedef struct uniform_t {
	bool isFloat;
	GLint location;
	GLfloat valuef;
	GLint valuei;
} Uniform;

/*
	Function Prototypes
*/
void printInfoLog(char *name, GLhandleARB obj);
char *readShaderSource( const char *filename );

#endif