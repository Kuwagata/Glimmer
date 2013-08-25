uniform sampler2DRect permutation;
uniform sampler2DRect index;
uniform float permwidth;

void main()
{
	vec2 selfcoord = floor( gl_TexCoord[0].xy );
	vec4 indices = texture2DRect(index, selfcoord );
	vec4 newval = texture2DRect( permutation, vec2( mod(indices.x,permwidth), floor( indices.x/permwidth) ) );
	indices.x = newval.x;
	newval = texture2DRect( permutation, vec2( mod(indices.y,permwidth), floor( indices.y/permwidth) ) );
	indices.y = newval.x;
	newval = texture2DRect( permutation, vec2( mod(indices.z,permwidth), floor( indices.z/permwidth) ) );
	indices.z = newval.x;
	newval = texture2DRect( permutation, vec2( mod(indices.w,permwidth), floor( indices.w/permwidth) ) );
	indices.w = newval.x;
	gl_FragColor = indices;
}