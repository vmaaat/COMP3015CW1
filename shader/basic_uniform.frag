#version 460

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D diffuseTex;

struct Light {
    vec3 position;
    vec3 color;
};

uniform Light light1;
uniform Light light2;
uniform vec3 viewPos;

vec3 CalcLight(Light light, vec3 norm, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - FragPos);

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);

    // Specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), 64.0);

    // Tuned strengths: low ambient, strong diffuse, strong specular
    float ambientStrength  = 0.05;
    float diffuseStrength  = 0.9;
    float specularStrength = 1.2;

    vec3 ambient  = ambientStrength  * light.color;
    vec3 diffuse  = diffuseStrength  * diff * light.color;
    vec3 specular = specularStrength * spec * light.color;

    return ambient + diffuse + specular;
}

void main()
{
    vec3 albedo = texture(diffuseTex, TexCoord).rgb;

    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 lighting = CalcLight(light1, norm, viewDir)
                  + CalcLight(light2, norm, viewDir);

    // Prevent things from blowing out too hard
    lighting = clamp(lighting, vec3(0.0), vec3(2.0));

    FragColor = vec4(albedo * lighting, 1.0);
}