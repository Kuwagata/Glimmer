#ifndef TEXTURE_MDSGPU
#define TEXTURE_MDSGPU 0

/*
	Data Structures
*/

typedef struct texture_t {
	int width;
	int height;
	int attachments;
	int attach_idx;
	GLuint frame_buffer;
	GLuint texture_name[2];	
	GLuint texture_unit;
	GLint offset;	
	int *tier_width;
	int *tier_height;
} Texture;

typedef struct pyramid_t {
	int height;
	Texture *t_level;
} Pyramid;

typedef struct tier_t {

	Texture *t_idx;
	Texture *t_reference;
	Texture *t_pts;
	Texture *t_embed;
	Texture *t_rand;
	Texture *t_diff;
	Texture *t_perm;
	Texture *t_stress;

	// textures for GPU-SF-Sparse

	Texture *t_vec_idx;

	// textures for GPU-SF

	Texture *t_d;
	Texture *t_g;
	//Texture *t_sum;
	Texture *t_force;
	Texture *t_velocity;

	// textures for GPU-Majorization
	
	Texture *t_ratio;
	Texture *t_Vdiag;
	

	GLuint vb_ssetBuffer;
	GLuint vb_vsetBuffer;
	GLuint vb_vertexBuffer;
	GLuint vb_edgeBuffer;
	GLuint vb_colorBuffer;
	int n_edgecount;
	int n_waffle;
	int n_setPoints;
	int n_points;
	bool b_update_s;
	struct tier_t *next;
	struct tier_t *prev;
	float *randr;
	float *perm;
	float *reference;
	float *d_page;
	int level;
	int n_levels;
} Tier;

/*
	Function Prototypes
*/
void sizeTextures( Tier *tier, int decimation_factor );
void genTextures( Tier *tier );
void outputTexture( Texture *texture, const char *filename );
void outputTextureLevel( Texture *texture, int level, const char *filename );
void setup_sh_copy( void );
void copyFlip( Texture *texture, int src, int dest, Tier *tier );
#endif