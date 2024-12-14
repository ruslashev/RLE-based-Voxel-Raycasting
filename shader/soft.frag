uniform sampler2D texDecal;
uniform sampler2D texDecal2;
varying vec2 texCoord;

void main(void)
{
	gl_FragColor = texture2D(texDecal, texCoord);
}
