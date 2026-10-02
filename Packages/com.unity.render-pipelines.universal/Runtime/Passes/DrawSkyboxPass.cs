using System;
using UnityEngine.Rendering.RenderGraphModule;
using UnityEngine.Experimental.Rendering;
using UnityEngine.Rendering.Universal.Internal;
using System.Collections.Generic;

namespace UnityEngine.Rendering.Universal
{
    /// <summary>
    /// Draw the skybox into the given color buffer using the given depth buffer for depth testing.
    ///
    /// This pass renders the standard Unity skybox.
    /// </summary>
    public partial class DrawSkyboxPass : ScriptableRenderPass
    {

/// SLZ MODIFIED 2026-10-01 - add fullscreen tri rendering path
        public bool useMotionVectorData;
        static GlobalKeyword s_DrawProcedural = GlobalKeyword.Create("DRAW_SKY_PROCEDURAL");
        static readonly int s_WorldSpaceLightPos0 = Shader.PropertyToID("_WorldSpaceLightPosSun");
        static readonly int s_LightColor0 = Shader.PropertyToID("_LightColorSun");

        struct SkyShaderInfo
        {
            public bool isLegacy;
        }

        [ThreadStatic] static Dictionary<Shader, bool> s_SkyShaderCache;

        static bool isNotLegacySky(Shader shader)
        {
            bool returnVal = false;
            s_SkyShaderCache ??= new Dictionary<Shader, bool>();
            if (s_SkyShaderCache.TryGetValue(shader, out returnVal ))
            {
                return returnVal;
            }
            else
            {
                LocalKeyword kw = shader.keywordSpace.FindKeyword("DRAW_SKY_PROCEDURAL");
                returnVal = kw.isValid;
                s_SkyShaderCache.Add(shader, returnVal);
                return returnVal;
            }
        }

        #if UNITY_EDITOR
        [OnEnteringPlayMode]
        [OnExitingPlayMode]
        static void CleanupSkyCache()
        {
            s_SkyShaderCache?.Clear();
        }
        #endif

/// END SLZ MODIFIED 2026-10-01 

        // Pre-exposes the built-in skybox shaders (see Skybox*.shader in DefaultResourcesExtra).
        private const string k_PreExposeSkyKeyword = "PRE_EXPOSE_SKY";

        /// <summary>
        /// Creates a new <c>DrawSkyboxPass</c> instance.
        /// </summary>
        /// <param name="evt">The <c>RenderPassEvent</c> to use.</param>
        /// <seealso cref="RenderPassEvent"/>
        public DrawSkyboxPass(RenderPassEvent evt)
        {
            s_SkyShaderCache = new Dictionary<Shader, bool>();
            profilingSampler = URPProfilingSamplers.DrawSkybox;
            renderPassEvent = evt;
        }

        private RendererListHandle CreateSkyBoxRendererList(RenderGraph renderGraph, UniversalCameraData cameraData)
        {
            var skyRendererListHandle = new RendererListHandle();

#if ENABLE_VR && ENABLE_XR_MODULE
            if (cameraData.xr.enabled)
            {
                // Setup Legacy XR buffer states
                if (cameraData.xr.singlePassEnabled)
                {
                    skyRendererListHandle = renderGraph.CreateSkyboxRendererList(cameraData.camera,
                        cameraData.GetProjectionMatrix(0), cameraData.GetViewMatrix(0),
                        cameraData.GetProjectionMatrix(1), cameraData.GetViewMatrix(1));
                }
                else
                {
                    skyRendererListHandle = renderGraph.CreateSkyboxRendererList(cameraData.camera, cameraData.GetProjectionMatrix(0), cameraData.GetViewMatrix(0));
                }
            }
            else
#endif
            {
                skyRendererListHandle = renderGraph.CreateSkyboxRendererList(cameraData.camera);
            }

            return skyRendererListHandle;
        }

        private static void ExecutePass(RasterCommandBuffer cmd, XRPass xr, RendererList rendererList, bool renderExposure, PassData data)
        {
/// SLZ MODIFIED 2026-10-01 - add fullscreen tri rendering path
            if (isNotLegacySky(data.material.shader))
            {
                CoreUtils.SetKeyword(cmd, k_PreExposeSkyKeyword, renderExposure);
                Light sun = RenderSettings.sun;
                Vector4 sunDir;
                Vector4 lightColor;
                if (sun && sun.isActiveAndEnabled)
                {
                    sunDir = -sun.transform.forward;
                    lightColor = (Vector4)sun.color * sun.intensity;
                }
                else 
                { 
                    sunDir = new Vector4(0,0,-1,0);
                    lightColor = Color.black;
                }
                data.material.SetVector(s_WorldSpaceLightPos0, sunDir);
                data.material.SetVector(s_LightColor0, lightColor);
                
                cmd.GetWrappedCommandBufferExt().EnableKeyword(s_DrawProcedural);
                cmd.DrawProcedural(Matrix4x4.identity, data.material, 0, MeshTopology.Triangles, 3, 1);
                cmd.GetWrappedCommandBufferExt().DisableKeyword(s_DrawProcedural);
                CoreUtils.SetKeyword(cmd, k_PreExposeSkyKeyword, false);
                return;
            }
/// END SLZ MODIFIED 2026-10-01
            CoreUtils.SetKeyword(cmd, k_PreExposeSkyKeyword, renderExposure);
#if ENABLE_VR && ENABLE_XR_MODULE
            if (xr.enabled && xr.singlePassEnabled)
                cmd.SetSinglePassStereo(SystemInfo.supportsMultiview ? SinglePassStereoMode.Multiview : SinglePassStereoMode.Instancing);
#endif
            cmd.DrawRendererList(rendererList);

#if ENABLE_VR && ENABLE_XR_MODULE
            if (xr.enabled && xr.singlePassEnabled)
                cmd.SetSinglePassStereo(SinglePassStereoMode.None);
#endif
            CoreUtils.SetKeyword(cmd, k_PreExposeSkyKeyword, false);
        }

