#define DEF_ARRAY 1

struct VS_INPUT
{
	float3 vPos : POSITION;
	float3 vColor : COLOR0;
	uint id : SV_InstanceID;

	float4 iPos : INSTANCEPOS;
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
#if DEF_ARRAY
	float4 instancePosition;

#else
	float voxelWidth;
	float voxelHeight;
	float voxelResolution;
	float4 boundingBoxExtent;
#endif
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	float4 pos = float4(input.vPos, 1.0f);


	pos += input.iPos;
#if 0
#if DEF_ARRAY
	float4 inPos = instancePosition[input.id];
	
	pos += inPos;



#else
	//Calculate per instance position
	float x, y, z;
#if 0
	x = (input.id % (int)voxelWidth);
	y = ((input.id / voxelWidth) % voxelHeight);
	z = input.id / (voxelWidth * voxelHeight);
#else
	x = fmod((float)input.id, voxelWidth);
	y = fmod(((float)input.id / voxelWidth), voxelHeight);
	z = input.id / (voxelWidth * voxelHeight);
#endif

	float4 tempPos = 0;
	tempPos.x = ((float)x * voxelResolution) - boundingBoxExtent.x;
	tempPos.y = ((float)y * voxelResolution) - boundingBoxExtent.y;
	tempPos.z = ((float)z * voxelResolution) - boundingBoxExtent.z;


	pos += tempPos;
#endif
#endif
	pos = mul(pos, mWorld);
	pos = mul(pos, View);
	pos = mul(pos, Projection);
	
	output.position = pos;

	output.color = float4(input.vColor, 1.0f);

	return(output);
}