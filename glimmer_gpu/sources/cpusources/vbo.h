#ifndef VBO_MDSGPU
#define VBO_MDSGPU 0

#include "texture.h"
#include "graph.h"

/*
	Function Prototypes
*/
void initVBO( Tier *tier, const char *colors_filename );
void initEdges( Tier *tier, const char *matrixname );
void initEdgesAdj( Tier *tier, const char *adjfilename );
void initEdgesAdj( Tier *tier, vtx_data *graph, int n );

#endif