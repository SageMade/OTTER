#version 430

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec3 outColor;

uniform layout(binding = 0) sampler2D s_Image;
uniform layout(binding = 1) sampler3D s_Lut;

void main() {
    outColor = texture(s_Lut, texture(s_Image, inUV).rgb).rgb;
}
