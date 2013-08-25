uniform sampler2DRect pts;
uniform sampler2DRect reference;
uniform float pointwidth;
uniform float inputptx;
uniform float inputpty;

void main( )
{
	vec2 owncoord = floor( gl_TexCoord[0].xy );
	vec4 input = texture2DRect(pts,vec2(inputptx+mod( owncoord.x, pointwidth),inputpty));
	vec4 self = texture2DRect(pts,owncoord);
	vec4 color = input-self;

	// only set color to the difference if its index is less than the input point

	vec4 referenceself = texture2DRect(reference,vec2(floor(owncoord.x/pointwidth),owncoord.y));
	vec4 referenceinput = texture2DRect(reference,vec2(inputptx/pointwidth,inputpty));

	color = referenceself.x<referenceinput.x?color:vec4(0.0);

	gl_FragColor = color*color;
}
