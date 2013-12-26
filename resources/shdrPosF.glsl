#version 120

uniform sampler2D texPos;
uniform sampler2D texVel;
uniform sampler2D texInf;



varying float age;

void main()
{
	vec4 colFac = vec4(1.0);//texture2D(spriteTex, gl_PointCoord);
  //  colFac.rgb *= texture2D( texPos, gl_TexCoord[0].st ).rgb;
	
	//    colFac.a *= .35;
	colFac.r *= (1.0-age);
	colFac.b = sin(age*10.0)*0.5 + 0.5;
	colFac.a = 0.6;
	
//	colFac.r = texture2D(texSy, gl_TexCoord[0].st).r;
    
	gl_FragColor = colFac;
}