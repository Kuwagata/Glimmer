uniform sampler2DRect key;
uniform sampler2DRect value;
uniform float distwidth;
uniform float idxwidth;

///////////////////////////////////////////////////////////////
//
//  Shader:  sh_dup.glsl
//
//           Checks the value to the right of the vector's
//           components.  If it sees a duplicate, then it sets
//           the difference to a predefined value (1000.0)
//

void main()
{	
	vec2 selfcoord = floor(gl_TexCoord[0].xy);
	vec4 key1 = texture2DRect( key, selfcoord );
	vec4 value1 = texture2DRect( value, selfcoord );
	vec4 compare1;

	float indexval = floor( selfcoord.x / distwidth ) + selfcoord.y * idxwidth;
	
	if(indexval==key1.x)value1.x=1000.0;
	if(indexval==key1.y)value1.y=1000.0;
	if(indexval==key1.z)value1.z=1000.0;
	if(indexval==key1.w)value1.w=1000.0;

	float valuex = value1.x;
	float valuew = value1.w;

	float i = mod(selfcoord.x, distwidth );
	if( i > 0.0 ) {
		compare1 = texture2DRect( key, selfcoord - vec2(1.0, 0.0) );
		valuex = (key1.x == compare1.w) ? 1000.0 : valuex;
	}
	gl_FragColor.x=valuex;
	gl_FragColor.y = (key1.y == key1.x) ? 1000.0 : value1.y;
	gl_FragColor.z = (key1.z == key1.y) ? 1000.0 : value1.z;
	gl_FragColor.w = (key1.w == key1.z) ? 1000.0 : value1.w;
}