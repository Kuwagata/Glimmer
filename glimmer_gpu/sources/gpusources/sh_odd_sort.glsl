uniform sampler2DRect key;
uniform sampler2DRect value;
uniform float distwidth;

///////////////////////////////////////////////////////////////
//
//  Shader:  sh_odd_sort.glsl
//
//           The odd sort of an even-odd sorting network.
//

void main()
{
	vec4 key1 = texture2DRect( key, floor(gl_TexCoord[0].xy) );
	vec4 value1 = texture2DRect( value, floor(gl_TexCoord[0].xy) );
	float i = mod(floor( gl_TexCoord[0].x ), distwidth );
	gl_FragColor.x = (key1.x > key1.y) ? value1.y : value1.x;
	gl_FragColor.y = (key1.x > key1.y) ? value1.x : value1.y;
	gl_FragColor.z = (key1.z > key1.w) ? value1.w : value1.z;
	gl_FragColor.w = (key1.z > key1.w) ? value1.z : value1.w;
}
