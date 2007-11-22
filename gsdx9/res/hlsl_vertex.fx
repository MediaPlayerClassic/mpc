struct VS_INPUT
{
    float4 pos : POSITION; 
	float4 diff : COLOR0;
	float4 fog : COLOR1;
    float2 tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos : POSITION;
	float4 diff : COLOR0;
	float4 fog : COLOR1;
	float3 tex : TEXCOORD0;
};

float4 g_params[2];

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	
	// pos
		
	float4 pos = input.pos;
	pos.xy = (pos.xy - g_params[0].xy) * g_params[0].zw - 1;
	pos.y = -pos.y;
	pos.w = 1;
	output.pos = pos;
	
	// color
	
	output.diff = input.diff;
	
	// fog
	
	output.fog = input.fog;
	
	// tex
	
	output.tex.xy = input.tex * g_params[1].xy;
	output.tex.z = input.pos.w > 0 ? input.pos.w : 1;

	//

	return output;
}