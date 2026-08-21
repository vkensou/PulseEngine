struct PassData
{
    float4x4    vpMatrix;
}

[[vk::binding(0, 0)]]
ConstantBuffer<PassData> passData;

struct MaterialData
{
    float4      albedo;
}

[[vk::binding(0, 1)]]
ConstantBuffer<MaterialData> materialData;

struct ObjectData
{
    float4x4    wMatrix;
};

[[vk::binding(0, 2)]]
ConstantBuffer<ObjectData> objectData;

struct VSInput
{
	float3 position : POSITION;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
};

[shader("vertex")]
VSOutput vert(VSInput input)
{
	VSOutput output = (VSOutput)0;
	float3 worldPos = mul(float4(input.position, 1), objectData.wMatrix).xyz;
	output.Pos = mul(float4(worldPos, 1), passData.vpMatrix);
	return output;
}

[shader("pixel")]
float4 frag(VSOutput input) : SV_TARGET
{
	return materialData.albedo;
}