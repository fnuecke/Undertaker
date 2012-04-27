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
uniform vec3 WorldCameraPosition;
// The light for which we're now shading.
// The diffuse color (and power - can be larger than one).
uniform vec3 LightDiffuseColor;
// The specular color (and power - can be larger than one).
uniform vec3 LightSpecularColor;
// The position of the light, in world space.
uniform vec3 LightWorldPosition;
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
// Local diffuse material color.
vec3 diffuseAlbedo;
vec3 specularAlbedo;
vec3 emissiveAlbedo;
vec3 position;
vec3 normal;
float exponent;
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Computes diffuse light term.
vec3 diffuseTerm() {
	vec3 toLight = normalize(LightWorldPosition - position);
	return diffuseAlbedo * LightDiffuseColor * max(dot(normal, toLight), 0);
}
// Computes specular light term.
vec3 specularTerm() {
	return vec3(0);
}
// Reads local information from g-buffer.
void init() {
	diffuseAlbedo = texture2D(DiffuseAlbedo, fs_TextureCoordinate).rgb;
	specularAlbedo = texture2D(SpecularAlbedo, fs_TextureCoordinate).rgb;
	emissiveAlbedo = texture2D(EmissiveAlbedo, fs_TextureCoordinate).rgb;
	position = texture2D(VertexPosition, fs_TextureCoordinate).xyz;
	vec4 tmp = texture2D(SurfaceNormalAndSpecularExponent, fs_TextureCoordinate);
	normal = tmp.xyz;
	exponent = tmp.w;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Main routing, does what a main does. Freakin' EVERYTHING!
void main(void) {
	// Get local values.
	init();

	// Apply lighting.
	Color = diffuseTerm() + specularTerm() + emissiveAlbedo;
}
///////////////////////////////////////////////////////////////////////////////
