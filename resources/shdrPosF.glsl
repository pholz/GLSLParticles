#version 120

uniform sampler2D texPos;
uniform sampler2D texVel;
uniform sampler2D texInf;

uniform sampler2D texSprite;

varying float age;

void main()
{
	vec4 colFac = vec4(1.0);//texture2D(spriteTex, gl_PointCoord);
  //  colFac.rgb *= texture2D( texPos, gl_TexCoord[0].st ).rgb;
	
	//    colFac.a *= .35;
	colFac.a *= age;
    
	gl_FragColor = colFac;
}