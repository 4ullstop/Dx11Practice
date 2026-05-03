cbuffer SceneBuffer : register(b0)
{
	matrix orthoMatrix;
};

struct VS_INPUT
{
	float3 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output;
	output.pos = mul(float4(input.pos, 1.0f), orthoMatrix);
	output.uv = input.uv;
	return(output);
}
