uniform sampler2DRect permutation;
uniform sampler2DRect copytexture;
uniform float permwidth;
uniform float inputlength;

void main()
{
	vec2 selfcoord = floor( gl_TexCoord[0].xy );
	float offset = mod( selfcoord.x, inputlength );
	selfcoord.x = floor( selfcoord.x / inputlength );	
	vec4 posfull = vec4(texture2DRect(permutation, selfcoord ));
	vec2 newpos = vec2(mod(posfull.x, permwidth)*inputlength+offset,floor(posfull.x/permwidth));
	gl_FragColor = vec4(texture2DRect(copytexture, newpos ));
}