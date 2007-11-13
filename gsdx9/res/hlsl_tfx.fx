
sampler Texture : register(s0);
sampler1D Palette : register(s1);

float4 Params0 : register(c0);

#define bTCC	(Params0[0] >= 0)
#define fRT		(Params0[1])
#define TA0		(Params0[2])
#define TA1		(Params0[3])

float4 FOGCOL : register(c1);

float2 W_H : register(c2);
float2 RW_RH : register(c3);
float2 RW_ZERO : register(c4);
float2 ZERO_RH : register(c5);

struct PS_INPUT
{
	float4 pos : POSITION;
	float4 diff : COLOR0;
	float4 fog : COLOR1;
	float3 tex : TEXCOORD0;
};

//
// texture sampling
//

float4 SampleTexture_32(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, Tex);
	c.a *= fRT;
	return c;	
}

float4 SampleTexture_24(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, Tex);
	c.a = TA0;
	// c.a *= fRT; // premultiplied
	return c;	
}

float4 SampleTexture_24AEM(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, Tex);
	c.a = any(c.rgb) ? TA0 : 0;
	// c.a *= fRT; // premultiplied
	return c;	
}

float4 SampleTexture_16(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, Tex);
	c.a = c.a != 0 ? TA1 : TA0;
	// c.a *= fRT; // premultiplied
	return c;	
}

float4 SampleTexture_16AEM(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, Tex);
	c.a = c.a != 0 ? TA1 : any(c.rgb) ? TA0 : 0;
	// c.a *= fRT; // premultiplied
	return c;
}

static const float s_palerr = 0.001/256;

float4 SampleTexture_8P_pt(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex1D(Palette, tex2D(Texture, Tex).x - s_palerr);
	// c.a *= fRT; // premultiplied
	return c;
}
	
float4 SampleTexture_8P_ln(in float2 Tex : TEXCOORD0) : COLOR
{
	Tex -= 0.5*RW_RH; // ?
	float4 c00 = tex1D(Palette, tex2D(Texture, Tex).x - s_palerr);
	float4 c01 = tex1D(Palette, tex2D(Texture, Tex + RW_ZERO).x - s_palerr);
	float4 c10 = tex1D(Palette, tex2D(Texture, Tex + ZERO_RH).x - s_palerr);
	float4 c11 = tex1D(Palette, tex2D(Texture, Tex + RW_RH).x - s_palerr);
	float2 dd = frac(Tex * W_H); 
	float4 c = lerp(lerp(c00, c01, dd.x), lerp(c10, c11, dd.x), dd.y);
	c.a *= fRT;
	return c;
}

float4 SampleTexture_8HP_pt(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex1D(Palette, tex2D(Texture, Tex).a - s_palerr);
	c.a *= fRT;
	return c;
}
	
float4 SampleTexture_8HP_ln(in float2 Tex : TEXCOORD0) : COLOR
{
	Tex -= 0.5*RW_RH; // ?
	float4 c00 = tex1D(Palette, tex2D(Texture, Tex).a - s_palerr);
	float4 c01 = tex1D(Palette, tex2D(Texture, Tex + RW_ZERO).a - s_palerr);
	float4 c10 = tex1D(Palette, tex2D(Texture, Tex + ZERO_RH).a - s_palerr);
	float4 c11 = tex1D(Palette, tex2D(Texture, Tex + RW_RH).a - s_palerr);
	float2 dd = frac(Tex * W_H); 
	float4 c = lerp(lerp(c00, c01, dd.x), lerp(c10, c11, dd.x), dd.y);
	c.a *= fRT;
	return c;
}

//
// fog
//

float4 ApplyFog(in float4 diff : COLOR, in float4 fog : COLOR) : COLOR
{
	diff = saturate(diff);
	diff.rgb = lerp(FOGCOL.rgb, diff.rgb, fog.a);
	return diff;
}

//
// tfx
//

float4 tfx0(float4 diff, float4 tex) : COLOR
{
	diff *= 2;
	diff.rgb *= tex.rgb;
	if(bTCC) diff.a *= tex.a;
	return diff;
}

float4 tfx1(float4 diff, float4 tex) : COLOR
{
	diff = tex;
	return diff;
}

float4 tfx2(float4 diff, float4 tex) : COLOR
{
	diff.rgb *= tex.rgb * 2;
	diff.rgb += diff.a;
	if(bTCC) diff.a = diff.a * 2 + tex.a;
	return diff;
}

