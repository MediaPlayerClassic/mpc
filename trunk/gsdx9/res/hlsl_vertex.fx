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

float2 g_pos_offset;
float2 g_pos_scale;
float2 g_tex_scale;

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	
	// pos
		
	float4 pos = input.pos;
	pos.xy = (pos.xy - g_pos_offset) * g_pos_scale - 1;
	pos.y = -pos.y;
	pos.w = 1;
	output.pos = pos;
	
	// color
	
	output.diff = input.diff;
	
	// fog
	
	output.fog = input.fog;
	
	// tex
	
	output.tex.xy = input.tex * g_tex_scale;
	output.tex.z = input.pos.w > 0 ? input.pos.w : 1;

	//

	return output;
}