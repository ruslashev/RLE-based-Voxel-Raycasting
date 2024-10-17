varying vec2 texCoord;

void main(void)
{
	texCoord    = gl_MultiTexCoord0.xy;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);
}
