#include "frame_uniforms.glsl"

uniform layout (binding=14) sampler3D s_ColorCorrection;

vec3 ColorCorrect(vec3 input) {
    if (IsFlagSet(FLAG_ENABLE_COLOR_CORRECTION)) {
        return texture(s_ColorCorrection, input).rgb;
    }
    else {
        return input;
    }
}