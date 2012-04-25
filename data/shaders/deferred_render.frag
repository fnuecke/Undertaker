#version 150

uniform sampler2D tDiffuse; 
uniform sampler2D tPosition;
uniform sampler2D tNormals;

uniform vec3 cameraPosition;

void main(void) {
	vec4 image = texture2D(tDiffuse, gl_TexCoord[0].xy);
	vec4 position = texture2D(tPosition, gl_TexCoord[0].xy) + vec4(cameraPosition, 0);
	vec4 normal = normalize(texture2D(tNormals, gl_TexCoord[0].xy));
	
	vec3 light = vec3(0, 0, 100);
	vec3 toLight = light - position.xyz;
	float brightness = 5000 / (toLight.x * toLight.x + toLight.y * toLight.y + toLight.z * toLight.z);
	toLight = normalize(toLight);
	
	
	vec3 diffuse = max(dot(normal.xyz, toLight), 0) * image.rgb * brightness;
	
	//vec3 toEye = normalize(cameraPosition - position.xyz);
	//vec3 vHalfVector = normalize(toLight + toEye);
	//vec3 specular = pow(max(dot(normal.xyz, vHalfVector), 0.0), 100) * 1.5;
	gl_FragColor = vec4(diffuse, 1);
}
