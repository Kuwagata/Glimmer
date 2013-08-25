#ifndef SH_SORT_MDSGPU
#define SH_SORT_MDSGPU 0

#include "texture.h"

/*
	Function Prototypes
*/
void setup_sh_sort( );
void sort_even( Texture *texture_value, Texture *texture_key, Tier *tier );
void sort_odd( Texture *texture_value, Texture *texture_key, Tier *tier );
void mark_duplicates( Texture *texture_value, Texture *texture_key, Tier *tier );
void near_update_gen( Texture *texture_gen, Tier *tier );

#endif