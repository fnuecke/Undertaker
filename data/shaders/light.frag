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

///////////////////////////////////////////////////////////////////////////////
// Diffuse albedo material color.
uniform sampler2D DiffuseAlbedo;
// Specular albedo material color.
uniform sampler2D SpecularAlbedo;
// Emissive albedo material color.
uniform sampler2D EmissiveAlbedo;
// Pixel position in world space.
uniform sampler2D VertexPosition;
// Surface normal and specular exponent.
uniform sampler2D SurfaceNormalAndSpecularExponent;
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
	// Get local values.
	vec3 diffuseAlbedo = texture2D(DiffuseAlbedo, fs_TextureCoordinate).rgb;
	vec3 specularAlbedo = texture2D(SpecularAlbedo, fs_TextureCoordinate).rgb;
	vec3 emissiveAlbedo = texture2D(EmissiveAlbedo, fs_TextureCoordinate).rgb;
	vec3 position = texture2D(VertexPosition, fs_TextureCoordinate).xyz;
	vec4 tmp = texture2D(SurfaceNormalAndSpecularExponent, fs_TextureCoordinate);
	vec3 normal = tmp.xyz;
	float specularExponent = tmp.w;

	// Apply lighting. Punch in emissive / ambient light as base.
	Color = emissiveAlbedo;

	// Do Blinn-Phong.
	vec3 toLight = LightPosition - position;
	float distance = dot(toLight, toLight);
	vec3 toCamera = CameraPosition - position;

	// Diffuse lighting.
	float lambertTerm = max(0, dot(normalize(toLight), normal));
	Color += diffuseAlbedo * DiffuseLightColor * lambertTerm * DiffuseLightPower / distance;

	// Specular lighting.
	vec3 h = normalize(toLight + toCamera);
	float hdotn = max(0, dot(h, normal));
	float specularTerm = pow(hdotn, specularExponent);
	Color += specularAlbedo * SpecularLightColor * specularTerm * SpecularLightPower / distance;
}
///////////////////////////////////////////////////////////////////////////////
