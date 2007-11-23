sampler Texture : register(s0);

float4 main0(float2 tex : TEXCOORD0) : COLOR
{
	return tex2D(Texture, tex);
}

float4 main1(float2 tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, tex);
	c.a = c.a / 2;
	return c;
}

float4 main2() : COLOR
{
	return float4(0, 0, 0, 1);
}