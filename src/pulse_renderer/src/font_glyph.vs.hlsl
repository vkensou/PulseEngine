#define MAX_INSTANCES 512

struct ObjectData
{
    float4x4 wMatrix;
};

struct GlyphData
{
    float4 Rect;
    float4 UVRect;
    float4 Color;
};

struct GlyphBuffer
{
    GlyphData Glyphs[MAX_INSTANCES];
};

[[vk::binding(0, 2)]]
ConstantBuffer<ObjectData> objectData : register(b0, space2);
[[vk::binding(1, 2)]]
ConstantBuffer<GlyphBuffer> glyphBuffer : register(b1, space2);

struct VSInput
{
    uint VertexID : SV_VertexID;
    uint InstanceID : SV_InstanceID;
};

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
    GlyphData glyph = glyphBuffer.Glyphs[input.InstanceID];
    float2 corner = kCorners[input.VertexID];
    float2 local = glyph.Rect.xy + corner * glyph.Rect.zw;
    float4 world = mul(objectData.wMatrix, float4(local.x, -local.y, 0.0, 1.0));
    output.Pos = mul(world, float4x4(1.0 / 320.0, 0.0, 0.0, 0.0,
                                     0.0, 1.0 / 240.0, 0.0, 0.0,
                                     0.0, 0.0, 1.0, 0.0,
                                     0.0, 0.0, 0.0, 1.0));
    output.UV = float2(lerp(glyph.UVRect.x, glyph.UVRect.z, corner.x), lerp(glyph.UVRect.y, glyph.UVRect.w, corner.y));
    output.Color = glyph.Color;
    return output;
}
