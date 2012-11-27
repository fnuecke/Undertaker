///////////////////////////////////////////////////////////////////////////////
// Deferred Shading - Light Shader
// 
// This shader is part of the deferred shading pipeline. It reads the data from
// the g-buffer, and uses it to compute diffuse and specular light values for
// the single pixels.
// 
// Copyright (C) 2012 Florian Nuecke
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// We use this with OpenGL 3.3 capabilities, so require shader version 3.3
#version 330
///////////////////////////////////////////////////////////////////////////////

#define HALF_LAMBERT 0
#define MAX_LIGHTS_PER_TILE 16

///////////////////////////////////////////////////////////////////////////////
uniform sampler2D GBuffer0;
uniform sampler2D GBuffer1;
uniform sampler2D GBuffer2;
///////////////////////////////////////////////////////////////////////////////
// The position of the camera.
uniform vec3 CameraPosition;
// The number of lights we actually have to process.
uniform int LightCount;
// The position of the light, in world space.
uniform vec3 LightPosition[MAX_LIGHTS_PER_TILE];
// The diffuse color of the light.
uniform vec3 DiffuseLightColor[MAX_LIGHTS_PER_TILE];
// Power of the diffuse light.
uniform float DiffuseLightRange[MAX_LIGHTS_PER_TILE];
// The specular color of the light.
uniform vec3 SpecularLightColor[MAX_LIGHTS_PER_TILE];
// Power of the specular light.
uniform float SpecularLightRange[MAX_LIGHTS_PER_TILE];
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// We get the texture coordinate on our g-buffer from the vertex shader.
in vec2 fs_TextureCoordinate;
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// We return the final color after lighting computation.
out vec3 Color;
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Main routine, does what a main does. Freakin' EVERYTHING!
void main(void) {
	vec4 tmp;
	// Get local values.
	tmp = texture2D(GBuffer0, fs_TextureCoordinate);
	vec3 diffuseAlbedo = tmp.rgb;
	tmp = texture2D(GBuffer1, fs_TextureCoordinate);
	vec3 position = tmp.xyz;
	float specularIntensity = tmp.w;
	tmp = texture2D(GBuffer2, fs_TextureCoordinate);
	vec3 normal = tmp.xyz;
	float specularExponent = tmp.w;

	//Color = vec3(0.05, 0, 0.05) * LightCount;
	Color = vec3(0, 0, 0);
	for (int i = 0; i < LightCount; ++i) {
		// Do Blinn-Phong.
		vec3 toLight = LightPosition[i] - position;
		float distance = dot(toLight, toLight);
		vec3 toCamera = CameraPosition - position;

		// Diffuse lighting.
		#if HALF_LAMBERT
		float lambertTerm = (dot(normalize(toLight), normal) * 0.5 + 0.5);
		lambertTerm = lambertTerm * lambertTerm;
		#else
		float lambertTerm = max(0, dot(normalize(toLight), normal));
		#endif
		float mult = distance / (DiffuseLightRange[i] * DiffuseLightRange[i]);
		mult = 1 - clamp(mult * mult, 0, 1);
		Color += diffuseAlbedo * DiffuseLightColor[i] * lambertTerm * mult;

		// Specular lighting.
		vec3 h = normalize(toLight + toCamera);
		float hdotn = max(0, dot(h, normal));
		float specularTerm = pow(hdotn, specularExponent);
		mult = distance / (SpecularLightRange[i] * SpecularLightRange[i]);
		mult = 1 - clamp(mult * mult, 0, 1);
		Color += specularIntensity * diffuseAlbedo * SpecularLightColor[i] * specularTerm * mult;
	}
}
///////////////////////////////////////////////////////////////////////////////
