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
// Position render target of our g-buffer.
uniform sampler2D PositionBuffer; 
// Normal render target of our g-buffer.
uniform sampler2D NormalBuffer;
// Color render target of our g-buffer.
uniform sampler2D ColorBuffer;
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
// Main routing, does what a main does. Freakin' EVERYTHING!
void main(void) {
	// Get position in the world.
	vec4 tmp = texture2D(PositionBuffer, fs_TextureCoordinate);
	vec3 position = tmp.xyz;
	float emissivity = tmp.w;

	// Get surface normal.
	vec3 normal = texture2D(NormalBuffer, fs_TextureCoordinate).xyz;

	// Get material color components.
	tmp = texture2D(ColorBuffer, fs_TextureCoordinate);
	vec3 materialDiffuse = tmp.rgb;
	vec3 materialSpecular = vec3(tmp.a);

	// Apply lighting.
	//vec3 toLight = normalize(LightWorldPosition - position);
	vec3 toLight = vec3(0, 0, 128) - position;
	float distance = length(toLight);
	toLight = normalize(toLight);
	vec3 diffuse = max(dot(normal, toLight), emissivity) * materialDiffuse * 100 / distance;

	// Write final color.
	Color = diffuse;
}
///////////////////////////////////////////////////////////////////////////////
