uniform sampler2DRect pts;	// distance lookup texture
uniform sampler2DRect idx;	// set index texture
uniform sampler2DRect perm;	// personal index lookup texture

uniform float lookupheight;	// height of lookup table
uniform float lookupwidth;	// width of lookup table * 4 - zero buffer
uniform float diffwidth;	// width of a difference element
uniform float pointcount;	// total number of points to process
uniform float refwidth;		// size of reference width

///////////////////////////////////////////////////////////////
//
//  Shader:  sh_dist_lookup.glsl
//
//           lookup each point's difference between itself and
//           the elements of its index set.


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

void main() {

	// init (grab current coords)

	vec4 color = vec4(0.0);
	vec2 self = floor( gl_TexCoord[0].xy );
	vec2 owncoord = vec2( floor( self.x / diffwidth ), self.y );  // get own coordinate
	vec4 temp = texture2DRect( perm, owncoord );
	float ownindex = temp.x;

	vec4 index = texture2DRect( idx, self ); // read quad from index set

	// transform values in index according to reference texture

	vec4 indexx = mod( index, refwidth );
	vec4 indexy = floor( index / refwidth );	
	temp = texture2DRect( perm, vec2( indexx.x, indexy.x ) );
	index.x = temp.x;	
	temp = texture2DRect( perm, vec2( indexx.y, indexy.y ) );
	index.y = temp.x;	
	temp = texture2DRect( perm, vec2( indexx.z, indexy.z ) );
	index.z = temp.x;	
	temp = texture2DRect( perm, vec2( indexx.w, indexy.w ) );
	index.w = temp.x;	

	// copy each element in the output quad from the distance lookup
	
	float row = max(index.x,ownindex)+1.0;
	float col = min(index.x,ownindex)+1.0;
	col = (row <= lookupheight)?col-1.0:lookupwidth-col;
	row = (row <= lookupheight)?row-1.0:(pointcount-row);
	vec4 tempresult = texture2DRect( pts, vec2(floor(col/4.0), row) );
	color.x = selectVal( mod( col , 4.0 )+1.0, tempresult );
	//color.x = ownindex;

	row = max(index.y,ownindex)+1.0;
	col = min(index.y,ownindex)+1.0;
	col = (row <= lookupheight)?col-1.0:lookupwidth-col;
	row = (row <= lookupheight)?row-1.0:(pointcount-row);
	tempresult = texture2DRect( pts, vec2(floor(col/4.0), row) );
	color.y = selectVal( mod( col , 4.0 )+1.0, tempresult );

	row = max(index.z,ownindex)+1.0;
	col = min(index.z,ownindex)+1.0;
	col = (row <= lookupheight)?col-1.0:lookupwidth-col;
	row = (row <= lookupheight)?row-1.0:(pointcount-row);
	tempresult = texture2DRect( pts, vec2(floor(col/4.0), row) );
	color.z = selectVal( mod( col , 4.0 )+1.0, tempresult );

	row = max(index.w,ownindex)+1.0;
	col = min(index.w,ownindex)+1.0;
	col = (row <= lookupheight)?col-1.0:lookupwidth-col;
	row = (row <= lookupheight)?row-1.0:(pointcount-row);
	tempresult = texture2DRect( pts, vec2(floor(col/4.0), row) );
	color.w = selectVal( mod( col , 4.0 )+1.0, tempresult );

	gl_FragColor = color;
}