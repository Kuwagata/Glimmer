uniform sampler2DRect pts;
uniform sampler2DRect vec_idx;	// indices texture
uniform sampler2DRect reference;
uniform float pointwidth;
uniform float inputptx;
uniform float inputpty;
uniform float maxnz;		// the maximum number of nonzeros - 1
uniform float logmaxnz;		// ceil(log2(maxnz+1));
uniform float halfN;		// pow(2.0,logmaxnz-1);

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

void main( )
{
	vec2 owncoord = floor( gl_TexCoord[0].xy );
	vec4 input = texture2DRect(pts,vec2(inputptx+mod( owncoord.x, pointwidth),inputpty));
	vec4 self = texture2DRect(pts,owncoord);

	vec4 vindexchunk = texture2DRect(vec_idx, vec2(inputptx+mod( owncoord.x, pointwidth), inputpty) );
	vec4 ptchunk     = texture2DRect(pts,     vec2(inputptx+mod( owncoord.x, pointwidth), inputpty) );
	vec2 searchaddr  = vec2(floor(owncoord.x/pointwidth),owncoord.y);
	vec4 color = vec4(	ptchunk.x*binsearch(vindexchunk.x, searchaddr ),
				ptchunk.y*binsearch(vindexchunk.y, searchaddr ),
				ptchunk.z*binsearch(vindexchunk.z, searchaddr ),
				ptchunk.w*binsearch(vindexchunk.w, searchaddr ) );

	// only set color to the dot terms if its index is less than the input point

	vec4 referenceself = texture2DRect(reference,vec2(floor(owncoord.x/pointwidth),owncoord.y));
	vec4 referenceinput = texture2DRect(reference,vec2(inputptx/pointwidth,inputpty));

	gl_FragColor = referenceself.x<referenceinput.x?color:vec4(0.0);
}
