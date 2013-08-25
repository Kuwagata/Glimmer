uniform sampler2DRect t_embed;
void main( ) {
	vec4 test = texture2DRect(t_embed,gl_Color.xy);
	//vec4 test = vec4(gl_Color.x,gl_Color.y,0.0,1.0);
	test.w = 1.0;
	gl_FrontColor = vec4(1.0,1.0,1.0,0.5);
	gl_Position = gl_ModelViewProjectionMatrix * test;
}
