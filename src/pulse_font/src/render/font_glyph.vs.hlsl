struct VSInput
{
    [[vk::location(0)]]
    float4 Rect : RECT0;
    [[vk::location(1)]]
    float4 UVRect : TEXCOORD0;
    [[vk::location(2)]]
    float4 Color : COLOR0;
    uint VertexID : SV_VertexID;
};

struct PushConstants
{
    float2 scale;
    float2 translate;
};

[[vk::push_constant]]
PushConstants pc;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]]
    float2 UV : TEXCOORD0;
    [[vk::location(1)]]
    float4 Color : COLOR0;
};

static const float2 kCorners[6] =
{
    float2(0.0, 0.0),
    float2(1.0, 0.0),
    float2(0.0, 1.0),
    float2(0.0, 1.0),
    float2(1.0, 0.0),
    float2(1.0, 1.0)
};

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;
    float2 corner = kCorners[input.VertexID];
    float2 local = input.Rect.xy + corner * input.Rect.zw;
    output.Pos = float4(local * pc.scale + pc.translate, 0.0, 1.0);
    output.UV = float2(lerp(input.UVRect.x, input.UVRect.z, corner.x), lerp(input.UVRect.y, input.UVRect.w, corner.y));
    output.Color = input.Color;
    return output;
}
