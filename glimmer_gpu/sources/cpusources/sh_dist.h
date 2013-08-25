#ifndef SH_DIST_MDSGPU
#define SH_DIST_MDSGPU 0

#include "texture.h"

/*
	Function Prototypes
*/
void setup_sh_dist( );
void calc_dist_g( Texture *texture, Tier *tier  );
void calc_diff(  Tier *tier  );
void lookup_dist(  Tier *tier, bool b_fix  );
void calc_dot_product( Tier *tier );
void interpolate_points( Texture *texture, Tier *tier );

#endif