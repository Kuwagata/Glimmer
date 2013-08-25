uniform sampler2DRect old;
uniform sampler2DRect embed;
uniform sampler2DRect reference;
uniform float pointwidth;
uniform float inputptx;
uniform float inputpty;

void main( )
{
	vec2 self = floor( gl_TexCoord[0].xy );
	vec4 color = vec4(0.0);
	vec4 pt = texture2DRect(old,vec2(self.x*pointwidth,self.y));
	vec4 temp = texture2DRect(embed,vec2(inputptx,inputpty));
	vec4 embedee = texture2DRect(embed,self);
	vec4 referenceself = texture2DRect(reference,self);
	vec4 referenceinput = texture2DRect(reference,vec2(inputptx,inputpty));

	// the final value stored is the (highD-lowD)^2 or highD^2

	float dist = max(pow(1.0-pt.x,2.0),0.0001);
	color.x = pow(distance(temp,embedee)-dist,2.0);
	color.y = dist*dist;

	// set the pixel to zero if the point's index is greater than the input point

	color.xy = (referenceself.x<referenceinput.x)?color.xy:vec2(0.0);

	gl_FragColor = color;
}