float4 tfx3(float4 diff, float4 tex) : COLOR
{
	diff.rgb *= tex.rgb * 2;
	diff.rgb += diff.a;
	if(bTCC) diff.a = tex.a;
	return diff;
}

//
// main tfx 32
//

float4 main_tfx0_32(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx0(input.diff, SampleTexture_32(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx1_32(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx1(input.diff, SampleTexture_32(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx2_32(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx2(input.diff, SampleTexture_32(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx3_32(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx3(input.diff, SampleTexture_32(input.tex.xy / input.tex.z)), input.fog);
}

//
// main tfx 24
//

float4 main_tfx0_24(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx0(input.diff, SampleTexture_24(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx1_24(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx1(input.diff, SampleTexture_24(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx2_24(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx2(input.diff, SampleTexture_24(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx3_24(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx3(input.diff, SampleTexture_24(input.tex.xy / input.tex.z)), input.fog);
}

//
// main tfx 24 AEM
//

float4 main_tfx0_24AEM(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx0(input.diff, SampleTexture_24AEM(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx1_24AEM(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx1(input.diff, SampleTexture_24AEM(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx2_24AEM(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx2(input.diff, SampleTexture_24AEM(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx3_24AEM(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx3(input.diff, SampleTexture_24AEM(input.tex.xy / input.tex.z)), input.fog);
}

//
// main tfx 16
//

float4 main_tfx0_16(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx0(input.diff, SampleTexture_16(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx1_16(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx1(input.diff, SampleTexture_16(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx2_16(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx2(input.diff, SampleTexture_16(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx3_16(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx3(input.diff, SampleTexture_16(input.tex.xy / input.tex.z)), input.fog);
}

//
// main tfx 16 AEM
//

float4 main_tfx0_16AEM(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx0(input.diff, SampleTexture_16AEM(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx1_16AEM(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx1(input.diff, SampleTexture_16AEM(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx2_16AEM(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx2(input.diff, SampleTexture_16AEM(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx3_16AEM(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx3(input.diff, SampleTexture_16AEM(input.tex.xy / input.tex.z)), input.fog);
}

//
// main tfx 8P pt
//

float4 main_tfx0_8P_pt(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx0(input.diff, SampleTexture_8P_pt(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx1_8P_pt(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx1(input.diff, SampleTexture_8P_pt(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx2_8P_pt(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx2(input.diff, SampleTexture_8P_pt(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx3_8P_pt(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx3(input.diff, SampleTexture_8P_pt(input.tex.xy / input.tex.z)), input.fog);
}

//
// main tfx 8P ln
//

float4 main_tfx0_8P_ln(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx0(input.diff, SampleTexture_8P_ln(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx1_8P_ln(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx1(input.diff, SampleTexture_8P_ln(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx2_8P_ln(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx2(input.diff, SampleTexture_8P_ln(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx3_8P_ln(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx3(input.diff, SampleTexture_8P_ln(input.tex.xy / input.tex.z)), input.fog);
}

//
// main tfx 8HP pt
//

float4 main_tfx0_8HP_pt(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx0(input.diff, SampleTexture_8HP_pt(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx1_8HP_pt(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx1(input.diff, SampleTexture_8HP_pt(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx2_8HP_pt(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx2(input.diff, SampleTexture_8HP_pt(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx3_8HP_pt(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx3(input.diff, SampleTexture_8HP_pt(input.tex.xy / input.tex.z)), input.fog);
}

//
// main tfx 8HP ln
//

float4 main_tfx0_8HP_ln(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx0(input.diff, SampleTexture_8HP_ln(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx1_8HP_ln(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx1(input.diff, SampleTexture_8HP_ln(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx2_8HP_ln(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx2(input.diff, SampleTexture_8HP_ln(input.tex.xy / input.tex.z)), input.fog);
}

float4 main_tfx3_8HP_ln(PS_INPUT input) : COLOR
{
	return ApplyFog(tfx3(input.diff, SampleTexture_8HP_ln(input.tex.xy / input.tex.z)), input.fog);
}

//
// main notfx
//

float4 main_notfx(PS_INPUT input) : COLOR
{
	return ApplyFog(input.diff, input.fog);
}

//
// main 8P -> 32
//

float4 main_8PTo32(float4 diff : COLOR0, float2 tex : TEXCOORD0) : COLOR
{
	return tex1D(Palette, tex2D(Texture, tex).x - s_palerr);
}
