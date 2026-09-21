#version 330 core
layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_UV;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in float a_Material;
uniform vec2 u_Viewport;
out vec2 v_UV;
out vec4 v_Color;
flat out int v_Material;

void main()
{
    gl_Position = vec4(
        a_Position.x * 2.0 / u_Viewport.x - 1.0, 1.0 - a_Position.y * 2.0 / u_Viewport.y, 0.0, 1.0);
    v_UV = a_UV;
    v_Color = a_Color;
    v_Material = int(a_Material + 0.5);
}
