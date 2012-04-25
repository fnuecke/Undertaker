#version 150

uniform sampler2D tDiffuse;

varying vec4 position;
varying vec3 normal;

void main(void) {
	gl_FragData[0] = vec4(texture2D(tDiffuse, gl_TexCoord[0].st).rgb, 0);
	gl_FragData[1] = vec4(position.xyz, 1);
	gl_FragData[2] = vec4(normal.xyz, 0);
}
