uniform sampler2DRect page;	// the page snagged from D''
uniform float dwidth;		// width of a distance element
uniform float dspan;		// number of distance elements in a span
uniform float pspan;		// number of elements in a span of a page
uniform float iterationoffs;	// iteration within page

// read from the distance page texture by
// offsetting according to the current iteration

void main()
{
	vec2 self = floor(gl_TexCoord[0].xy);
	float read_order = self.y*dspan + floor( self.x / dwidth ) + iterationoffs;
	gl_FragColor = 	texture2DRect( page, vec2(mod(read_order,pspan),floor(read_order/pspan)) );
}