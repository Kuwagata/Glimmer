uniform sampler2DRect old;
uniform float texwidth;
uniform float texheight;
uniform float offset;

// Performs sum reduction by summing nearest 3 neighbors and self

void main( )
{

	// initialize

	vec2 self = floor( gl_TexCoord[0].xy );	// get own coordinate

	vec4 sumvec = vec4(0.0);		// vector to hold read summation values
	vec4 dotvec = vec4(1.0);		// selection vector for dot product


	// read intermediate sums

	vec4 readvec 	= abs(texture2DRect(old,self));
	sumvec.x 	= readvec.x;
	readvec 	= abs(texture2DRect(old,vec2(self.x+offset,self.y)));
	sumvec.y 	= readvec.x;
	readvec 	= abs(texture2DRect(old,vec2(self.x,self.y+offset)));
	sumvec.z 	= readvec.x;
	readvec 	= abs(texture2DRect(old,vec2(self.x+offset,self.y+offset)));
	sumvec.w 	= readvec.x;

	// construct selection vector

	dotvec.y = (texwidth-1.0<self.x+offset)?0.0:1.0;
	dotvec.z = (texheight-1.0<self.y+offset)?0.0:1.0;
	dotvec.w = (texwidth-1.0<self.x+offset)?0.0:1.0;
	dotvec.w = (texheight-1.0<self.y+offset)?0.0:dotvec.w;

	// sum values

	dotvec = ((mod(self.x,offset*2.0)>0.0)||(mod(self.y,offset*2.0)>0.0))?vec4(0.0):dotvec;

	// output to Texture

	gl_FragColor = vec4(dot(dotvec,sumvec));
}