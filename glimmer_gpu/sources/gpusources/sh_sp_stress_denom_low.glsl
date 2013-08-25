uniform sampler2DRect t_d;
uniform sampler2DRect t_g;

/*
	Grab the low dimensional distances for a point
	Compute the square and
	sum them into a single value.

	to adjust this for index sets larger than 2 texels wide
	change the 2's to the desired width

*/
void main( ) {

	int i 		= 0;	// loop variable
	float stress_low	= 0.0;	// final denominator low^2

	// get read coordinate on distance textures

	vec2 self 	= floor( gl_TexCoord[0].xy );
	self.x 		= 2.0*self.x;			// adjust to index width

	// loop over each set of distances

	while( i < 2 ) {

		// read distance from the high/low textures

		vec4 high_d 	= texture2DRect(t_d,self);
		vec4 low_d 	= texture2DRect(t_g,self);
	
		// zero out any "ignored" distances (using special value 1000.0)
	
		low_d.x = (high_d.x==1000.0)?0.0:low_d.x;
		low_d.y = (high_d.y==1000.0)?0.0:low_d.y;
		low_d.z = (high_d.z==1000.0)?0.0:low_d.z;
		low_d.w = (high_d.w==1000.0)?0.0:low_d.w;

		high_d.x = (high_d.x==1000.0)?0.0:high_d.x;
		high_d.y = (high_d.y==1000.0)?0.0:high_d.y;
		high_d.z = (high_d.z==1000.0)?0.0:high_d.z;
		high_d.w = (high_d.w==1000.0)?0.0:high_d.w;

		// compute the squared differences and sum	

		stress_low 	+= dot(low_d,low_d);

		// adjust position on distance texture

		self.x = self.x + 1.0;
		i = i + 1;
	}

	// output raw stress sum

	gl_FragColor = vec4(stress_low,0.0,0.0,0.0);

}
