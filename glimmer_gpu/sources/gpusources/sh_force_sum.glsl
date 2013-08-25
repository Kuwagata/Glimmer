uniform sampler2DRect t_idx;
uniform sampler2DRect t_d;
uniform sampler2DRect t_g;
uniform sampler2DRect t_embed;
uniform sampler2DRect t_old;
uniform sampler2DRect t_velocity;
uniform float n_pass;
uniform float binsperpt;
uniform float embedwidth;
uniform float distwidth;
uniform float springforce;
uniform float damping;
uniform float finalpass;
uniform float sizefactor;
uniform float debug;
uniform float weight;

void main()
{
	vec2 selfcoord = floor( gl_TexCoord[0].xy );
	vec2 owncoord = vec2( selfcoord.x * distwidth + n_pass, selfcoord.y);
	vec4 d_vec = texture2DRect( t_d, owncoord );
	vec4 g_vec = texture2DRect( t_g, owncoord );
	vec4 indices = texture2DRect( t_idx, owncoord);

	vec4 diff_vec;
	//diff_vec.x = ((d_vec.x==1000.0)||(d_vec.x>weight))?0.0:(g_vec.x-d_vec.x);
	//diff_vec.y = ((d_vec.y==1000.0)||(d_vec.y>weight))?0.0:(g_vec.y-d_vec.y);
	//diff_vec.z = ((d_vec.z==1000.0)||(d_vec.z>weight))?0.0:(g_vec.z-d_vec.z);
	//diff_vec.w = ((d_vec.w==1000.0)||(d_vec.w>weight))?0.0:(g_vec.w-d_vec.w);
	diff_vec.x = (d_vec.x==1000.0)?0.0:(g_vec.x-d_vec.x);
	diff_vec.y = (d_vec.y==1000.0)?0.0:(g_vec.y-d_vec.y);
	diff_vec.z = (d_vec.z==1000.0)?0.0:(g_vec.z-d_vec.z);
	diff_vec.w = (d_vec.w==1000.0)?0.0:(g_vec.w-d_vec.w);
	//diff_vec.x = (d_vec.x==1000.0)?0.0:(g_vec.x-step(0.5,d_vec.x));
	//diff_vec.y = (d_vec.y==1000.0)?0.0:(g_vec.y-step(0.5,d_vec.y));
	//diff_vec.z = (d_vec.z==1000.0)?0.0:(g_vec.z-step(0.5,d_vec.z));
	//diff_vec.w = (d_vec.w==1000.0)?0.0:(g_vec.w-step(0.5,d_vec.w));
	//diff_vec.x = (d_vec.x==1000.0)?0.0:(g_vec.x-pow(d_vec.x,0.5));
	//diff_vec.y = (d_vec.y==1000.0)?0.0:(g_vec.y-pow(d_vec.y,0.5));
	//diff_vec.z = (d_vec.z==1000.0)?0.0:(g_vec.z-pow(d_vec.z,0.5));
	//diff_vec.w = (d_vec.w==1000.0)?0.0:(g_vec.w-pow(d_vec.w,0.5));
	
	// newer heuristic
	//diff_vec.x = diff_vec.x*max(0.1,1.0-pow(d_vec.x,4.0));
	//diff_vec.y = diff_vec.y*max(0.1,1.0-pow(d_vec.y,4.0));
	//diff_vec.z = diff_vec.z*max(0.1,1.0-pow(d_vec.z,4.0));
	//diff_vec.w = diff_vec.w*max(0.1,1.0-pow(d_vec.w,4.0));

	// newer heuristic
	//diff_vec.x = diff_vec.x*min(1.0,((d_vec.x==1000.0)?0.0:(1.0/(d_vec.x*d_vec.x*3.0))));
	//diff_vec.y = diff_vec.y*min(1.0,((d_vec.y==1000.0)?0.0:(1.0/(d_vec.y*d_vec.y*3.0))));
	//diff_vec.z = diff_vec.z*min(1.0,((d_vec.z==1000.0)?0.0:(1.0/(d_vec.z*d_vec.z*3.0))));
	//diff_vec.w = diff_vec.w*min(1.0,((d_vec.w==1000.0)?0.0:(1.0/(d_vec.w*d_vec.w*3.0))));

	// original one... hold on to this one
	//diff_vec.x = diff_vec.x*(1.0-smoothstep(weight,weight+debug,diff_vec.x));
	//diff_vec.y = diff_vec.y*(1.0-smoothstep(weight,weight+debug,diff_vec.y));
	//diff_vec.z = diff_vec.z*(1.0-smoothstep(weight,weight+debug,diff_vec.z));
	//diff_vec.w = diff_vec.w*(1.0-smoothstep(weight,weight+debug,diff_vec.w));

	diff_vec = diff_vec*springforce;

	vec4 mypt = texture2DRect(t_embed, selfcoord);
	vec4 myvel = texture2DRect(t_velocity, selfcoord);
	vec4 indexx   = binsperpt * mod( indices, embedwidth );
	vec4 indexy   = floor( indices / embedwidth );
	vec4 zero = vec4(0.0);

	//vec4 relativeVel0 = ((d_vec.x==1000.0)||(d_vec.x>weight))?zero:(texture2DRect(t_velocity, vec2(indexx.x,indexy.x))-myvel);
	//vec4 relativeVel1 = ((d_vec.y==1000.0)||(d_vec.y>weight))?zero:(texture2DRect(t_velocity, vec2(indexx.y,indexy.y))-myvel);
	//vec4 relativeVel2 = ((d_vec.z==1000.0)||(d_vec.z>weight))?zero:(texture2DRect(t_velocity, vec2(indexx.z,indexy.z))-myvel);
	//vec4 relativeVel3 = ((d_vec.w==1000.0)||(d_vec.w>weight))?zero:(texture2DRect(t_velocity, vec2(indexx.w,indexy.w))-myvel);
	vec4 relativeVel0 = (d_vec.x==1000.0)?zero:(texture2DRect(t_velocity, vec2(indexx.x,indexy.x))-myvel);
	vec4 relativeVel1 = (d_vec.y==1000.0)?zero:(texture2DRect(t_velocity, vec2(indexx.y,indexy.y))-myvel);
	vec4 relativeVel2 = (d_vec.z==1000.0)?zero:(texture2DRect(t_velocity, vec2(indexx.z,indexy.z))-myvel);
	vec4 relativeVel3 = (d_vec.w==1000.0)?zero:(texture2DRect(t_velocity, vec2(indexx.w,indexy.w))-myvel);

	vec4 old = texture2DRect( t_old, selfcoord );

	vec4 tst0 = texture2DRect(t_embed, vec2(indexx.x,indexy.x))-mypt;
	vec4 tst1 = texture2DRect(t_embed, vec2(indexx.y,indexy.y))-mypt;
	vec4 tst2 = texture2DRect(t_embed, vec2(indexx.z,indexy.z))-mypt;
	vec4 tst3 = texture2DRect(t_embed, vec2(indexx.w,indexy.w))-mypt;

	vec4 pt0 = (length(tst0)==0.0)?zero:normalize( tst0 );
	vec4 pt1 = (length(tst1)==0.0)?zero:normalize( tst1 );
	vec4 pt2 = (length(tst2)==0.0)?zero:normalize( tst2 );
	vec4 pt3 = (length(tst3)==0.0)?zero:normalize( tst3 );

	vec4 sep = (damping * vec4(	dot(pt0,relativeVel0), dot(pt1,relativeVel1), dot(pt2,relativeVel2), dot(pt3,relativeVel3))) + diff_vec;

	vec4 final = (sep.x*(pt0))+(sep.y*(pt1))+(sep.z*(pt2))+(sep.w*(pt3));

	//if( n_pass > 0.0 ) {
		final = final + old;
	//}

	if( n_pass >= (finalpass - 1.0) ) {
		final = final * sizefactor;
	}
	//if( debug == 1.0 ) {
	//	final = sep;
	//}
	gl_FragColor = final;
}