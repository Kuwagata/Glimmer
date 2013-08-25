uniform sampler2DRect pts;	// values texture
uniform sampler2DRect vec_idx;	// indices texture
uniform sampler2DRect idx;	// set index texture
uniform float pointwidth;	// width of a single point
uniform float diffwidth;	// width of a difference element
uniform float idxwidth;		// width of a single index set
uniform float chunkwidth;	// width of a reduction element
uniform float binsperpt;	// typically the same as pointwidth
uniform float maxnz;		// the maximum number of nonzeros - 1
uniform float logmaxnz;		// ceil(log2(maxnz+1));
uniform float halfN;		// pow(2.0,logmaxnz-1);

///////////////////////////////////////////////////////////////
//
//  Shader:  sh_dot_term.glsl
//
//           compute each point's dot product terms between itself and
//           the elements of its index set.  This is complicated
//	     by the sparse storage strategy.  Instead of simply
//	     multiplying the two, we do a binary search over the
//           other points indices for the matching index.


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

// perform a binary search to find an index
//
// srcindex - the index we are searching for
// baseaddr - the base texture coordinate to search from 
//
// returns 0 if not found
// returns the corresponding value in pts if found

float binsearch( float srcindex, vec2 baseaddr ) {

	float lastguess = halfN;
	float two = ceil(halfN/2.0);
	vec4 guessvec;
	float guessval=0.0;
	float capval;

	float i = 0.0;
	//while( i < 1.0 ) {
	while( i < logmaxnz ) {
		capval = min(maxnz,lastguess);
		guessvec = texture2DRect( vec_idx, vec2(baseaddr.x+floor(capval/4.0), baseaddr.y ) );
		guessval = selectVal( mod(capval,4.0)+1.0, guessvec );
		if( srcindex > guessval ) {
			lastguess = lastguess + ceil(two);				
		}
		else if ( srcindex < guessval) {
			lastguess = lastguess - ceil(two);
		}

		// increment
		two = two / 2.0;
		i = i + 1.0;
	}	

	capval = min(maxnz,lastguess);
	guessvec = texture2DRect( vec_idx, vec2(baseaddr.x+floor(capval/4.0), baseaddr.y ) );
	guessval = selectVal( mod(capval,4.0)+1.0, guessvec );
	guessvec = texture2DRect( pts, vec2(baseaddr.x+floor(capval/4.0), baseaddr.y ) );
	return (srcindex==guessval)?selectVal( mod(capval,4.0)+1.0, guessvec ):0.0;
}

// 
//

vec2 getsearchaddr( vec4 index, float chunk ) {

	float address = mod(chunk,4.0)+1.0;
	float indy = selectVal(address,index);

	return vec2( binsperpt * mod( indy, pointwidth ), floor( indy / pointwidth ) );
}

//
// perform a binary search over a quad of points
//
float calc_prod( vec2 self, vec2 owncoord, float base ) {

	float chadd = mod(base,chunkwidth); // the offset in the chunk
	float chunk = floor(base/chunkwidth);   // which chunk
	vec4 index = texture2DRect( idx, vec2( owncoord.x * idxwidth + floor(chunk/4.0), self.y) );
	vec2 searchaddr  = getsearchaddr( index, chunk );
	vec4 vindexchunk = texture2DRect(vec_idx, vec2( owncoord.x * binsperpt + chadd, owncoord.y) );
	vec4 ptchunk     = texture2DRect(pts,     vec2( owncoord.x * binsperpt + chadd, owncoord.y) );
	vec4 temp = vec4(	ptchunk.x*binsearch(vindexchunk.x, searchaddr ),
				ptchunk.y*binsearch(vindexchunk.y, searchaddr ),
				ptchunk.z*binsearch(vindexchunk.z, searchaddr ),
				ptchunk.w*binsearch(vindexchunk.w, searchaddr ) );
	return dot(vec4(1.0),temp);
	//return binsearch(vindexchunk.y, searchaddr );
}

void main() {

	// init (grab current coords)

	vec4 ones = vec4(1.0);						
	vec2 self = floor( gl_TexCoord[0].xy );
	vec2 owncoord = vec2( floor( self.x / diffwidth ), self.y);  // get base address of diff element

	// for each group (quad) of a point's coords

	float base = mod(self.x,diffwidth)*4.0; 
	vec4 color = vec4(	calc_prod( self, owncoord, base),
				calc_prod( self, owncoord, base+1.0),
				calc_prod( self, owncoord, base+2.0),
				calc_prod( self, owncoord, base+3.0) );	

	gl_FragColor = color;
}