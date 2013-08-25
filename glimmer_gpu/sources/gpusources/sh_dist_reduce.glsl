uniform sampler2DRect diff;
uniform float oldchunkwidth;
uniform float newchunkwidth;

///////////////////////////////////////////////////////////////
//
//  Shader:  sh_dist_reduce.glsl
//
//           sum/reduce the squared sums produced by sh_dist_diff
//           by a factor of 4.
//
//           the main complications of the shader is the computation
//           of the addressing.
//

// grab the inital components of a vector up to 'address'

vec4 prevec( float address ) {

return clamp(address-vec4(0.0,1.0,2.0,3.0), 0.0,1.0 );
}

// grab the components of a vector after and including 'address'

vec4 postvec( float address ) {

return clamp( floor( (1.0/address) * vec4(1.0,2.0,3.0,4.0) ), 0.0, 1.0);
}

void main() {

	vec4 color = vec4(0.0);
	vec2 self = floor( gl_TexCoord[0].xy );
	float base = self.x*4.0;
	float chunk = floor( base / newchunkwidth );
	float chadd = mod( base, newchunkwidth )*4.0;
	float absolutepos = chunk * oldchunkwidth + chadd;
	vec4 bin1=texture2DRect( diff, vec2( floor( absolutepos / 4.0 ), self.y ) );
	vec4 bin2=texture2DRect( diff, vec2( floor( absolutepos / 4.0 )+1.0, self.y ) );
	float pos = mod( absolutepos, 4.0 );
	float rn = min(4.0,oldchunkwidth-chadd);
	vec4 selecvec = vec4(1.0);
	if( pos == 0.0 ) {
		selecvec = prevec( rn );
	}
	else {
		if( rn > 4.0-pos ){
			selecvec = prevec( rn-(4.0-pos) );
			color.x = dot(selecvec,bin2);
		}
		selecvec = postvec( pos + 1.0 );
		if(rn<4.0-pos)selecvec=selecvec*prevec( pos + 1.0 );
	}
	color.x = color.x+dot( selecvec, bin1 );

	base = base+1.0;
	chunk = floor( base / newchunkwidth );
	chadd = mod( base, newchunkwidth )*4.0;
	absolutepos = chunk * oldchunkwidth + chadd;
	bin1=texture2DRect( diff, vec2( floor( absolutepos / 4.0 ), self.y ) );
	bin2=texture2DRect( diff, vec2( floor( absolutepos / 4.0 )+1.0, self.y ) );
	pos = mod( absolutepos, 4.0 );
	rn = min(4.0,oldchunkwidth-chadd);
	selecvec = vec4(1.0);
	if( pos == 0.0 ) {
	selecvec = prevec( rn );
	}
	else {
		if( rn > 4.0-pos){
			selecvec = prevec( rn-(4.0-pos) );
			color.y = dot(selecvec,bin2);
		}
		selecvec = postvec( pos + 1.0 );
		if(rn<4.0-pos)selecvec=selecvec*prevec( pos + 1.0 );
	}
	color.y = color.y+dot( selecvec, bin1 );

	base = base+1.0;
	chunk = floor( base / newchunkwidth );
	chadd = mod( base, newchunkwidth )*4.0;
	absolutepos = chunk * oldchunkwidth + chadd;
	bin1=texture2DRect( diff, vec2( floor( absolutepos / 4.0 ), self.y ) );
	bin2=texture2DRect( diff, vec2( floor( absolutepos / 4.0 )+1.0, self.y ) );
	pos = mod( absolutepos, 4.0 );
	rn = min(4.0,oldchunkwidth-chadd);
	selecvec = vec4(1.0);
	if( pos == 0.0 ) {
		selecvec = prevec( rn );
	}
	else {
		if( rn > 4.0-pos){
			selecvec = prevec( rn-(4.0-pos) );
			color.z = dot(selecvec,bin2);
		}
		selecvec = postvec( pos + 1.0 );
		if(rn<4.0-pos)selecvec=selecvec*prevec( pos + 1.0 );
	}
	color.z = color.z+dot( selecvec, bin1 );

	base = base+1.0;
	chunk = floor( base / newchunkwidth );
	chadd = mod( base, newchunkwidth )*4.0;
	absolutepos = chunk * oldchunkwidth + chadd;
	bin1=texture2DRect( diff, vec2( floor( absolutepos / 4.0 ), self.y ) );
	bin2=texture2DRect( diff, vec2( floor( absolutepos / 4.0 )+1.0, self.y ) );
	pos = mod( absolutepos, 4.0 );
	rn = min(4.0,oldchunkwidth-chadd);
	selecvec = vec4(1.0);
	if( pos == 0.0 ) {
		selecvec = prevec( rn );
	}
	else {
		if( rn > 4.0-pos){
			selecvec = prevec( rn-(4.0-pos) );
			color.w = dot(selecvec,bin2);
		}
		selecvec = postvec( pos + 1.0 );
		if(rn<4.0-pos)selecvec=selecvec*prevec( pos + 1.0 );
	}
	color.w = color.w+dot( selecvec, bin1 );

	gl_FragColor = color;
}