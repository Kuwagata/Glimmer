uniform sampler2DRect key_tex;
uniform sampler2DRect value_tex;

///////////////////////////////////////////////////////////////
//
//  Shader:  sh_near_update_gen.glsl
//
//           Simply sorts a pair of sets by some key


void sort_odd( vec4 key, inout vec4 value ) {
	value.xy = (key.x > key.y) ? value.yx : value.xy;
	value.zw = (key.z > key.w) ? value.wz : value.zw;
}

void sort_even( vec4 key_left, vec4 key_right, inout vec4 value_left, inout vec4 value_right ) {
	value_left.yz  = (key_left.y  > key_left.z ) ? value_left.zy  : value_left.yz;
	value_right.yz = (key_right.y > key_right.z) ? value_right.zy : value_right.yz;
	vec2 temp = vec2( value_left.w, value_right.x );
	value_left.w   = (key_left.w > key_right.x ) ? temp.y	: temp.x;
	value_right.x  = (key_left.w > key_right.x ) ? temp.x  	: temp.y;
}

void main() {

	float i = 0.0;
	vec2 self = floor(gl_TexCoord[0].xy);	// get current texture coord

	// extract the left/right pixels

	vec4 left_tex 	= texture2DRect( value_tex, vec2(self.x - mod(self.x,2.0), self.y) 	);
	vec4 right_tex 	= texture2DRect( value_tex, vec2(self.x + mod(self.x+1.0,2.0), self.y)	);
	vec4 left_key 	= texture2DRect( key_tex, vec2(self.x - mod(self.x,2.0), self.y) 	);
	vec4 right_key 	= texture2DRect( key_tex, vec2(self.x + mod(self.x+1.0,2.0), self.y) 	);

	// sort by index

	while( i < 4.0 ) {
		sort_odd( left_key, 	left_tex );
		sort_odd( right_key, 	right_tex );
		sort_odd( left_key, 	left_key );
		sort_odd( right_key, 	right_key );
		sort_even( left_key, right_key, left_tex, right_tex );

		//left_tex.yz  = (left_key.y  > left_key.z ) ? left_tex.zy  : left_tex.yz;
		//right_tex.yz = (right_key.y > right_key.z) ? right_tex.zy : right_tex.yz;
		//vec2 temp = vec2( left_tex.w, right_tex.x );
		//left_tex.w   = (left_key.w > right_key.x ) ? temp.y	: temp.x;
		//right_tex.x  = (left_key.w > right_key.x ) ? temp.x  	: temp.y;

		sort_even( left_key, right_key, left_key, right_key );
		i = i + 1.0;
	}

	gl_FragColor = (mod(self.x,2.0)	== 0.0)?left_tex:right_tex;
}