#version 120

varying vec4 position;
varying vec3 normals;
uniform mat4 WorldMatrix;

void main(void) {
	// Move the normals back from the camera space to the world space
	mat3 worldRotationInverse = transpose(mat3(WorldMatrix));
	
	gl_Position		= gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0]	= gl_MultiTexCoord0;

	normals			= worldRotationInverse * gl_NormalMatrix * gl_Normal;
	position		= gl_ModelViewMatrix * gl_Vertex;
	
    gl_FrontColor = vec4(1.0, 1.0, 1.0, 1.0);
}
