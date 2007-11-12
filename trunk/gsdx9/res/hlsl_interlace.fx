
sampler s0 : register(s0);

float4 Params1 : register(c0);

#define Height	(Params1[0])
#define Field	(Params1[1])

float4 main0(float2 tex : TEXCOORD0) : COLOR
{
	float f = fmod(tex.y * Height, 2) + 0.5;
	
	clip(f - 1);
	
	return tex2D(s0, tex);
}

float4 main1(float2 tex : TEXCOORD0) : COLOR
{
	float f = fmod(tex.y * Height, 2) + 0.5;
	
	clip(1 - f);
	
	return tex2D(s0, tex);
}

float4 main2(float2 tex : TEXCOORD0) : COLOR
{
	float f = 1 / Height;
	
	float4 c0 = tex2D(s0, tex - float2(0, f));
	float4 c1 = tex2D(s0, tex);
	float4 c2 = tex2D(s0, tex + float2(0, f));
	
	return (c0 + c1 * 2 + c2) / 4;
}

float4 main3(float2 tex : TEXCOORD0) : COLOR
{
	return tex2D(s0, tex);
}
