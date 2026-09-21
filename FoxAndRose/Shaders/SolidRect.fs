#version 330 core
in vec2 v_UV;
in vec4 v_Color;
flat in int v_Material;
uniform sampler2D u_Texture;
uniform int u_Textured;
uniform float u_Time;
out vec4 FragColor;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p)
{
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x),
               mix(hash(i + vec2(0, 1)), hash(i + 1.0), f.x),
               f.y);
}

void main()
{
    FragColor = v_Color;
    if (u_Textured == 1)
        FragColor.a *= texture(u_Texture, v_UV).r;
    else if (u_Textured == 2)
        FragColor *= texture(u_Texture, v_UV);
    else if (v_Material > 0)
    {
        vec2 uv = v_UV;
        float grain = noise(uv * 48.0), weather = noise(uv * 2.8);
        vec3 base = v_Color.rgb;
        if (v_Material == 1)
        {
            base *= 0.78 + 0.26 * grain + 0.18 * weather;
            float joint = 1.0 - smoothstep(0.0, 0.035, min(fract(uv.x), fract(uv.y * 1.6)));
            base *= 1.0 - 0.2 * joint;
            float moss =
                smoothstep(0.58, 0.83, noise(uv * 3.2 + 7.0)) * (0.35 + 0.65 * fract(uv.y));
            base = mix(base, vec3(0.15, 0.24, 0.13), moss * 0.7);
        }
        else if (v_Material == 2)
        {
            base *= 0.83 + grain * 0.2;
            float seam = step(0.97, fract(uv.x * 2.0)) + step(0.96, fract(uv.y * 3.0));
            base *= 1.0 - 0.3 * min(seam, 1.0);
            base = mix(base, vec3(0.28, 0.14, 0.07), smoothstep(0.64, 0.85, weather) * 0.65);
            base += vec3(0.08, 0.1, 0.1) * pow(max(0.0, 1.0 - abs(fract(uv.x) - 0.4)), 12.0);
        }
        else if (v_Material == 4)
        {
            float fibres = noise(vec2(uv.x * 3.0, uv.y * 60.0));
            base *= 0.72 + fibres * 0.35 + weather * 0.12;
            base *= 1.0 - 0.25 * step(0.97, fract(uv.x * 3.0));
        }
        else if (v_Material == 5)
        {
            float weave = sin(uv.x * 170.0) * sin(uv.y * 170.0);
            base *= 0.9 + 0.055 * weave + weather * 0.1;
        }
        else if (v_Material == 6)
        {
            base = mix(base, vec3(0.34, 0.45, 0.43), smoothstep(0.35, 0.7, uv.x) * 0.35);
            float scratch = 1.0 - smoothstep(0.0, 0.012, abs(uv.y - uv.x * 0.65 - 0.2));
            base += scratch * vec3(0.25, 0.29, 0.26);
        }
        else if (v_Material == 7)
        {
            float ripple = sin(uv.x * 25.0 + uv.y * 19.0 - u_Time * 3.0);
            float shimmer = noise(uv * 14.0 + vec2(u_Time * 0.35, -u_Time * 0.2));
            base = vec3(0.09, 0.24, 0.27) + vec3(0.1, 0.2, 0.17) * (ripple * 0.5 + 0.5) * shimmer;
            base += vec3(0.28, 0.38, 0.3) * pow(max(0.0, ripple), 12.0);
        }
        else if (v_Material == 8)
        {
            float height = 1.0 - uv.y;
            float flicker = noise(vec2(uv.x * 7.0, uv.y * 5.0 + u_Time * 2.8));
            float width = (1.0 - height) * 0.43 + flicker * 0.16;
            float flame = (1.0 - smoothstep(width - 0.07, width, abs(uv.x - 0.5))) *
                          smoothstep(0.0, 0.12, uv.y) * smoothstep(0.0, 0.08, 1.0 - uv.y);
            FragColor.a *= flame;
            base = mix(vec3(1.7, 0.26, 0.025), vec3(2.3, 1.5, 0.3), (1.0 - height) * flicker);
        }
        else
        {
            base *= 0.82 + grain * 0.2 + weather * 0.14;
            float crack =
                1.0 - smoothstep(0.015, 0.045, abs(fract(uv.x + noise(uv * 4.0) * 0.12) - 0.5));
            base *= 1.0 - 0.18 * crack;
        }
        FragColor.rgb = base;
    }
}
