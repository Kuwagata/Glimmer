uniform sampler2DRect P;	// Permutation texture P
uniform float N;		// number of data points/4
uniform float offset; 		// linear offset into P
uniform float idxwidth;		// width of an index element (usually 2)	
uniform float idxspan;		// number of index elements in a texture row
uniform float randwidth;	// width of P

uniform float itemcount;
uniform float sw;
uniform float bw;

//uniform float roffset_x;
//uniform float roffset_y;
//uniform float texwidth;
//uniform float texheight;
//uniform float sw;
//uniform float bw;
//uniform float itemcount;

// grab the value of component the vector inputvec at 'address'
// 1,2,3,4 correspond to x y z w

float selectVal( float address, vec4 inputvec ) {

	float inv=1.0/address;
	vec4 selecvec=vec4(	floor(inv),
				floor(2.0*inv*clamp(address-1.0,0.0,1.0)),
				floor(3.0*inv*clamp(address-2.0,0.0,1.0)),
				floor(4.0*inv*clamp(address-3.0,0.0,1.0)));
	return dot(inputvec,selecvec);
}

void main()
{
	vec2 self = floor(gl_TexCoord[0].xy);
	float read_order = self.y*idxspan + floor( self.x / idxwidth );
	float read_p = floor( read_order / 4.0 );	
	vec4 perm = texture2DRect( P, vec2( mod(read_p , randwidth), floor( read_p / randwidth ) ) );
	float permsel = mod(selectVal(mod(read_order,4.0)+1.0, perm)+offset,N);
	vec4 randtex = texture2DRect( P, vec2( mod(permsel , randwidth), floor( permsel / randwidth ) ) );
	randtex = mod( randtex, itemcount );
	vec4 sx = mod( randtex, sw );
	vec4 sy = floor( randtex / sw );	
	gl_FragColor = sx + sy*bw;
	

	//float temp1 = roffset_x + floor(gl_TexCoord[0].x);
	//float temp2 = floor(temp1/texwidth);
	//float xoff = mod(temp1, texwidth);
	//float yoff = mod( roffset_y + floor(gl_TexCoord[0].y) + temp2, texheight);
	//vec4 randtex = vec4(texture2DRect(randomtexture, vec2( xoff, yoff ) ));
	//randtex = mod( randtex, itemcount );
	//vec4 sx = mod( randtex, sw );
	//vec4 sy = floor( randtex / sw );	
	//gl_FragColor = sx + sy*bw;
}