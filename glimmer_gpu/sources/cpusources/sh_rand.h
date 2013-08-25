#ifndef SH_RAND_MDSGPU
#define SH_RAND_MDSGPU 0

#include "texture.h"

#define SEQUENTIAL_SAMPLE	0

/*
	Function Prototypes
*/
int myrand( );
void setup_sh_rand( );
void random_sample( Texture *texture, Tier *tier, bool b_fix );
//void fix_indices( Texture *texture, Tier *tier );
void shuffle_texture( Texture *texture, Tier *tier );
void shuffle_index( Texture *texture, Tier *tier );
void initPerm( Tier *tier );
void initRand( Tier *tier );

#endif