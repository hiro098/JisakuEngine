struct VSIn { float3 pos:POSITION; float2 uv:TEXCOORD0; };
struct PSIn { float4 svpos:SV_POSITION; float2 uv:TEXCOORD0; };
PSIn VSMain(VSIn i){
  PSIn o; o.svpos=float4(i.pos,1); o.uv=i.uv; return o;
}
Texture2D    gTex  : register(t0);
SamplerState gSamp : register(s0);
float4 PSMain(PSIn i):SV_TARGET{
  float4 texColor = gTex.Sample(gSamp, i.uv);
  if (texColor.r > 0.1 || texColor.g > 0.1) {
    return texColor;
  } else {
    return float4(1, 0, 1, 1);
  }
}
