uniform sampler2DRect dup_key;
uniform sampler2DRect dst_key;

///////////////////////////////////////////////////////////////
//
//  Shader:  sh_near_update_dup.glsl
//
//           Update near set with sorting strategy.
//	     First sort by index, mark duplicates as inf h. distance
//	     then resort by h. distance.
//	     This code is hardcoded for neighbor sets of size 4 each.

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

	vec4 left_dup 	= texture2DRect( dup_key, vec2(self.x - mod(self.x,2.0), self.y) 	);
	vec4 right_dup 	= texture2DRect( dup_key, vec2(self.x + mod(self.x+1.0,2.0), self.y)	);
	vec4 left_dst 	= texture2DRect( dst_key, vec2(self.x - mod(self.x,2.0), self.y) 	);
	vec4 right_dst 	= texture2DRect( dst_key, vec2(self.x + mod(self.x+1.0,2.0), self.y) 	);


	// sort by index

	while( i < 4.0 ) {
		sort_odd( left_dup, 	left_dst );
		sort_odd( right_dup, 	right_dst );
		sort_odd( left_dup, 	left_dup );
		sort_odd( right_dup, 	right_dup );
		sort_even( left_dup, right_dup, left_dst, right_dst );
		sort_even( left_dup, right_dup, left_dup, right_dup );
		i = i + 1.0;
	}

	// mark duplicates

	left_dst.y = (left_dup.x==left_dup.y)?1000.0:left_dst.y;
	left_dst.z = (left_dup.y==left_dup.z)?1000.0:left_dst.z;
	left_dst.w = (left_dup.z==left_dup.w)?1000.0:left_dst.w;
	right_dst.x = (left_dup.w==right_dup.x)?1000.0:right_dst.x;
	right_dst.y = (right_dup.x==right_dup.y)?1000.0:right_dst.y;
	right_dst.z = (right_dup.y==right_dup.z)?1000.0:right_dst.z;
	right_dst.w = (right_dup.z==right_dup.w)?1000.0:right_dst.w;

	// sort by h. distance

	i = 0.0;
	while( i < 4.0 ) {
		sort_odd( left_dst, 	left_dup );
		sort_odd( right_dst, 	right_dup );
		sort_odd( left_dst, 	left_dst );
		sort_odd( right_dst, 	right_dst );
		sort_even( left_dst, right_dst, left_dup, right_dup );
		sort_even( left_dst, right_dst, left_dst, right_dst );
		i = i + 1.0;
	}

	// output resulting index set

	gl_FragColor = (mod(self.x,2.0)	== 0.0)?left_dup:right_dup;
}