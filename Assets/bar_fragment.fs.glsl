#version 410

layout(location = 0) out vec4 fragColor;

uniform vec3 barColor;

void main()
{
    fragColor = vec4(barColor, 1.0);
}