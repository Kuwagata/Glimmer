#ifndef SH_STRESS_MDSGPU
#define SH_STRESS_MDSGPU 0

#include "texture.h"

/*
	Function Prototypes
*/
void setup_sh_stress( );
float calc_stress( Tier *tier );
float sum_velocity( Tier *tier, bool b_smartnorm );
float calc_displacement( Tier *tier );
float sum_all_forces( Tier *tier, bool b_smartnorm );
float calc_sp_stress_denom( Tier *tier, bool b_low_dim );
float calc_sp_stress( Tier *tier );
float calc_sp_stress_raw( Tier *tier );
float calc_sp_stress_1( Tier *tier );
float calc_sp_stress_norm( Tier *tier );
#endif