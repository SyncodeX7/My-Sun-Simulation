// Fullscreen triangle vertex shader — no vertex buffer needed.
// Generates 3 vertices that cover the entire screen.
#version 450

layout(location = 0) out vec2 fragUV;

void main() {
    // gl_VertexIndex: 0, 1, 2 → large triangle covering clip space
    fragUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(fragUV * 2.0 - 1.0, 0.0, 1.0);
}
