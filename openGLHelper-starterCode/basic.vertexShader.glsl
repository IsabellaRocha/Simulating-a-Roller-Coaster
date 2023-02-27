#version 150

in vec3 position;
in vec4 color;
out vec4 col;

uniform int mode;
uniform int constant;
in vec3 upCoors;
in vec3 downCoors;
in vec3 rightCoors;
in vec3 leftCoors;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main()
{
	if (mode == 0) {
		// compute the transformed and projected vertex position (into gl_Position) 
		// compute the vertex color (into col)
		gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
		col = color;
	}
	else if (mode == 1) {
		float eps = 0.00001;
		float smoothenedY = (upCoors.y + downCoors.y + rightCoors.y + leftCoors.y) / 4.0f;
		float smoothenedX = (upCoors.x + downCoors.x + rightCoors.x + leftCoors.x) / 4.0f;
		float smoothenedZ = (upCoors.z + downCoors.z + rightCoors.z + leftCoors.z) / 4.0f;
		vec4 outputColor = max(color, vec4(eps)) / max(position.y, eps * constant) * smoothenedY;
		gl_Position = projectionMatrix * modelViewMatrix * vec4(smoothenedX, smoothenedY, smoothenedZ, 1.0f);
		col = outputColor;
	}
  
}

