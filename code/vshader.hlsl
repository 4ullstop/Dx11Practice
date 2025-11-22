
struct VS_INPUT
{
	float3 vPos : POSITION;
	float3 vColor : COLOR0;
	uint id : SV_InstanceID;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
};

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
       matrix mWorld; //world matrix for object
       matrix View; //view matrix
       matrix Projection; //projection matrix
};

cbuffer VoxelPositionInstanceBuffer : register(b1)
{
	float4 instancePosition;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	float4 pos = float4(input.vPos, 1.0f);

	float4 inPos = instancePosition[input.id];
	
	pos += inPos;


	pos = mul(pos, mWorld);
	pos = mul(pos, View);
	pos = mul(pos, Projection);

#if 0
	//Calculate per instance position
	int x, y, z;
	x = (input.id % (int)voxelWidth);
	y = ((input.id / voxelWidth) % voxelHeight);
	z = input.id / (voxelWidth * voxelHeight);

	float4 tempPos = 0;
	tempPos.x = ((float)x * voxelResolution) - boundingBoxExtent.x;
	tempPos.y = ((float)y * voxelResolution) - boundingBoxExtent.y;
	tempPos.z = ((float)z * voxelResolution) - boundingBoxExtent.z;

	pos += tempPos;
#endif
	output.position = pos;

	output.color = float4(input.vColor, 1.0f);

	return(output);
}