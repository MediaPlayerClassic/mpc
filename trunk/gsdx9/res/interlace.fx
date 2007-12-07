
sampler s0 : register(s0);

float4 Params1 : register(c0);

#define HalfHeight	(Params1.w)
#define ZeroRHeight	(Params1.xy)

float4 main0(float2 tex : TEXCOORD0) : COLOR
{
	clip(frac(tex.y * HalfHeight) - 0.5);
	
	return tex2D(s0, tex);
}

float4 main1(float2 tex : TEXCOORD0) : COLOR
{
	clip(0.5 - frac(tex.y * HalfHeight));
	
	return tex2D(s0, tex);
}

float4 main2(float2 tex : TEXCOORD0) : COLOR
{
	float4 c0 = tex2D(s0, tex - ZeroRHeight);
	float4 c1 = tex2D(s0, tex);
	float4 c2 = tex2D(s0, tex + ZeroRHeight);
	
	return (c0 + c1 * 2 + c2) / 4;
}

float4 main3(float2 tex : TEXCOORD0) : COLOR
{
	return tex2D(s0, tex);
}
