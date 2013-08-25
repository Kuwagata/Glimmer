uniform sampler2DRect old;
uniform float texwidth;
uniform float texheight;
uniform float offset;

// Performs sum reduction by summing nearest 3 neighbors and self

void main( )
{

	// initialize

	vec2 self = floor( gl_TexCoord[0].xy );	// get own coordinate

	vec4 sumvec1 = vec4(0.0);		// vector to hold read summation values
	vec4 sumvec2 = vec4(0.0);		// vector to hold read summation values
	vec4 dotvec = vec4(1.0);		// selection vector for dot product


	// read intermediate sums

	vec4 readvec 	= abs(texture2DRect(old,self));
	sumvec1.x 	= readvec.x;
	sumvec2.x 	= readvec.y;
	readvec 	= abs(texture2DRect(old,vec2(self.x+offset,self.y)));
	sumvec1.y 	= readvec.x;
	sumvec2.y 	= readvec.y;
	readvec 	= abs(texture2DRect(old,vec2(self.x,self.y+offset)));
	sumvec1.z 	= readvec.x;
	sumvec2.z 	= readvec.y;
	readvec 	= abs(texture2DRect(old,vec2(self.x+offset,self.y+offset)));
	sumvec1.w 	= readvec.x;
	sumvec2.w 	= readvec.y;

	// construct selection vector

	dotvec.y = (texwidth-1.0<self.x+offset)?0.0:1.0;
	dotvec.z = (texheight-1.0<self.y+offset)?0.0:1.0;
	dotvec.w = (texwidth-1.0<self.x+offset)?0.0:1.0;
	dotvec.w = (texheight-1.0<self.y+offset)?0.0:dotvec.w;

	// sum values

	dotvec = ((mod(self.x,offset*2.0)>0.0)||(mod(self.y,offset*2.0)>0.0))?vec4(0.0):dotvec;

	// output to Texture

	gl_FragColor = vec4(dot(dotvec,sumvec1),dot(dotvec,sumvec2),0.0,0.0);
}