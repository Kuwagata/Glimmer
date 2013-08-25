uniform sampler2DRect t_embed;
void main( ) {
	vec4 test = texture2DRect(t_embed,gl_Vertex.xy);
	test.w = 1.0;
	gl_FrontColor = vec4(0.3,0.3,0.3,0.2);
	//gl_FrontColor = vec4(0.0,1.0,0.0,0.1);
	gl_Position = gl_ModelViewProjectionMatrix * test;
}