        private class PassData
        {
            internal XRPass xr;
            internal RendererListHandle skyRendererListHandle;
            internal Material material;
            internal bool applyExposure; // Off while capturing the environment reflection from the skybox, so the capture stays un-exposed.
            /// SLZ MODIFIED - allow fullscreen tri sky shaders instead of shitty icosphere with modified camera matrix
            public bool useFullscreenTri;
        }

        private void InitPassData(ref PassData passData, in XRPass xr, in RendererListHandle handle)
        {
            passData.xr = xr;
            passData.skyRendererListHandle = handle;
        }

        /// SLZ MODIFIED - Add shading rate image
        internal void Render(RenderGraph renderGraph, ContextContainer frameData, ScriptableRenderContext context, in TextureHandle colorTarget, in TextureHandle depthTarget, in TextureHandle shadingRateTexture, Material skyboxMaterial)
        /*
        internal void Render(RenderGraph renderGraph, ContextContainer frameData, ScriptableRenderContext context, in TextureHandle colorTarget, in TextureHandle depthTarget, Material skyboxMaterial)
        */
        {
            UniversalCameraData cameraData = frameData.Get<UniversalCameraData>();
            UniversalResourceData resourceData = frameData.Get<UniversalResourceData>();

            var activeDebugHandler = GetActiveDebugHandler(cameraData);
            if (activeDebugHandler != null)
            {
                // TODO: The skybox needs to work the same as the other shaders, but until it does we'll not render it
                // when certain debug modes are active (e.g. wireframe/overdraw modes)
                if (activeDebugHandler.IsScreenClearNeeded)
                {
                    return;
                }
            }

            using (var builder = renderGraph.AddRasterRenderPass<PassData>(passName, out var passData, profilingSampler))
            {
                /// SLZ MODIFIED - Use fragment shading rate image when available
                if (shadingRateTexture.IsValid())
                {
                    builder.SetShadingRateImageAttachment(shadingRateTexture);
                    builder.SetShadingRateCombiner(ShadingRateCombinerStage.Primitive, ShadingRateCombiner.Max);
                    builder.SetShadingRateCombiner(ShadingRateCombinerStage.Fragment, ShadingRateCombiner.Max);
                }
                else
                {
                    builder.SetShadingRateCombiner(ShadingRateCombinerStage.Primitive, ShadingRateCombiner.Max);
                    builder.SetShadingRateCombiner(ShadingRateCombinerStage.Fragment, ShadingRateCombiner.Max);
                }
                /// END SLZ MODIFIED

                var skyRendererListHandle = CreateSkyBoxRendererList(renderGraph, cameraData);
                InitPassData(ref passData, cameraData.xr, skyRendererListHandle);
                passData.material = skyboxMaterial;

                passData.applyExposure = GraphicsSettings.TryGetRenderPipelineSettings<URPExposureSettings>(out var exposureSetting) && exposureSetting.UseExposure &&
                    !(cameraData.cameraType == CameraType.Reflection && cameraData.camera.reflectionProbeRendered == null);

                if (resourceData.exposureMultiplier.IsValid())
                {
                    builder.UseTexture(resourceData.exposureMultiplier, AccessFlags.Read);
                }
                builder.UseRendererList(skyRendererListHandle);
                builder.SetRenderAttachment(colorTarget, 0, AccessFlags.Write);
                builder.SetRenderAttachmentDepth(depthTarget, AccessFlags.ReadWrite);

                builder.AllowPassCulling(false);
                if (cameraData.xr.enabled)
                {
                    bool passSupportsFoveation = cameraData.xrUniversal.canFoveateIntermediatePasses || resourceData.isActiveTargetBackBuffer;
                    builder.EnableFoveatedRasterization(cameraData.xr.supportsFoveatedRendering && passSupportsFoveation);
                    // Multiview render regions are incompatible with the inner (foveal) pass in Quad View
                    if (!cameraData.xr.isQuadViewInnerPass)
                    {
                        builder.SetExtendedFeatureFlags(ExtendedFeatureFlags.MultiviewRenderRegionsCompatible);
                    }
                }

                builder.SetRenderFunc(static (PassData data, RasterGraphContext context) =>
                {
                    ExecutePass(context.cmd, data.xr, data.skyRendererListHandle, data.applyExposure, data);
                });
            }
        }
    }
}
