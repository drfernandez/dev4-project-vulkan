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
    float3 Kd; // diffuse reflectivity
    float d; // dissolve (transparency) 
    float3 Ks; // specular reflectivity
    float Ns; // specular exponent
    float3 Ka; // ambient reflectivity
    float sharpness; // local reflection map sharpness
    float3 Tf; // transmission filter
    float Ni; // optical density (index of refraction)
    float3 Ke; // emissive reflectivity
    uint illum; // illumination model
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
    float3 uv : TEXCOORD;
    float3 nrm : NORMAL;
    float3 world_pos : WORLD;
};

StructuredBuffer<SHADER_DATA> SceneData : register(b0);
TextureCube skybox : register(t1);
SamplerState filter : register(s1);

[[vk::push_constant]]
cbuffer MESH_INDEX
{
    uint mesh_ID;
    uint material_ID;
    uint hasColorTexture;
};

float4 main(PS_IN input) : SV_TARGET
{
    return skybox.Sample(filter, input.uv);
}