struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]]
    float2 UV : TEXCOORD0;
    [[vk::location(1)]]
    float4 Color : COLOR0;
};

[[vk::binding(0, 0)]]
Texture2D<float> fontTexture : register(t0);
[[vk::binding(1, 0)]]
SamplerState fontSampler : register(s0);

[shader("pixel")]
float4 main(VSOutput input) : SV_TARGET
{
    float distance = fontTexture.Sample(fontSampler, input.UV).r;
    float width = max(fwidth(distance), 1.0e-5);
    float alpha = saturate((distance - 0.5) / width + 0.5);
    return float4(input.Color.rgb, input.Color.a * alpha);
}
