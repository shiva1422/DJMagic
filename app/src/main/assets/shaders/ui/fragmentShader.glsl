#version 310 es
precision highp float;
in vec2 uv;
out vec4 FragColor;
uniform sampler2D image;
void main()
{
    vec4 finalColor=texture(image,vec2(uv.x,1.0-uv.y));
    FragColor=finalColor;
}