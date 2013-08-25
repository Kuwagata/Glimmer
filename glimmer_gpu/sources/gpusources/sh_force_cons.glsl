uniform sampler2DRect t_idx;
uniform sampler2DRect t_d;
uniform sampler2DRect t_sum;
uniform sampler2DRect t_old;
uniform float binsperpt;
uniform float embedwidth;
uniform float distwidth;
uniform float n_pass;

void main()
{
	vec2 selfcoord = floor( gl_TexCoord[0].xy );
	
	//vec2 owncoord = vec2( selfcoord.x * distwidth + n_pass, selfcoord.y);
	//vec4 indices = texture2DRect( t_idx, owncoord);
	//vec4 d_vec = texture2DRect( t_d, owncoord );
	//vec4 myforce = texture2DRect(t_old, selfcoord);
	
	vec4 mysum = texture2DRect(t_sum, selfcoord);
	
	//vec4 indexx   = binsperpt * mod( indices, embedwidth );
	//vec4 indexy   = floor( indices / embedwidth );\n"
	//vec4 zero = vec4(0.0,0.0,0.0,0.0);
	//vec4 sum0 = (d_vec.x==1000.0)?zero:texture2DRect(t_sum, vec2(indexx.x,indexy.x));
	//vec4 sum1 = (d_vec.y==1000.0)?zero:texture2DRect(t_sum, vec2(indexx.y,indexy.y));
	//vec4 sum2 = (d_vec.z==1000.0)?zero:texture2DRect(t_sum, vec2(indexx.z,indexy.z));
	//vec4 sum3 = (d_vec.w==1000.0)?zero:texture2DRect(t_sum, vec2(indexx.w,indexy.w));
	//sum0 = zero;//(d_vec.x==0.0)?zero:sum0;
	//sum1 = zero;//(d_vec.y==0.0)?zero:sum1;
	//sum2 = zero;//(d_vec.z==0.0)?zero:sum2;
	//sum3 = zero;//(d_vec.w==0.0)?zero:sum3;
	//vec4 final = myforce;//-sum0-sum1-sum2-sum3;
	//final = (n_pass==0.0)?final+mysum:final;
	
	gl_FragColor = mysum;
}