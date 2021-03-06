///////////////////////////////////////////////////////////////////////////////
// Deferred Shading - Geometry Shader
// 
// This shader is part of the deferred shading pipeline. It writes the data to
// the g-buffer, where it is used in the lighting pass to compute diffuse and
// specular light values for the single pixels.
// 
// Copyright (C) 2012 Florian Nuecke
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// We use this with OpenGL 3.3 capabilities, so require shader version 3.3
#version 330
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// The textures to use.
uniform sampler2D Textures[4];
// The number of textures to use.
uniform int TextureCount;
// The diffuse base color of the material.
uniform vec3 ColorDiffuse;
// The specular intensity of the material.
uniform float SpecularIntensity;
// The specular exponent of the material.
uniform float SpecularExponent;
// The unattenuated color amount to use.
uniform float Emissivity;
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Position of the vertex in world space.
in vec4 fs_WorldVertex;
// Normal of the vertex in world space.
in vec3 fs_WorldNormal;
// The texture coordinate on the object's surface.
in vec2 fs_TextureCoordinate;
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
out vec4 GBuffer0;
out vec4 GBuffer1;
out vec4 GBuffer2;
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Utility method to get grayscale value (brightness) of the specular color
// component of a texture.
float brightness(vec3 color) {
	return color.r * 0.2126 + color.g * 0.7152 + color.b * 0.0722;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Main routine, does what a main does. Freakin' EVERYTHING!
void main(void) {
	vec3 color = ColorDiffuse;
	if (TextureCount > 0) {
		for (int i = 0; i < TextureCount; ++i) {
			vec4 texture = texture2D(Textures[i], fs_TextureCoordinate);
			color = color * (1 - texture.a) + texture.rgb * texture.a;
		}
	}
	GBuffer0 = vec4(color, Emissivity);
	GBuffer1 = vec4(fs_WorldVertex.xyz, SpecularIntensity);
	GBuffer2 = vec4(fs_WorldNormal, SpecularExponent);
}
///////////////////////////////////////////////////////////////////////////////
