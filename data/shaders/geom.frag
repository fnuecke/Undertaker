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
// The textures of the object we're shading.
uniform sampler2D Textures;
// The number of textures to use.
uniform int TextureCount;
// The diffuse color of the object.
uniform vec3 ColorDiffuse;
// The specular color of the object.
uniform vec3 ColorSpecular;
// How much light the texture emits.
uniform float ColorEmissivity;
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Position of the vertex in world space.
in vec3 fs_WorldVertex;
// Normal of the vertex in world space.
in vec3 fs_WorldNormal;
// The texture coordinate on the object's surface.
in vec2 fs_TextureCoordinate;
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// We return the vertex position in world space.
out vec4 Vertex;
// And the normal direction in world space.
out vec3 Normal;
// We store the diffuse and specular color in a single vec4 by making the
// specular monochromatic and storing it in the 4th component.
out vec4 Color;
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Utility method to get grayscale value (brightness) of the specular color
// component of a texture.
float brightness(vec3 color) {
	return color.r * 0.2126 + color.g * 0.7152 + color.b * 0.0722;
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Main routing, does what a main does. Freakin' EVERYTHING!
void main(void) {
	if (TextureCount > 0) {
		vec4 textureColor = texture2D(Textures, fs_TextureCoordinate);
		Color = vec4(textureColor.rgb * ColorDiffuse, brightness(textureColor.rgb * ColorSpecular));
		Color = textureColor;
	} else {
		Color = vec4(ColorDiffuse.rgb, brightness(ColorSpecular));
	}
	Vertex = vec4(fs_WorldVertex.xyz, ColorEmissivity);
	Normal = fs_WorldNormal;
}
///////////////////////////////////////////////////////////////////////////////
