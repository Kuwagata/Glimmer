uniform sampler2DRect diff; // imported difference texture

///////////////////////////////////////////////////////////////
//
//  Shader:  sh_dist_copy.glsl
//
//           Simply copies values from the difference texture
//           at the same coordinates as our shaded pixel
//           while taking the square root of the stored value.

void main() {

	vec2 self = floor( gl_TexCoord[0].xy );		// compute current coordinates
	vec4 diffval = texture2DRect( diff, self );	// grab the value from diff

	gl_FragColor = sqrt(diffval);			// store the square root of the value
}
