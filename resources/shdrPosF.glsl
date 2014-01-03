#version 120

uniform sampler2D texPos;
uniform sampler2D texVel;
uniform sampler2D texInf;
uniform sampler2D texSprite;

varying float doDiscard;

varying float age;

uniform float rotSpeed;

const float PI = 3.14159;

void main()
{
	
//	vec4 colFac = vec4(1.0);
	
//	if (doDiscard != 1.0)
//		discard;
	
	float phi = PI * age * rotSpeed;
	mat2 rotmat = mat2(cos(phi), sin(phi), -sin(phi), cos(phi));
	vec2 coords = rotmat * (gl_PointCoord.st - vec2(0.5)) + vec2(0.5);
	vec4 colFac = texture2D(texSprite, coords).aaaa;
	
//	vec4 colFac = vec4(gl_PointCoord.s , gl_PointCoord.t, 0.0, 1.0);
	
//	float rage = clamp(age, 0.0, 1.0);
	
	colFac.a *= 2.0 * age*(1.0-age) * doDiscard;
	colFac.g *= 1.0-age;
	colFac.r *= sin(age*12.56)*0.5 + 0.5;
	colFac.b *= sqrt(1.0-age);
//	colFac.a *= 0.6;
	
//	colFac.r = texture2D(texSy, gl_TexCoord[0].st).r;
    
	gl_FragColor = colFac;
}