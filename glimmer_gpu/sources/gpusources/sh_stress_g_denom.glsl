uniform sampler2DRect embed;
uniform sampler2DRect reference;
uniform float pointwidth;
uniform float inputptx;
uniform float inputpty;

void main( )
{
	vec2 self = floor( gl_TexCoord[0].xy );
	vec4 color = vec4(0.0);
	vec4 temp = texture2DRect(embed,vec2(inputptx,inputpty));
	vec4 embedee = texture2DRect(embed,self);
	vec4 referenceself = texture2DRect(reference,self);
	vec4 referenceinput = texture2DRect(reference,vec2(inputptx,inputpty));

	// the final value stored is the (lowD)^2

	color.x = pow(distance(temp,embedee),2.0);

	// set the pixel to zero if the point's index is greater than the input point

	color.x = (referenceself.x<referenceinput.x)?color.x:0.0;

	gl_FragColor = color;
}