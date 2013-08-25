uniform sampler2DRect pts;	// points texture
uniform sampler2DRect idx;	// set index texture
uniform float pointwidth;	// width of a single point
uniform float diffwidth;	// width of a difference element
uniform float idxwidth;		// width of a single index set
uniform float chunkwidth;	// width of a reduction element
uniform float binsperpt;	// typically the same as pointwidth

///////////////////////////////////////////////////////////////
//
//  Shader:  sh_dist_diff.glsl
//
//           compute each point's difference between itself and
//           the elements of its index set.  square the values,
//           sum the quad, and store in a single component
//           of the output pixel


// grab the value of component the vector inputvec at 'address'
// 0,1,2,3 correspond to x y z w

float selectVal( float address, vec4 inputvec ) {

	float inv=1.0/address;
	vec4 selecvec=vec4(	floor(inv),
				floor(2.0*inv*clamp(address-1.0,0.0,1.0)),
				floor(3.0*inv*clamp(address-2.0,0.0,1.0)),
				floor(4.0*inv*clamp(address-3.0,0.0,1.0)));
	return dot(inputvec,selecvec);
}

void main() {

	// init (grab current coords)

	vec4 ones = vec4(1.0);						
	vec4 color = vec4(0.0);
	vec2 self = floor( gl_TexCoord[0].xy );
	vec2 owncoord = vec2( floor( self.x / diffwidth ), self.y);  // get base address of diff element

	// for each group (quad) of a point's coords

	float base = mod(self.x,diffwidth)*4.0; 
	float chadd = mod(base,chunkwidth); // the offset in the chunk
	float chunk = floor(base/chunkwidth);   // which chunk
	vec4 index = texture2DRect( idx, vec2( owncoord.x * idxwidth + floor(chunk/4.0), self.y) );
	float address = mod(chunk,4.0)+1.0;
	float indy = selectVal(address,index);
	float idxx = binsperpt * mod( indy, pointwidth );
	float idxy = floor( indy / pointwidth );
	vec4 ptchunk = texture2DRect(pts, vec2( owncoord.x * binsperpt + chadd, owncoord.y) );
	vec4 nbchunk = texture2DRect(pts, vec2( idxx + chadd, idxy) );
	vec4 temp = ptchunk-nbchunk;
	color.x = dot(ones,temp*temp);
	
	base=base+1.0;
	chadd = mod(base,chunkwidth); // the offset in the chunk
	chunk = floor(base/chunkwidth);   // which chunk
	index = texture2DRect(idx, vec2( owncoord.x * idxwidth + floor(chunk/4.0), self.y));
	address = mod(chunk,4.0)+1.0;
	indy = selectVal(address,index);
	idxx = binsperpt * mod( indy, pointwidth );
	idxy = floor( indy / pointwidth );
	ptchunk = texture2DRect(pts, vec2( owncoord.x * binsperpt + chadd, owncoord.y) );
	nbchunk = texture2DRect(pts, vec2( idxx + chadd, idxy) );
	temp = ptchunk-nbchunk;
	color.y = dot(ones,temp*temp);
	
	base=base+1.0;
	chadd = mod(base,chunkwidth); // the offset in the chunk 
	chunk = floor(base/chunkwidth);   // which chunk
	index = texture2DRect(idx, vec2( owncoord.x * idxwidth + floor(chunk/4.0), self.y));
	address = mod(chunk,4.0)+1.0;
	indy = selectVal(address,index);
	idxx = binsperpt * mod( indy, pointwidth );
	idxy = floor( indy / pointwidth );
	ptchunk = texture2DRect(pts, vec2( owncoord.x * binsperpt + chadd, owncoord.y) );
	nbchunk = texture2DRect(pts, vec2( idxx + chadd, idxy) );
	temp = ptchunk-nbchunk;	
	color.z = dot(ones,temp*temp);

	base=base+1.0;
	chadd = mod(base,chunkwidth); // the offset in the chunk 
	chunk = floor(base/chunkwidth);   // which chunk
	index = texture2DRect(idx, vec2( owncoord.x * idxwidth + floor(chunk/4.0), self.y));
	address = mod(chunk,4.0)+1.0;
	indy = selectVal(address,index);
	idxx = binsperpt * mod( indy, pointwidth );
	idxy = floor( indy / pointwidth );
	ptchunk = texture2DRect(pts, vec2( owncoord.x * binsperpt + chadd, owncoord.y) );
	nbchunk = texture2DRect(pts, vec2( idxx + chadd, idxy) );
	temp = ptchunk-nbchunk;
	color.w = dot(ones,temp*temp);

	gl_FragColor = color;
}