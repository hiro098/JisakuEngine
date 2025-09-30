struct VSInput { 
    float3 pos : POSITION; 
    float3 col : COLOR; 
};

struct PSInput { 
    float4 pos : SV_POSITION; 
    float3 col : COLOR; 
};

PSInput VSMain(VSInput input) {
    PSInput o;
    o.pos = float4(input.pos, 1.0);
    o.col = input.col;
    return o;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return float4(input.col, 1.0);
}