Texture2D shaderTexture : register(t0);
SamplerState sampleType : register(s0);

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 main(PS_INPUT input) : SV_TARGET
{
	return shaderTexture.Sample(sampleType, input.uv);	
}