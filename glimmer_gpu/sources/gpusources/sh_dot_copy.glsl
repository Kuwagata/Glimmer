uniform sampler2DRect diff; // imported difference texture

///////////////////////////////////////////////////////////////
//
//  Shader:  sh_dist_copy.glsl
//
//           Simply copies values from the difference texture
//           at the same coordinates as our shaded pixel
//           while subracting the stored values from 1.0.

void main() {

	vec2 self = floor( gl_TexCoord[0].xy );		// compute current coordinates
	vec4 diffval = texture2DRect( diff, self );	// grab the value from diff

	gl_FragColor = max(pow(vec4(1.0)-diffval,vec4(2.0)),0.0001);		// store 1 minus the value
}
