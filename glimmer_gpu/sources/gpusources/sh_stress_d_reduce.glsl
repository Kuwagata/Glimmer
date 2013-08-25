uniform sampler2DRect old;
uniform float pointwidth;
uniform float chunkwidth;
void main( )
{
	vec2 self = floor( gl_TexCoord[0].xy );

	float offset = mod(self.x,pointwidth)*4.0;
	float base = floor(self.x/pointwidth)*pointwidth;

	vec4 ones = vec4(1.0);
	vec4 color = vec4(0.0);

	vec4 temp = texture2DRect(old,vec2(base+offset,self.y));
	color.x = (offset > chunkwidth-1.0)?0.0:dot(temp,ones);

	temp = texture2DRect(old,vec2(base+offset+1.0,self.y));
	color.y = (offset + 1.0 > chunkwidth-1.0)?0.0:dot(temp,ones);

	temp = texture2DRect(old,vec2(base+offset+2.0,self.y));
	color.z = (offset + 2.0 > chunkwidth-1.0)?0.0:dot(temp,ones);

	temp = texture2DRect(old,vec2(base+offset+3.0,self.y));
	color.w = (offset + 3.0 > chunkwidth-1.0)?0.0:dot(temp,ones);

	gl_FragColor = color;
}