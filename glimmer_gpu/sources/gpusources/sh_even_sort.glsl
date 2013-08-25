uniform sampler2DRect key;
uniform sampler2DRect value;
uniform float distwidth;

///////////////////////////////////////////////////////////////
//
//  Shader:  sh_sort_even.glsl
//
//           The even sort of an even-odd sorting network.
//

void main()
{

	vec2 coord = floor(gl_TexCoord[0].xy);
	vec4 key1 = texture2DRect( key, coord );
	vec4 value1 = texture2DRect( value, coord );
	vec4 compare1;
	vec4 compare2;
	vec4 set1;
	vec4 set2;

	float valuex=value1.x;
	float valuew=value1.w;

	float i = mod(coord.x, distwidth );
	if( i > 0.0 ) {
		compare1 = texture2DRect( key, coord - vec2(1.0, 0.0) );
		set1 = texture2DRect( value, coord - vec2(1.0, 0.0) );
		valuex = (key1.x >= compare1.w) ? valuex : set1.w;
	}
	gl_FragColor.x=valuex;

	if( i < (distwidth - 1.0) ) {
		compare2 = texture2DRect( key, coord + vec2( 1.0, 0.0 ) );
		set2 = texture2DRect( value, coord + vec2( 1.0, 0.0 ) );
		valuew = (key1.w > compare2.x) ? set2.x : valuew ;
	}
	gl_FragColor.w=valuew;

	gl_FragColor.y = (key1.y > key1.z) ? value1.z : value1.y;
	gl_FragColor.z = (key1.y > key1.z) ? value1.y : value1.z;
}
