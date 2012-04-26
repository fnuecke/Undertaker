///////////////////////////////////////////////////////////////////////////////
// Deferred Shading - Geometry Shader
// 
// This shader is part of the deferred shading pipeline. It prepares data for
// the fragment shader of the geometry pass, which writes the data to the
// g-buffer, where it is used in the lighting pass to compute diffuse and
// specular light values for the single pixels.
// 
// Copyright (C) 2012 Florian Nuecke
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// We use this with OpenGL 3.3 capabilities, so require shader version 3.3
#version 330
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// The matrices used to transform vertices and normals. The world/view and
// projection matrices are constant for a single render pass, so we can
// precompute their product. The model matrix may vary for each vertex, but
// normally only for different models / vertex groups, so we include that, too.
// We still pass the model matrix separately, though, to get coordinates in
// world space (as opposed to projection space).
uniform mat4 ModelMatrix;
uniform mat4 ModelViewProjectionMatrix;
uniform mat3 NormalMatrix;
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Position of the vertex in model space.
layout(location=0) in vec4 ModelVertex;
// Normal of the vertex in model space.
layout(location=2) in vec3 ModelNormal;
// Texture coordinate for the vertex in texture space.
layout(location=8) in vec2 TextureCoordinate;
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// We pass some information on to our fragment shader, so that it can write it
// to the g-buffer.
//
// Position of the vertex in world space.
out vec3 fs_WorldVertex;
// Normal of the vertex in world space.
out vec3 fs_WorldNormal;
// Texture coordinate for the vertex in texture space.
out vec2 fs_TextureCoordinate;
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Main routing, does what a main does. Freakin' EVERYTHING!
void main(void) {
	// Transform the vertex to projection space.
	gl_Position = ModelViewProjectionMatrix * ModelVertex;

	// Transform the vertex to world space.
	fs_WorldVertex = (ModelMatrix * ModelVertex).xyz;

	// Pass on the texture coordinate.
	fs_TextureCoordinate = TextureCoordinate;

	// Transform the normal to world space, only rotation applies to normals.
	//fs_WorldNormal = NormalMatrix * ModelNormal;
	fs_WorldNormal = transpose(inverse(mat3(ModelMatrix))) * ModelNormal;
}
///////////////////////////////////////////////////////////////////////////////
