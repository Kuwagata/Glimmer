uniform sampler2DRect t_copy;

void main()
{
	gl_FragColor = texture2DRect( t_copy, floor( gl_TexCoord[0].xy ) );
}
