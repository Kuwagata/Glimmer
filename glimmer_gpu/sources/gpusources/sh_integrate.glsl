uniform sampler2DRect t_force;
uniform sampler2DRect t_old;
uniform float freeness;
uniform float deltatime;

void main() {
	vec2 selfcoord = floor( gl_TexCoord[0].xy );
	vec4 force = texture2DRect( t_force, selfcoord );
	vec4 velocity = texture2DRect( t_old, selfcoord );

	gl_FragColor = clamp((velocity + (force*deltatime)) * freeness, -2.0, 2.0);
}