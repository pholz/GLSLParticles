#version 120

uniform sampler2D texPos;
uniform sampler2D texVel;
uniform sampler2D texInf;
uniform sampler2D texSprite;



varying float age;

void main()
{
//	vec4 colFac = vec4(1.0);
	
	vec4 colFac = texture2D(texSprite, gl_PointCoord).aaaa;
	
//	vec4 colFac = vec4(gl_PointCoord.s , gl_PointCoord.t, 0.0, 1.0);
	
	
	
	colFac.a *= (1.0-age);
	colFac.g *= 1.0-age;
	colFac.r *= sin(age*10.0)*0.5 + 0.5;
	colFac.b *= sqrt(1.0-age);
//	colFac.a *= 0.6;
	
//	colFac.r = texture2D(texSy, gl_TexCoord[0].st).r;
    
	gl_FragColor = colFac;
}