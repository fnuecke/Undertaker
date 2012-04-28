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

///////////////////////////////////////////////////////////////////////////////
uniform sampler2D GBuffer0;
uniform sampler2D GBuffer1;
uniform sampler2D GBuffer2;
///////////////////////////////////////////////////////////////////////////////
// The position of the camera.
uniform vec3 CameraPosition;
// The position of the light, in world space.
uniform vec3 LightPosition;
// The diffuse color of the light.
uniform vec3 DiffuseLightColor;
// Power of the diffuse light.
uniform float DiffuseLightPower;
// The specular color of the light.
uniform vec3 SpecularLightColor;
// Power of the specular light.
uniform float SpecularLightPower;
// Power of ambient light (which is presumed to be white).
uniform float AmbientLightPower;
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
// Main routing, does what a main does. Freakin' EVERYTHING!
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

	// Do Blinn-Phong.
	vec3 toLight = LightPosition - position;
	float distance = dot(toLight, toLight);
	if (distance < max(DiffuseLightPower * DiffuseLightPower, SpecularLightPower * SpecularLightPower)) {
		vec3 toCamera = CameraPosition - position;

		// Diffuse lighting.
		#if HALF_LAMBERT
		float lambertTerm = (dot(normalize(toLight), normal) * 0.5 + 0.5);
		lambertTerm = lambertTerm * lambertTerm;
		#else
		float lambertTerm = dot(normalize(toLight), normal);
		if (lambertTerm <= 0) {
			discard;
		}
		#endif
		Color = diffuseAlbedo * DiffuseLightColor * lambertTerm * DiffuseLightPower / distance;

		// Specular lighting.
		vec3 h = normalize(toLight + toCamera);
		float hdotn = max(0, dot(h, normal));
		float specularTerm = pow(hdotn, specularExponent);
		Color += specularIntensity * diffuseAlbedo * SpecularLightColor * specularTerm * SpecularLightPower / distance;
	} else {
		discard;
	}
}
///////////////////////////////////////////////////////////////////////////////
