#if !defined(FLUORESCENT_LIGHTING_MODEL_INCLUDED)
#define FLUORESCENT_LIGHTING_MODEL_INCLUDED
namespace SLZ
{

    half3 Fluorescence(half4 diffuseLight, half4 absorbance, half3 emissionColor ){
       
        half4 absorbedLight = diffuseLight * absorbance;					

        //Combine each color from high to low frequency to account for dual-excitation
        half absorbedB = absorbedLight.b + absorbedLight.a;
        half absorbedG = absorbedB + absorbedLight.g;
        half absorbedR = absorbedG + absorbedLight.r;

        half3 reemittedLight = half3(Absorbed_R, Absorbed_G, Absorbed_B) * emissionColor.rgb ;
        return reemittedLight;					
    }

    class DiffuseModelLambertFluorescent
    {
        half4 absorbance;
        half4 fluorescence;

        static LIGHT_VEC PunctualDiffuse(LightMeshData md, LightPhysData ps, Light light)
        {
            LIGHT_VEC diffuseLight = DiffuseModelLambert.PunctualDiffuse(md, ps, light);

        }

        static half3 ShDiffuse(LightMeshData md, LightPhysData ps, ShCoefficients sh)
        {
            half3 result = half3(
                            dot(md.normal, sh.L1L0r.rgb) + sh.L1L0r.a,
                            dot(md.normal, sh.L1L0g.rgb) + sh.L1L0g.a,
                            dot(md.normal, sh.L1L0b.rgb) + sh.L1L0b.a
                        );
            #if !defined(PROBE_VOLUMES_L1) && !defined(EVALUATE_SH_MIXED)
                result += SHEvalLinearL2(md.normal, sh.L2Br, sh.L2Bg, sh.L2Bb, half4(sh.L2C, 0.0));
            #endif

            return max(0, result);
        }

        static half3 LightmapDiffuse(LightMeshData md, LightPhysData ps, half3 lightmapTexel, half4 dirLmTexel)
        {
            half3 result = (half3)0;
            #if defined(LIGHTMAP_ON) || defined(DIRLIGHTMAP_COMBINED)
            result = lightmapTexel;
            #endif

            #if defined(DIRLIGHTMAP_COMBINED)
            result = ApplyDirLightmap(md.normal, result, dirLmTexel);
            #endif

            return result;
        }
    }
}
#endif
