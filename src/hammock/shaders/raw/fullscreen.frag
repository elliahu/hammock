#version 450
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

void main(){
    color = vec4(uv.x,uv.y, 0.0, 1.0);
}