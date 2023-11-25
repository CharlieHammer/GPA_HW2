#version 410

layout(location = 0) out vec4 fragColor;

uniform mat4 um4mv;
uniform mat4 um4p;
uniform bool isNormal;

in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
    vec3 normal;
} vertexData;

uniform sampler2D tex;

void main()
{
    if(isNormal){
        fragColor = vec4(vertexData.normal, 1.0);
    } else {
        vec3 texColor = texture(tex,vertexData.texcoord).rgb;
        fragColor = vec4(texColor, 1.0);
    }
}