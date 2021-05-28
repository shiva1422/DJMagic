#version 310 es
precision highp float;
layout( location=0 ) in vec2 verts;
layout(location=0) uniform vec2 scale;
layout(location=2) uniform vec2 translate;
void main()
{
    vec2 finalVerts=scale*verts+translate;
    gl_Position = vec4(finalVerts,0.0 ,1.0);
}