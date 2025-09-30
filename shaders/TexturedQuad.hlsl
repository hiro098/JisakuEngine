cbuffer CB : register(b0)
{
  float4x4 gMVP;
};
struct VSIn { float3 pos:POSITION; float2 uv:TEXCOORD0; };
struct PSIn { float4 svpos:SV_POSITION; float2 uv:TEXCOORD0; };
PSIn VSMain(VSIn i){
  PSIn o;
  o.svpos = mul(float4(i.pos,1), gMVP);
  o.uv = i.uv;
  return o;
}
Texture2D    gTex  : register(t0);
SamplerState gSamp : register(s0);
float4 PSMain(PSIn i):SV_TARGET { return gTex.Sample(gSamp, i.uv); }
float4 PSMain(PSIn i):SV_TARGET
{
    float3 rgb = gTex.Sample(gSamp, i.uv).rgb;
    float luma = dot(rgb, float3(0.299, 0.587, 0.114));
    return float4(luma.xxx, 1);
}
