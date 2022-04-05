#version 440

layout (points) in;
layout (points) out;
layout (max_vertices = 32) out;

// Define inputs to match the vertex shader
layout (location = 0) in flat uint inType[];
layout (location = 1) in flat uint inTexID[];
layout (location = 2) in vec3 inPosition[];
layout (location = 3) in vec3 inVelocity[];
layout (location = 4) in vec4 inColor[];
layout (location = 5) in float inLifetime[];
layout (location = 6) in vec4 inMetadata[];
layout (location = 7) in vec4 inMetadata2[];

// Our per-vertex outputs
out flat uint out_Type;
out flat uint out_TexID;
out vec3 out_Position;
out vec3 out_Velocity;
out vec4 out_Color;
out float out_Lifetime;
out vec4 out_Metadata;
out vec4 out_Metadata2;

#include "../fragments/frame_uniforms.glsl"

// Uniforms
uniform vec3  u_Gravity;

uniform mat4 u_ModelMatrix;

#define TYPE_EMITTER 0
#define TYPE_PARTICLE 1

// See https://thebookofshaders.com/10/
// Returns a random number between 0 and 1
float rand(vec2 seed) {
    return fract(sin(dot(seed, vec2(12.9898, 78.233) * 43758.5453123)));
}

void main() {
    float lifetime = inLifetime[0] - u_DeltaTime;
    vec4 meta = inMetadata[0];

    switch (inType[0]) {
        // Handling emitters
        case TYPE_EMITTER:
            int emitted = 1;

            float startLife = lifetime;
            int toEmit = 0;
            
            while ((lifetime < 0) && (emitted < 32)) {
                lifetime += meta.x;
                toEmit ++;
                emitted++;
            }

            // Push the emitter back into the output stream
            out_Type     = TYPE_EMITTER;
            out_TexID    = inTexID[0];
            out_Position = inPosition[0];
            out_Velocity = inVelocity[0];
            out_Color    = inColor[0];
            out_Lifetime = lifetime;
            out_Metadata = inMetadata[0];
            out_Metadata2 = inMetadata2[0];
            
            EmitVertex();
            EndPrimitive();

            // If the lifetime is at 0, we emit a particle
            for (int ix = 0; ix < toEmit; ix++) {
                float timeAdjust = (-startLife + (ix * meta.x));
                out_Type = TYPE_PARTICLE;
                out_TexID = inTexID[0];
                out_Position = (u_ModelMatrix * vec4(inPosition[0] + inVelocity[0] * timeAdjust, 1.0f)).xyz;
                out_Velocity = mat3(u_ModelMatrix) * inVelocity[0];
                out_Lifetime = meta.z + (meta.w - meta.z) * rand(vec2(inPosition[0].x, u_DeltaTime));
                out_Metadata = vec4(out_Lifetime, meta.y, 0, 0);
                out_Metadata2 = vec4(0);
                out_Color    = inColor[0];
                
                EmitVertex();
                EndPrimitive();
            }

            return;

        // Handling particles
        case TYPE_PARTICLE:
            if (lifetime > 0) {
                out_Type = TYPE_PARTICLE;
                out_TexID = inTexID[0];

                // Update position and apply forces
                out_Position = inPosition[0] + inVelocity[0] * u_DeltaTime;
                out_Velocity = inVelocity[0] + (u_Gravity * u_DeltaTime);
                                
                // Update lifetime
                out_Lifetime = lifetime;

                // For now, just pass through metadata and color
                out_Metadata = inMetadata[0];
                out_Color    = vec4(inColor[0].rgb, (lifetime / meta.x));
                out_Metadata = inMetadata[0];
                out_Metadata2 = inMetadata2[0];

                // Emit into vertex stream
                EmitVertex();
                EndPrimitive();
            }
            return;
        // Anything else, for debug purposes
        default:
            return;
    }
}