uniform sampler2D texPos;
uniform sampler2D texInf;

uniform float scale;
varying float age;

void main()
{
	vec4 newVertexPos;
	vec4 dv;
	
	dv = texture2D( texPos, gl_MultiTexCoord0.st );
	
    age = texture2D(texInf, gl_MultiTexCoord0.st).r;
	
    //scale vertex position to screen size
	newVertexPos = vec4(scale * dv.x, scale * dv.y, scale * dv.z, 1);
	
    //adjust point size, increasing size kills performance
	gl_PointSize = (16.8 * age);
	
	gl_Position = gl_ModelViewProjectionMatrix * newVertexPos;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_FrontColor  = gl_Color;
}