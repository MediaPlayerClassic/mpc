
texture Texture : register(t0);

sampler Sampler : register(s0) = sampler_state
{
	texture = <Texture>;
};

float4 Params1 : register(c0); // TFX, fTCC, fRT, fTME

#define TFX		(Params1[0])
#define fTCC	(Params1[1] > 0)
#define fRT		(Params1[2] > 0)
//#define ASC		(Params1[2])
#define fTME	(Params1[3] > 0)

float4 Params2 : register(c1); // PSM (TEX0), AEM, TA0, TA1

#define PSM		(Params2[0])
#define AEM		(Params2[1] > 0)
#define TA0		(Params2[2])
#define TA1		(Params2[3])

#define PSM_PSMCT32		0
#define PSM_PSMCT24		1
#define PSM_PSMCT16		2
#define PSM_PSMCT16S	10

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

float4 SampleTexture(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 TexColor = tex2D(Sampler, Tex);
/*
	if(fRT) CorrectTexColor(TexColor);
	else TexColor.a *= 2;
*/
	if(!fRT) TexColor.a *= 2;
	//TexColor.a *= ASC;

	return TexColor;	
}

float4 ApplyFog(in float4 Diff : COLOR, in float4 Fog : COLOR) : COLOR
{
	Diff = saturate(Diff);
	Diff.rgb = lerp(Fog.rgb, Diff.rgb, Fog.a);	
	return Diff;
}

float4 main_tfx0(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	float4 TexColor = SampleTexture(Tex);

	Diff *= 2;
	Diff.rgb *= TexColor.rgb;
	if(fTCC) Diff.a *= TexColor.a;

	return ApplyFog(Diff, Fog);
}

float4 main_tfx1(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	float4 TexColor = SampleTexture(Tex);
	
	Diff = TexColor;

	return ApplyFog(Diff, Fog);
}

float4 main_tfx2(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	float4 TexColor = SampleTexture(Tex);
		
	Diff.rgb *= TexColor.rgb * 2;
	Diff.rgb += Diff.a;
	if(fTCC) Diff.a = Diff.a * 2 + TexColor.a;

	return ApplyFog(Diff, Fog);
}

float4 main_tfx3(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	float4 TexColor = SampleTexture(Tex);
		
	Diff.rgb *= TexColor.rgb * 2;
	Diff.rgb += Diff.a;
	if(fTCC) Diff.a = TexColor.a;

	return ApplyFog(Diff, Fog);
}

float4 main_tfx4(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	Diff.a *= 2;
	return ApplyFog(Diff, Fog);
}


void ApplyTFX(inout float4 Diff : COLOR, in float4 TexColor : COLOR)
{
	if(TFX == 1)
	{
		Diff = TexColor;
	}
	else
	{
		Diff.rgb *= TexColor.rgb * 2;
		
		if(TFX == 0)
		{
			if(fTCC) Diff.a *= TexColor.a;
		}
		else
		{
			Diff.rgb += Diff.a;

			if(fTCC)
			{
				TexColor.a /= 2;

				if(TFX == 2)
				{
					Diff.a += TexColor.a;
				}
				else if(TFX == 3)
				{
					Diff.a = TexColor.a;
				}
			}
		}
	}
}

float4 main_tfx(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	if(fTME) ApplyTFX(Diff, SampleTexture(Tex));

	return ApplyFog(Diff, Fog);
}

