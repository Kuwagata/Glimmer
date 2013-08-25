uniform sampler2DRect t_velocity;
uniform sampler2DRect t_old;
uniform float deltatime;

void main()
{

	vec2 selfcoord = floor( gl_TexCoord[0].xy );
	vec4 self = texture2DRect( t_old, selfcoord );
	vec4 velocity = texture2DRect( t_velocity, selfcoord );

	gl_FragColor = self+ (velocity * deltatime);

}
