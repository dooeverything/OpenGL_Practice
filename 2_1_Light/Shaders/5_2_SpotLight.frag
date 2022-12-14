#version 330 core


struct Material
{
	sampler2D diffuse;
	sampler2D specular;
	sampler2D emission;
	float shininess;
};

struct PointLight 
{
	vec3 position;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;
};

struct SpotLight
{
	vec3 position;
	vec3 direction;

	float cutOff;
	float outerCutOff;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;
};

out vec4 fragColor;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoords;

uniform vec3 viewPos;
uniform Material material;
uniform SpotLight light;

void main()
{
	// Phong Lighting Model -> light Intensity * (specular + diffuse + ambient)

	// Caculate Ambient
	vec3  ambient = light.ambient * texture(material.diffuse, texCoords).rgb; //material.ambient;

	// Calculate Diffuse
	vec3 norm = normalize(normal);
	vec3 lightDir = normalize(light.position - fragPos);
	float diffFactor = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = light.diffuse * diffFactor * texture(material.diffuse, texCoords).rgb;

	// Calculate Specular
	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 reflectDir = 2 * dot(norm, lightDir) * norm - lightDir;
	float specFactor = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);	
	vec3 specular = light.specular * specFactor * texture(material.specular, texCoords).rgb;	

	// Calculate Emission
	vec3 emission = vec3(0.0);
	if(texture(material.specular, texCoords).rgb == vec3(0.0))
	{
		emission = texture(material.emission, texCoords).rgb;
	}

	// Calculate spotlight
	float theta = dot(lightDir, -light.direction);
	float epsilon =  light.cutOff - light.outerCutOff;
	float intensity = (theta - light.outerCutOff)/epsilon;
	intensity = clamp(intensity, 0.0, 1.0);


	diffuse *= intensity;
	specular *= intensity;

	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	ambient  *= attenuation; 
    diffuse   *= attenuation;
    specular *= attenuation;   
        

	vec3 result = ambient + diffuse + specular;
	
	fragColor = vec4(result, 1.0);
}