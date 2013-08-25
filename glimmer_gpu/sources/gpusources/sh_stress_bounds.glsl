uniform sampler2DRect input;
uniform float texwidth;
uniform float texheight;
uniform float mode;
uniform float skip;

void main()
{
	
	vec2 self = floor( gl_TexCoord[0].xy );
	vec4 ownval = texture2DRect(input,self);
	vec4 color = vec4(0.0);
	if( mode < 1.0 ) {	// max mode

		vec4 rightval = (texwidth-1.0<self.x+skip)?vec4(-1000.0):texture2DRect(input,vec2(self.x+skip,self.y));
		vec4 downval = (texheight-1.0<self.y+skip)?vec4(-1000.0):texture2DRect(input,vec2(self.x,self.y+skip));
		vec4 diagval = ((texwidth-1.0<self.x+skip)||(texheight-1.0<self.y+skip))?vec4(-1000.0):texture2DRect(input,vec2(self.x+skip,self.y+skip));
		color.x = max( rightval.x, max( downval.x, max( diagval.x, ownval.x ) ) );
		color.y = max( rightval.y, max( downval.y, max( diagval.y, ownval.y ) ) );
	}	
	else {			// min mode

		vec4 rightval = (texwidth-1.0<self.x+skip)?vec4(1000.0):texture2DRect(input,vec2(self.x+skip,self.y));
		vec4 downval = (texheight-1.0<self.y+skip)?vec4(1000.0):texture2DRect(input,vec2(self.x,self.y+skip));
		vec4 diagval = ((texwidth-1.0<self.x+skip)||(texheight-1.0<self.y+skip))?vec4(1000.0):texture2DRect(input,vec2(self.x+skip,self.y+skip));
		color.x = min( rightval.x, min( downval.x, min( diagval.x, ownval.x ) ) );
		color.y = min( rightval.y, min( downval.y, min( diagval.y, ownval.y ) ) );
	}

	gl_FragColor = vec4( color.x, color.y, 0.0, 0.0 );
}
