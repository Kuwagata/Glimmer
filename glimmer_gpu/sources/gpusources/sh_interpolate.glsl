uniform sampler2DRect t_idx;
uniform sampler2DRect t_d;
uniform sampler2DRect t_embed;
uniform float embedwidth;
uniform float distwidth;
uniform float binsperpt;

/*
	Positions a point at the weighted barycenter of the points in its near set.		
*/

void main()
{
	vec2 selfcoord	= floor( gl_TexCoord[0].xy );
	vec2 owncoord 	= vec2( selfcoord.x * distwidth, selfcoord.y);
	vec4 indices 	= texture2DRect( t_idx, owncoord);
	vec4 dists   	= texture2DRect( t_d, owncoord);
	vec4 indexx   	= binsperpt * mod( indices, embedwidth );
	vec4 indexy   	= floor( indices / embedwidth );

/*
	float total_d 	= (dists.x==1000.0||dists.x==0.0)?0.0:(1.0/dists.x);	
	total_d 	= total_d + ((dists.y==1000.0||dists.y==0.0)?0.0:(1.0/dists.y));	
	total_d 	= total_d + ((dists.z==1000.0||dists.z==0.0)?0.0:(1.0/dists.z));	
	total_d 	= total_d + ((dists.w==1000.0||dists.w==0.0)?0.0:(1.0/dists.w));
	vec4 barycenter = (dists.x==1000.0||dists.x==0.0)?vec4(0.0):(1.0/dists.x)*texture2DRect(t_embed, vec2(indexx.x,indexy.x));
	barycenter = barycenter + ((dists.y==1000.0||dists.y==0.0)?vec4(0.0):(1.0/dists.y)*texture2DRect(t_embed, vec2(indexx.y,indexy.y)));
	barycenter = barycenter + ((dists.z==1000.0||dists.z==0.0)?vec4(0.0):(1.0/dists.z)*texture2DRect(t_embed, vec2(indexx.z,indexy.z)));
	barycenter = barycenter + ((dists.w==1000.0||dists.w==0.0)?vec4(0.0):(1.0/dists.w)*texture2DRect(t_embed, vec2(indexx.w,indexy.w)));
	gl_FragColor = (1.0/total_d)*barycenter;
*/
	float total_d 	= (dists.x==1000.0||dists.x==0.0)?0.0:(1.0/(dists.x*dists.x));	
	total_d 	= total_d + ((dists.y==1000.0||dists.y==0.0)?0.0:(1.0/(dists.y*dists.y)));	
	total_d 	= total_d + ((dists.z==1000.0||dists.z==0.0)?0.0:(1.0/(dists.z*dists.z)));	
	total_d 	= total_d + ((dists.w==1000.0||dists.w==0.0)?0.0:(1.0/(dists.w*dists.w)));
	vec4 barycenter = (dists.x==1000.0||dists.x==0.0)?vec4(0.0):(1.0/(dists.x*dists.x))*texture2DRect(t_embed, vec2(indexx.x,indexy.x));
	barycenter = barycenter + ((dists.y==1000.0||dists.y==0.0)?vec4(0.0):(1.0/(dists.y*dists.y))*texture2DRect(t_embed, vec2(indexx.y,indexy.y)));
	barycenter = barycenter + ((dists.z==1000.0||dists.z==0.0)?vec4(0.0):(1.0/(dists.z*dists.z))*texture2DRect(t_embed, vec2(indexx.z,indexy.z)));
	barycenter = barycenter + ((dists.w==1000.0||dists.w==0.0)?vec4(0.0):(1.0/(dists.w*dists.w))*texture2DRect(t_embed, vec2(indexx.w,indexy.w)));
	barycenter = (1.0/total_d)*barycenter;
	barycenter.w = 1.0;
	gl_FragColor = barycenter;

	//gl_FragColor = vec(0.0,0.0,0.0,0.1);

/*
	int i = 0;

	vec4 nnpos = texture2DRect(t_embed, vec2(indexx.x,indexy.x));
	vec4 nnpos2 = texture2DRect(t_embed, vec2(indexx.y,indexy.y));
	vec4 nnpos3 = texture2DRect(t_embed, vec2(indexx.z,indexy.z));
	vec4 nnpos4 = texture2DRect(t_embed, vec2(indexx.w,indexy.w));

	float angle_offset = 0.0;
	float new_angle_offset = 0.0;
	float angle_span   = 6.28318; // 2 * PI
	vec4 final_pos = vec4(0.0);
	vec4 posdebug = vec4(0.0);

	while( i < 2 ) {

	i = i + 1;

	angle_span = angle_span / 4.0;
	vec4 pos1 = nnpos + dists.x*vec4( cos( angle_span + angle_offset ), sin( angle_span + angle_offset ), 0.0, 0.0 );
	vec4 pos2 = nnpos + dists.x*vec4( cos( 2.0*angle_span + angle_offset ), sin( 2.0*angle_span + angle_offset ), 0.0, 0.0 );
	vec4 pos3 = nnpos + dists.x*vec4( cos( 3.0*angle_span + angle_offset ), sin( 3.0*angle_span + angle_offset ), 0.0, 0.0 );
	vec4 pos4 = nnpos + dists.x*vec4( cos( 4.0*angle_span + angle_offset ), sin( 4.0*angle_span + angle_offset ), 0.0, 0.0 );

	float total_1 	= abs((dists.y==1000.0)?0.0:distance( pos1, nnpos2 )-dists.y);
	total_1 	= total_1 + abs((dists.z==1000.0)?0.0:distance( pos1, nnpos3 )-dists.z);
	total_1 	= total_1 + abs((dists.w==1000.0)?0.0:distance( pos1, nnpos4 )-dists.w);
	float final_total 	= total_1;
	final_pos 		= pos1;
	new_angle_offset 	= angle_offset - angle_span;

	float total_2 	= abs((dists.y==1000.0)?0.0:distance( pos2, nnpos2 )-dists.y);
	total_2 	= total_2 + abs((dists.z==1000.0)?0.0:distance( pos2, nnpos3 )-dists.z);
	total_2 	= total_2 + abs((dists.w==1000.0)?0.0:distance( pos2, nnpos4 )-dists.w);
	final_pos 		= (total_2<final_total)?pos2:final_pos;
	new_angle_offset 	= (total_2<final_total)?angle_offset:new_angle_offset;
	final_total 		= (total_2<final_total)?total_2:final_total;

	float total_3 	= abs((dists.y==1000.0)?0.0:distance( pos3, nnpos2 )-dists.y);
	total_3 	= total_3 + abs((dists.z==1000.0)?0.0:distance( pos3, nnpos3 )-dists.z);
	total_3 	= total_3 + abs((dists.w==1000.0)?0.0:distance( pos3, nnpos4 )-dists.w);
	final_pos 		= (total_3<final_total)?pos3:final_pos;
	new_angle_offset 	= (total_3<final_total)?(angle_offset+angle_span):new_angle_offset;
	final_total 		= (total_3<final_total)?total_3:final_total;


	float total_4  	= abs((dists.y==1000.0)?0.0:distance( pos4, nnpos2 )-dists.y);
	total_4 	= total_4 + abs((dists.z==1000.0)?0.0:distance( pos4, nnpos3 )-dists.z);
	total_4 	= total_4 + abs((dists.w==1000.0)?0.0:distance( pos4, nnpos4 )-dists.w);
	new_angle_offset 	= (total_4<final_total)?(angle_offset+2.0*angle_span):new_angle_offset;
	final_pos 		= (total_4<final_total)?pos4:final_pos;

	angle_span = angle_span * 2.0;
	angle_offset = new_angle_offset;

	
	}

	gl_FragColor = vec4(angle_offset);//final_pos;
*/
}