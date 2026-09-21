#version 330 core
in vec2 v_UV;
out vec4 FragColor;
uniform sampler2D u_Source;
uniform sampler2D u_Bloom;
uniform int u_Mode;
uniform vec2 u_Direction;
uniform float u_Time;

void main()
{
    vec3 color = texture(u_Source, v_UV).rgb;
    if (u_Mode == 0)
    {
        vec3 blur = color * 0.227027;
        blur += (texture(u_Source, v_UV + u_Direction * 1.384615).rgb +
                 texture(u_Source, v_UV - u_Direction * 1.384615).rgb) *
                0.316216;
        blur += (texture(u_Source, v_UV + u_Direction * 3.230769).rgb +
                 texture(u_Source, v_UV - u_Direction * 3.230769).rgb) *
                0.070270;
        FragColor = vec4(blur, 1);
    }
    else if (u_Mode == 1)
    {
        FragColor = vec4(0.018, 0.031, 0.039, clamp(color.r, 0.0, 1.0) * 0.42);
    }
    else if (u_Mode == 2)
    {
        FragColor = vec4(max(color - vec3(0.78), vec3(0)), 1);
    }
    else
    {
        vec2 px = 1.0 / vec2(textureSize(u_Source, 0));
        vec3 n = texture(u_Source, v_UV + vec2(0, px.y)).rgb;
        vec3 s = texture(u_Source, v_UV - vec2(0, px.y)).rgb;
        vec3 e = texture(u_Source, v_UV + vec2(px.x, 0)).rgb;
        vec3 w = texture(u_Source, v_UV - vec2(px.x, 0)).rgb;
        float edge = length(n - s) + length(e - w);
        color = mix(color, (n + s + e + w) * 0.25, smoothstep(0.15, 0.65, edge) * 0.22);
        color += texture(u_Bloom, v_UV).rgb * 0.3;
        color *= vec3(1.055, 1.025, 0.94);
        color = color / (1.0 + color * 0.32);
        float vignette = smoothstep(0.18, 0.85, length((v_UV - 0.5) * vec2(1.0, 0.85)));
        color *= 1.0 - 0.18 * vignette;
        float grain =
            fract(sin(dot(gl_FragCoord.xy + floor(u_Time * 12.0), vec2(12.9898, 78.233))) *
                  43758.5453) -
            0.5;
        FragColor = vec4(max(color + grain * 0.004, vec3(0)), 1);
    }
}
