#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout (std140, binding = 0) uniform Camera
{
    mat4 u_View;
    mat4 u_Projection;
    mat4 u_ViewProjection;

    vec4 u_CameraPosition;
};

layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec3 v_CameraPosition;
//layout(location = 1) out vec4 v_DirLightViewProj;

//uniform mat4 u_DirLightViewProj;

void main()
{
	v_TexCoord = a_TexCoord;
    v_CameraPosition = u_CameraPosition.xyz;

	//v_DirLightViewProj = u_DirLightViewProj * u_Model * vec4(a_Position, 1.0);
	gl_Position = vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec3 v_CameraPosition;
//layout(location = 1) in vec4 v_DirLightViewProj;

layout(location = 0) out vec4 o_FragColor;

const float PI = 3.141592653589793;
const float EPSILON = 0.000000000000001;

const int MAX_NUM_LIGHTS = 25;

struct Light
{
    vec4 u_Position;

	/*
	a: intensity
	*/
    vec4 u_Color;
    
    /* packed into a vec4
    x: range
    y: cutOffAngle
    z: outerCutOffAngle
    w: light type */
    vec4 u_AttenFactors;
    
    // Used for directional and spot lights
    vec4 u_LightDir;
};

layout (std140, binding = 1) uniform LightBuffer
{
    Light u_Lights[MAX_NUM_LIGHTS];
    uint u_NumLights;
};

uniform sampler2D u_FragColor;
uniform sampler2D u_Albedo;
uniform sampler2D u_Position;
uniform sampler2D u_Normal;
//uniform sampler2D u_DirectionalShadowMap;

// N: Normal, H: Halfway, a2: pow(roughness, 2)
float DistributionGGX(const vec3 N, const vec3 H, const float a2)
{
    float NdotH = max(dot(N, H), 0.0);
	float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// N: Normal, V: View, L: Light, k: (roughness + 1)^2 / 8.0
float GeometrySmith(const float NdotL, const float NdotV, const float k)
{
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);

    return ggx1 * ggx2;
}

vec3 Fresnel(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

/*
float CalcDirectionalShadowFactor(Light directionalLight, const float NdotL)
{
	vec3 projCoords = Input.DirLightViewProj.xyz / Input.DirLightViewProj.w;
	projCoords = (projCoords * 0.5) + 0.5;

	float currentDepth = projCoords.z;

	float bias = max(0.0008 * (1.0 - NdotL), 0.0008);

	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(u_DirectionalShadowMap, 0);
	int sampleCount = 3;
	for(int x = -sampleCount; x <= sampleCount; ++x)
	{
		for(int y = -sampleCount; y <= sampleCount; ++y)
		{
			float pcfDepth = texture(u_DirectionalShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}
    float tmp = sampleCount * 2 + 1;
	shadow /= (tmp * tmp);

	if(projCoords.z > 1.0)
        shadow = 0.0;

	return shadow;
}
*/

float LengthSq(const vec3 v)
{
	return dot(v, v);
}

void main()
{
	vec3 fragCol = texture(u_FragColor, v_TexCoord).rgb;
	vec3 albedo = texture(u_Albedo, v_TexCoord).rgb;
	vec3 fragPos = texture(u_Position, v_TexCoord).rgb;
	vec3 normal = texture(u_Normal, v_TexCoord).rgb;
	float roughness = texture(u_Albedo, v_TexCoord).a;
	float metalness = texture(u_Normal, v_TexCoord).a;

	vec3 view = normalize(v_CameraPosition - fragPos);//121
	float NdotV = max(dot(normal, view), 0.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metalness);

	float a2 = roughness * roughness;
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < u_NumLights; ++i)
    {
		Light light = u_Lights[i];
        uint type = uint(round(light.u_AttenFactors.w));

        vec3 L;
		float NdotL;
		float shadow = 1.0;
        float attenuation = 1.0;
        switch (type)
        {
			case 0:
			{
				L = -1.0 * normalize(light.u_LightDir.xyz);
				NdotL = max(dot(normal, L), 0.0);
//				shadow = (1.0 - CalcDirectionalShadowFactor(light, NdotL));
				break;
			}

			case 1:
			{
				L = normalize(light.u_Position.rgb - fragPos);
				NdotL = max(dot(normal, L), 0.0);
				vec4 attenFactor = light.u_AttenFactors;
				float lightDistance2 = LengthSq(light.u_Position.xyz - fragPos);
				float lightRadius2 = attenFactor.x * attenFactor.x;
				attenuation = clamp(1 - ((lightDistance2 * lightDistance2) / (lightRadius2 * lightRadius2)), 0.0, 1.0);
				attenuation = (attenuation * attenuation) / (lightDistance2 + 1.0);
				break;
			}
			
			case 2:
			{
				L = normalize(light.u_Position.rgb - fragPos);
				NdotL = max(dot(normal, L), 0.0);
				vec4 attenFactor = light.u_AttenFactors;
				float lightDistance2 = LengthSq(light.u_Position.xyz - fragPos);
				float lightRadius2 = attenFactor.x * attenFactor.x;
				if (lightRadius2 > lightDistance2)
				{
					float theta = dot(L, normalize(-light.u_LightDir.xyz));
					float epsilon = attenFactor.y - attenFactor.z;
					float intensity = clamp((theta - attenFactor.z) / epsilon, 0.0, 1.0);
					attenuation = intensity / lightDistance2;
				}
				else
				{
					attenuation = 0.0;
				}
				break;
			}
        }

		if (shadow <= EPSILON)
			continue;

        vec3 radiance = shadow * light.u_Color.rgb * light.u_Color.a * attenuation;

        // Cook-Torrance BRDF
        vec3 H = normalize(L + view);
        float NDF = DistributionGGX(normal, H, a2);
        float G = GeometrySmith(NdotL, clamp(NdotV, 0.0, 1.0), k);
        vec3 F = FresnelSchlickRoughness(clamp(dot(H, view), 0.0, 1.0), F0, roughness);

        vec3 numerator = NDF * G * F;
        float denom = max(4.0 * NdotV * NdotL, 0.0001);
        vec3 specular = numerator / denom;

        vec3 kD = (1.0 - F) * (1.0 - metalness);
        Lo += (kD * (albedo / PI) + specular) * radiance * NdotL;
    }

	vec3 result = fragCol + Lo;
    o_FragColor = vec4(vec3(result), 1.0);
}
