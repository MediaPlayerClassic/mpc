
sampler Texture : register(s0);
sampler1D Palette : register(s1);

float4 Params0 : register(c0);

#define TA0		(Params0[0])
#define TA1		(Params0[1])

float4 FOGCOL : register(c1);

float2 W_H : register(c2);
float2 RW_RH : register(c3);
float2 RW_ZERO : register(c4);
float2 ZERO_RH : register(c5);
float2 CLAMPMIN : register(c6);
float2 CLAMPMAX : register(c7);

struct PS_INPUT
{
	float4 pos : POSITION;
	float4 diff : COLOR0;
	float4 fog : COLOR1;
	float3 tex : TEXCOORD0;
};

#define EPSILON 0.001/256

float4 main(PS_INPUT input) : COLOR
{
	// tex
	
	float2 tex = input.tex;
	
	if(FST == 0)
	{
		tex /= input.tex.z;
	}
	
	if(CLAMP == 1)
	{
		tex = clamp(tex, CLAMPMIN, CLAMPMAX);
	}
	
	//
	
	float4 t;
	
	if(BPP == 0) // 32
	{
		t = tex2D(Texture, tex);
		if(RT == 0) t.a *= 2;
	}
	else if(BPP == 1) // 24
	{
		t = tex2D(Texture, tex);
		t.a = AEM == 0 || any(t.rgb) ? TA0 : 0;
	}
	else if(BPP == 2) // 16
	{
		t = tex2D(Texture, tex);
		t.a = t.a >= 0.5 ? TA1 : AEM == 0 || any(t.rgb) ? TA0 : 0; // a bit incompatible with up-scaling because the 1 bit alpha is interpolated
	}
	else if(BPP == 3) // 8HP ln
	{
		tex -= 0.5*RW_RH; // ?
		float4 t00 = tex1D(Palette, tex2D(Texture, tex).a - EPSILON);
		float4 t01 = tex1D(Palette, tex2D(Texture, tex + RW_ZERO).a - EPSILON);
		float4 t10 = tex1D(Palette, tex2D(Texture, tex + ZERO_RH).a - EPSILON);
		float4 t11 = tex1D(Palette, tex2D(Texture, tex + RW_RH).a - EPSILON);
		float2 dd = frac(tex * W_H); 
		t = lerp(lerp(t00, t01, dd.x), lerp(t10, t11, dd.x), dd.y);
		if(RT == 0) t.a *= 2;
	}
	
	float4 c = input.diff;
	
	if(TFX == 0)
	{
		c *= 2;
		c.rgb *= t.rgb;
		if(TCC == 1) c.a *= t.a;
	}
	else if(TFX == 1)
	{
		c = t;
	}
	else if(TFX == 2)
	{
		c.rgb *= t.rgb * 2;
		c.rgb += c.a;
		if(TCC == 1) c.a = c.a * 2 + t.a;
	}
	else if(TFX == 3)
	{
		c.rgb *= t.rgb * 2;
		c.rgb += c.a;
		if(TCC == 1) c.a = t.a;
	}
	else
	{
		c.a *= 2;
	}
	
	if(FOG)
	{
		c = saturate(c);
		c.rgb = lerp(FOGCOL.rgb, c.rgb, input.fog.a);
	}
	
	return c;
}
