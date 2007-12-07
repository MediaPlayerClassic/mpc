sampler Texture : register(s0);

float4 main0(float2 tex : TEXCOORD0) : COLOR
{
	return tex2D(Texture, tex);
}

float4 main1(float2 tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, tex);
	c.a *= 128.0f / 255; // *= 0.5f is no good here, need to do this in order to get 0x80 for 1.0f (instead of 0x7f)
	return c;
}

float4 main2() : COLOR
{
	return float4(0, 0, 0, 1);
}

float4 main3(float2 tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, tex);
	if(c.a < 1) c.a = 0;
	return c;
}
