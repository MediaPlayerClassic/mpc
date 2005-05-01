
sampler Texture : register(s0);
sampler1D Palette : register(s1);

float4 Params1 : register(c0); // TFX, fTCC, fRT, fTME

#define TFX		(Params1[0])
#define fTCC	(Params1[1] != 0)
#define fRT		(Params1[2] != 0)
//#define ASC		(Params1[2])
#define fTME	(Params1[3] != 0)

float4 Params2 : register(c1); // PSM (TEX0), AEM, TA0, TA1

#define PSM		(Params2[0])
#define AEM		(Params2[1] != 0)
#define TA0		(Params2[2])
#define TA1		(Params2[3])

float2 W_H : register(c2);
float2 RW_RH : register(c3);
float2 RW_ZERO : register(c4);
float2 ZERO_RH : register(c5);

#define PSM_PSMCT32		0
#define PSM_PSMCT24		1
#define PSM_PSMCT16		2
#define PSM_PSMCT16S	10
/*
void CorrectTexColor(inout float4 TexColor : COLOR)
{
	if(PSM != PSM_PSMCT32)
	{
		float A = Params2[1] * any(TexColor.rgb) * TA0;
		// float A = AEM && !any(TexColor.rgb) ? 0 : TA0;
		
		if(PSM == PSM_PSMCT24)
		{
			TexColor.a = A;
		}
		else if(PSM == PSM_PSMCT16 || PSM == PSM_PSMCT16S)
		{
			TexColor.a = (TexColor.a < 1.0) ? A : TA1; // < 0.5 ?
		}
	}
}
*/

float4 SampleTexture(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 TexColor = tex2D(Texture, Tex);
	if(!fRT) TexColor.a *= 2; /*TexColor.a = saturate(TexColor.a*2);*/
	// TexColor.a *= ASC;
	return TexColor;	
}

static const float s_palerr = 0.001/256;

float4 SampleTexture_pal_ln(in float2 Tex : TEXCOORD0) : COLOR
{
	Tex -= 0.5*RW_RH;
	float4 c00 = tex1D(Palette, tex2D(Texture, Tex).x - s_palerr);
	float4 c01 = tex1D(Palette, tex2D(Texture, Tex + RW_ZERO).x - s_palerr);
	float4 c10 = tex1D(Palette, tex2D(Texture, Tex + ZERO_RH).x - s_palerr);
	float4 c11 = tex1D(Palette, tex2D(Texture, Tex + RW_RH).x - s_palerr);
	float2 dd = frac(Tex * W_H); 
	float4 TexColor = lerp(lerp(c00, c01, dd.x), lerp(c10, c11, dd.x), dd.y);
	if(!fRT) TexColor.a *= 2;
	return TexColor;
}

float4 SampleTexture_pal_pt(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 TexColor = tex1D(Palette, tex2D(Texture, Tex).x - s_palerr);
	if(!fRT) TexColor.a *= 2;
	return TexColor;
}
	
//
	
float4 ApplyFog(in float4 Diff : COLOR, in float4 Fog : COLOR) : COLOR
{
	Diff = saturate(Diff);
	Diff.rgb = lerp(Fog.rgb, Diff.rgb, Fog.a);	
	return Diff;
}

//

float4 tfx0(float4 Diff, float4 TexColor) : COLOR
{
	Diff *= 2;
	Diff.rgb *= TexColor.rgb;
	if(fTCC) Diff.a *= TexColor.a;
	return Diff;
}

float4 tfx1(float4 Diff, float4 TexColor) : COLOR
{
	Diff = TexColor;
	if(!fTCC) Diff.a = 1;
	return Diff;
}

float4 tfx2(float4 Diff, float4 TexColor) : COLOR
{
	Diff.rgb *= TexColor.rgb * 2;
	Diff.rgb += Diff.a;
	if(fTCC) Diff.a = Diff.a * 2 + TexColor.a;
	return Diff;
}

float4 tfx3(float4 Diff, float4 TexColor) : COLOR
{
	Diff.rgb *= TexColor.rgb * 2;
	Diff.rgb += Diff.a;
	if(fTCC) Diff.a = TexColor.a;
	return Diff;
}

// no texture

float4 tfx4(float4 Diff) : COLOR
{
	Diff.a *= 2;
	return Diff;
}

//

float4 main_tfx0(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx0(Diff, SampleTexture(Tex)), Fog);
}

float4 main_tfx1(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx1(Diff, SampleTexture(Tex)), Fog);
}

float4 main_tfx2(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx2(Diff, SampleTexture(Tex)), Fog);
}

float4 main_tfx3(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx3(Diff, SampleTexture(Tex)), Fog);
}

float4 main_tfx4(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx4(Diff), Fog);
}

//

float4 main_tfx0_pal_pt(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx0(Diff, SampleTexture_pal_pt(Tex)), Fog);
}

float4 main_tfx1_pal_pt(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx1(Diff, SampleTexture_pal_pt(Tex)), Fog);
}

float4 main_tfx2_pal_pt(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx2(Diff, SampleTexture_pal_pt(Tex)), Fog);
}

float4 main_tfx3_pal_pt(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx3(Diff, SampleTexture_pal_pt(Tex)), Fog);
}

float4 main_tfx4_pal_pt(float4 Diff : COLOR0, float2 Tex : TEXCOORD0) : COLOR
{
	return tex1D(Palette, tex2D(Texture, Tex).x - s_palerr);
}

//

float4 main_tfx0_pal_ln(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx0(Diff, SampleTexture_pal_ln(Tex)), Fog);
}

float4 main_tfx1_pal_ln(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx1(Diff, SampleTexture_pal_ln(Tex)), Fog);
}

float4 main_tfx2_pal_ln(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx2(Diff, SampleTexture_pal_ln(Tex)), Fog);
}

float4 main_tfx3_pal_ln(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx3(Diff, SampleTexture_pal_ln(Tex)), Fog);
}

//
/*
void ApplyTFX(inout float4 Diff : COLOR, in float4 TexColor : COLOR)
{
	if(TFX == 0)
	{
		Diff *= 2;
		Diff.rgb *= TexColor.rgb;
		if(fTCC) Diff.a *= TexColor.a;
	}
	else if(TFX == 1)
	{
		Diff = TexColor;
	}
	else if(TFX == 2)
	{
		Diff.rgb *= TexColor.rgb * 2;
		Diff.rgb += Diff.a;
		if(fTCC) Diff.a = Diff.a * 2 + TexColor.a;
	}
	else if(TFX == 3)
	{
		Diff.rgb *= TexColor.rgb * 2;
		Diff.rgb += Diff.a;
		if(fTCC) Diff.a = TexColor.a;
	}
}

float4 main_tfx(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	if(fTME) ApplyTFX(Diff, SampleTexture(Tex));

	return ApplyFog(Diff, Fog);
}
*/
