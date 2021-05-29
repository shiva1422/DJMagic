#version 310 es
precision highp float;
layout(location=0) in vec2 verts;
layout(location=1) in vec4 color;
layout(location=2) in vec2 texCoords;
layout(location=0) uniform vec2 scale;
layout(location=2) uniform vec2 translate;
out vec2 uv;

void main()
{
    vec2 finalVerts=scale*verts+translate;
    uv=texCoords;
    gl_Position = vec4(finalVerts,0.0 ,1.0);
}