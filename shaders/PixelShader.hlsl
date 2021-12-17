#define MAX_MESH_PER_DRAW 1024
#define MAX_LIGHT_PER_DRAW 1024

struct SURFACE
{
    float3 position;
    float3 normal;
};

struct LIGHT
{
    float4 position;
    float4 direction;
    float4 attributes;
    float4 color;
};

struct ATTRIBUTES
{
	float3 Kd;			// diffuse reflectivity
	float d;			// dissolve (transparency) 
	float3 Ks;			// specular reflectivity
	float Ns;			// specular exponent
	float3 Ka;			// ambient reflectivity
	float sharpness;	// local reflection map sharpness
	float3 Tf;			// transmission filter
	float Ni;			// optical density (index of refraction)
	float3 Ke;			// emissive reflectivity
	uint illum;			// illumination model
};

struct SHADER_DATA
{
    float4 lightDirection, lightColor, lightCount;
    float4 ambient, camPos;
    matrix view, projection;
    matrix matricies[MAX_MESH_PER_DRAW];
    ATTRIBUTES materials[MAX_MESH_PER_DRAW];
    LIGHT lights[MAX_LIGHT_PER_DRAW];
};

struct PS_IN
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 nrm : NORMAL;
	float3 world_pos : WORLD;
};

StructuredBuffer<SHADER_DATA> SceneData : register(b0);
TextureCube skyboxTexture : register(t1);
SamplerState skyboxFilter : register(s1);
Texture2D colorTexture[] : register(t2);
SamplerState colorTextureFilter : register(s2);

[[vk::push_constant]]
cbuffer MESH_INDEX
{
	uint mesh_ID;
	uint material_ID;
    uint hasColorTexture;
};

float4 CalculateDirectionalLight(ATTRIBUTES mat, LIGHT light, SURFACE surface)
{
    float lightRatio = saturate(dot(-light.direction.xyz, surface.normal.xyz));
    float4 result = float4(light.color.xyz * lightRatio, 0.0f);
    return result;
}

float4 CalculatePointLight(ATTRIBUTES mat, LIGHT light, SURFACE surface)
{
    float3 toLight = float3(light.position.xyz - surface.position.xyz);
    float lightRatio = saturate(dot(normalize(toLight), surface.normal));
    float attenuation = 1.0f - saturate(length(toLight) / light.attributes.z);
    float intensity = light.attributes.w;
    float4 result = float4(light.color * lightRatio * attenuation * intensity);
    return result;
}

float4 CalculateSpotLight(ATTRIBUTES mat, LIGHT light, SURFACE surface)
{  
    float3 toLight = normalize(light.position.xyz - surface.position.xyz);
    float toLightDistance = length(light.position.xyz - surface.position.xyz);
    float spotRatio = saturate(dot(-toLight, normalize(light.direction.xyz)));
    float intensity = light.attributes.w;
    float innerConeAngle = light.attributes.x;
    float outerConeAngle = light.attributes.y;
    float lightRatio = saturate(dot(toLight, surface.normal.xyz));
    float attenuation = 1.0f - saturate(toLightDistance / light.attributes.z);
    attenuation *= (1.0f - saturate((innerConeAngle - spotRatio) / (innerConeAngle - outerConeAngle)));
    float4 result = light.color * lightRatio * attenuation * intensity;
    return result;
}

float4 CalculateSpecular(ATTRIBUTES mat, LIGHT light, SURFACE surface, float3 cameraPos)
{
	float3 toCam = normalize(cameraPos - surface.position);
    float3 toLight = normalize(light.position.xyz - surface.position.xyz);
    float attenuation = 1.0f;
    float specPower = mat.Ns;
    
    switch (int(light.position.w))
    {
        case 0:
            toLight = -normalize(light.direction.xyz);
            specPower = 256.0f;
            break;
        case 1:
            attenuation = 1.0f - saturate(length(light.position.xyz - surface.position.xyz) / light.attributes.z);
            break;
        case 2:
            float spotRatio = saturate(dot(-toLight, normalize(light.direction.xyz)));
            float innerConeAngle = light.attributes.x;
            float outerConeAngle = light.attributes.y;
            attenuation = 1.0f - saturate(length(light.position.xyz - surface.position.xyz) / light.attributes.z);
            attenuation *= (1.0f - saturate((innerConeAngle - spotRatio) / (innerConeAngle - outerConeAngle)));
            break;
        default:
            break;
    };
    
    float inLight = dot(toLight, surface.normal.xyz);
    float3 reflec = reflect(-toLight, surface.normal);
    float specIntensity = saturate(pow(dot(toCam, reflec), specPower));
    float4 spec = float4(light.color.xyz * mat.Ks * specIntensity * attenuation * inLight, 0.0f);
    return spec;
};

float4 CalculateLight(ATTRIBUTES mat, LIGHT light, SURFACE surface)
{
    float4 luminance = float4(0, 0, 0, 0);
    switch (int(light.position.w))
    {
        case 1: // point light
            luminance += CalculatePointLight(mat, light, surface);
            break;
        case 2: // spot light
            luminance += CalculateSpotLight(mat, light, surface);
            break;
        default:
            break;
    }
    return luminance;
};

float4 main(PS_IN input) : SV_TARGET
{
	ATTRIBUTES material = SceneData[0].materials[material_ID];
    
    SURFACE surface = (SURFACE) 0;
    surface.position = input.world_pos.xyz;
    surface.normal = normalize(input.nrm);
    
    LIGHT directional = (LIGHT) 0;
    directional.position = float4(0, 0, 0, 0);
    directional.direction.xyz = normalize(SceneData[0].lightDirection.xyz);
    directional.color = SceneData[0].lightColor;
    
    float3 cameraPos = SceneData[0].camPos.xyz;
	
    float4 luminance = float4(0, 0, 0, 0);
    float4 specular = float4(0, 0, 0, 0);
    luminance += CalculateDirectionalLight(material, directional, surface);
    specular += CalculateSpecular(material, directional, surface, cameraPos);
    
    int count = int(SceneData[0].lightCount.x);
    for (int i = 0; i < count; i++)
    {
        LIGHT light = SceneData[0].lights[i];
        luminance += CalculateLight(material, light, surface);
        specular += CalculateSpecular(material, light, surface, cameraPos);
    }	
    float4 diffuse = (hasColorTexture == 1) ? colorTexture[material_ID].Sample(colorTextureFilter, input.uv.xy) : float4(material.Kd, material.d);
    float4 ambient = saturate(float4(/*SceneData[0].ambient.xyz + */material.Ka.xyz, 0.0f));
    float4 emissive = float4(material.Ke, 0.0f);
    float4 result = float4(luminance.xyz + ambient.xyz, 1.0f);
    return saturate((result * diffuse) + specular + emissive);
}