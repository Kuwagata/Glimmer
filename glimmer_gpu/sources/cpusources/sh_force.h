#ifndef SH_FORCE_MDSGPU
#define SH_FORCE_MDSGPU 0

#include "texture.h"

/*
	Function Prototypes
*/
void setup_sh_force( );
void sum_forces_graph( Texture *texture, Tier *tier, bool b_fix );
void sum_forces( Texture *texture, Tier *tier, bool b_fix );
void apply_forces( Texture *texture, Tier *tier, bool b_fix );
void integrate_forces( Texture *texture, Tier *tier, bool b_fix );
void apply_velocity( Texture *texture, Tier *tier, bool b_fix );

#define SPRING_FORCE		0.7
#define DAMPING_FACTOR		0.3
#define FREENESS			0.85
#define DELTA_TIME			0.3
#define DATA_SIZE_FACTOR	(1.0 / (double) (n_vmax + n_smax))
//#define DATA_SIZE_FACTOR	(1.0 / (double) 8.0)
//#define DAMPING_FACTOR		1.5
//#define DELTA_TIME			0.05
#endif