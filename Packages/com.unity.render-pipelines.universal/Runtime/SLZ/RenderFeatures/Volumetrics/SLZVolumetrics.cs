using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using UnityEngine.Rendering.RenderGraphModule;
using UnityEditor;
using Unity.Mathematics;
using System.Runtime.InteropServices;
using Unity.Collections;
using System.Reflection;
using UnityEngine.UI;

namespace SLZRendering.Runtime
{
    public class SLZVolumetrics : ScriptableRendererFeature
    {
        [System.Serializable]
        public class VolumetricData
        {
            [Header("Volumetric camera settings")]
            [Tooltip("Near Clip plane")]
            public float near = 0.01f;
            [Tooltip("Far Clip plane")]
            public float far = 80f;
            [Tooltip("Resolution")]
            public int FroxelWidthResolution = 32;
            [Tooltip("Resolution")]
            public int FroxelHeightResolution = 32;
            [Tooltip("Resolution")]
            public int FroxelDepthResolution = 24;
            //[Tooltip("Controls the bias of the froxel dispution. A value of 1 is linear. ")]
            //public float FroxelDispution;

            [Header("Prebaked clipmap settings - Controls both cascades")]
            [Tooltip("Textile resolution per unit")]
            public int ClipMapResolution = 64;
            [Tooltip("Size of inner clipmap in units. Outter clipmap is 5x the size")]
            public float ClipmapScale = 20;
            public float ClipmapScale2 = 200;
            [Tooltip("Distance (m) from previous sampling point to trigger resampling clipmap")]
            public float ClipmapResampleThreshold = 3;
        }

        static void ComputeOverlappingVolumes(Vector3 cameraPos, float farClipSize)
        {

        }

        public const string resultTextureName = "_VolumetricResult";
        public const string shaderCBName = "VolumetricsCB";
        public const string volumetricKWName = "_VOLUMETRICS_ENABLED";

        [Serializable]
        public class VolumetricsRenderFeatureSettings
        {
            [SerializeField] internal ComputeShader FroxelFogCompute;
            [SerializeField] internal ComputeShader FroxelIntegrationCompute;
            [SerializeField] internal ComputeShader FroxelLocalFogCompute;
            [SerializeField] internal ComputeShader ClipmapCompute;
            [SerializeField] internal ComputeShader BlurCompute;
        }

        [SerializeField] internal VolumetricsRenderFeatureSettings settings;
        [SerializeField] public VolumetricData defaultVolumetricData;

        VolumetricsRenderPass pass;

        public override void Create()
        {
            pass = new VolumetricsRenderPass(settings, defaultVolumetricData);
            pass.renderPassEvent = RenderPassEvent.BeforeRenderingPrePasses;
        }

        public override void AddRenderPasses(ScriptableRenderer renderer, ref RenderingData renderingData)
        {
            if (renderingData.postProcessingEnabled)
            {
                renderer.EnqueuePass(pass);
            }
        }

        public class VolumetricsRenderPass : ScriptableRenderPass, IDisposable
        {
            List<TextureHandle> volumeHandles = new List<TextureHandle>();


            [StructLayout(LayoutKind.Sequential)]
            struct VolumeInfo
            {
                public Vector4 position;
                public Vector4 bounds_intensity;
                public Vector4 resolution_intensity;
            }

            static int ID_VolumetricResult = Shader.PropertyToID(resultTextureName);
            static int ID_Result = Shader.PropertyToID("Result");
            static int ID_InLightingTexture = Shader.PropertyToID("InLightingTexture");
            static int ID_InTex = Shader.PropertyToID("InTex");
            static int ID_LightProjectionTextureArray = Shader.PropertyToID("LightProjectionTextureArray");
            static int ID_VolumetricClipmapTexture = Shader.PropertyToID("_VolumetricClipmapTexture");
            static int ID_VolumetricClipmapTexture2 = Shader.PropertyToID("_VolumetricClipmapTexture2");
            static int ID_PreResult = Shader.PropertyToID("PreResult");
            static int ID_VolumeMap = Shader.PropertyToID("VolumeMap");
            static int ID_PreviousFrameLighting = Shader.PropertyToID("PreviousFrameLighting");
            static int ID_HistoryBuffer = Shader.PropertyToID("HistoryBuffer");
            static int ID_LeftEyeMatrix = Shader.PropertyToID("LeftEyeMatrix");
            static int ID_RightEyeMatrix = Shader.PropertyToID("RightEyeMatrix");
            static int ID_ClipmapScale0 = Shader.PropertyToID("ClipmapScale");
            static int ID_ClipmapScale1 = Shader.PropertyToID("_ClipmapScale");
            static int ID_ClipmapScale2 = Shader.PropertyToID("_ClipmapScale2");
            static int ID_ClipmapWorldPosition = Shader.PropertyToID("ClipmapWorldPosition");
            static int ID_VBufferUnitDepthTexelSpacing = Shader.PropertyToID("_VBufferUnitDepthTexelSpacing");
            static int ID_VolZBufferParams = Shader.PropertyToID("_VolZBufferParams");
            static int ID_GlobalExtinction = Shader.PropertyToID("_GlobalExtinction");
            static int ID_StaticLightMultiplier = Shader.PropertyToID("_StaticLightMultiplier");
            static int ID_GlobalScattering = Shader.PropertyToID("_GlobalScattering");
            static int ID_VolumeWorldSize = Shader.PropertyToID("VolumeWorldSize");
            static int ID_VolumeWorldPosition = Shader.PropertyToID("VolumeWorldPosition");
            static int ID_media_sphere_buffer_length = Shader.PropertyToID("media_sphere_buffer_length");
            static int ID_media_sphere_buffer = Shader.PropertyToID("media_sphere_buffer");
            static int ID_PerFrameConstBuffer = Shader.PropertyToID("PerFrameCB");
            static int ID_PreviousFrameMatrix = Shader.PropertyToID("PreviousFrameMatrix");
            static int ID_ClipmapScale = Shader.PropertyToID("_ClipmapScale");
            static int ID_ClipmapTransform = Shader.PropertyToID("_ClipmapPosition");

            private class ClipmapPassData
            {
                public VolumetricsRenderFeatureSettings rfSettings;
                public bool update;
                public Vector3 cameraPos;
                public List<TextureHandle> volumes;
                public ClipmapHistory clipmapHistory;
            }

            VolumetricsRenderFeatureSettings rfSettings;
            VolumetricData volData;


            internal VolumetricsRenderPass(VolumetricsRenderFeatureSettings settings, VolumetricData data)
            {
                rfSettings = settings;
            }

            static FieldInfo fuckyou = typeof(BaseCommandBuffer).GetField("m_WrappedCommandBuffer", BindingFlags.Instance | BindingFlags.NonPublic);
            private static void ExecuteClipmapPass(ClipmapPassData data, ComputeGraphContext context)
            {
                ComputeCommandBuffer cmd = context.cmd;
                CommandBuffer realCmd = (CommandBuffer)fuckyou.GetValue(cmd);
                if (data.update)
                {
                    data.clipmapHistory.previousFrameFence = realCmd.CreateGraphicsFence(GraphicsFenceType.AsyncQueueSynchronisation, SynchronisationStageFlags.ComputeProcessing);
                }
            }

            public override void RecordRenderGraph(RenderGraph renderGraph, ContextContainer frameData)
            {

                if (disposed) return;

                const string passName = "SLZ Volumetrics";
                UniversalCameraData cameraData = frameData.Get<UniversalCameraData>();
                UniversalCameraHistory history = cameraData.historyManager;
                history.RequestAccess<ClipmapHistory>();
                ClipmapHistory clipInfo = history.GetHistoryForWrite<ClipmapHistory>();
                RenderTextureDescriptor clipDesc = new RenderTextureDescriptor()
                {
                    width = volData.ClipMapResolution,
                    height = volData.ClipMapResolution,
                    msaaSamples = 1,
                    volumeDepth = volData.ClipMapResolution,
                    mipCount = 1,
                    graphicsFormat = UnityEngine.Experimental.Rendering.GraphicsFormat.B10G11R11_UFloatPack32,
                    depthStencilFormat = UnityEngine.Experimental.Rendering.GraphicsFormat.None,
                    dimension = TextureDimension.Tex3D
                };
                clipDesc.enableRandomWrite = true;
                clipInfo.Update(ref clipDesc, ref clipDesc);
                Vector3 cameraPos = cameraData.camera.transform.position;
                if (clipInfo.ClipmapNeedsUpdate(cameraData.camera.transform.position, 1.0f))
                {
                    clipInfo.SetClipmapPosition(cameraPos);
                    // This adds a raster render pass to the graph, specifying the name and the data type that will be passed to the ExecutePass function.
                    using (IComputeRenderGraphBuilder builder = renderGraph.AddComputePass<ClipmapPassData>(passName, out var passData))
                    {                    
                        builder.AllowPassCulling(false);
                        builder.EnableAsyncCompute(true);

                        // REPLACE 1.0 WITH ACTUAL ClipmapResampleThreshold!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        clipInfo.GetCurrentTextures(out RTHandle clipNear, out RTHandle clipFar, out Vector3 clipPos);
                        TextureHandle clipNearHandle = renderGraph.ImportTexture(clipNear);
                        TextureHandle clipFarHandle = renderGraph.ImportTexture(clipFar);
                        builder.UseTexture(clipNearHandle, AccessFlags.ReadWrite);
                        builder.UseTexture(clipFarHandle, AccessFlags.ReadWrite);
                        //renderGraph.Imp(this.blueNoiseConstants, false);

                        // Use this scope to set the required inputs and outputs of the pass and to
                        // setup the passData with the required properties needed at pass execution time.

                        // Make use of frameData to access resources and camera data through the dedicated containers.
                        // Eg:
                        // UniversalCameraData cameraData = frameData.Get<UniversalCameraData>();
                        UniversalResourceData resourceData = frameData.Get<UniversalResourceData>();

                            // Setup pass inputs and outputs through the builder interface.
                            // Eg:
                            // builder.UseTexture(sourceTexture);
                            // TextureHandle destination = UniversalRenderer.CreateRenderGraphTexture(renderGraph, cameraData.cameraTargetDescriptor, "Destination Texture", false);

                            // This sets the render target of the pass to the active color texture. Change it to your own render target as needed.
                            //builder.SetRenderAttachment(resourceData.activeColorTexture, 0);

                            // Assigns the ExecutePass function to the render pass delegate. This will be called by the render graph when executing the pass.
                            builder.SetRenderFunc((ClipmapPassData data, ComputeGraphContext context) => ExecuteClipmapPass(data, context));
                    }
                }

            }


            bool disposed = false;

            public void Dispose()
            {
                Dispose(true);
            }

            public void Dispose(bool disposing = false)
            {
                if (!disposed)
                {
                   
                }

                if (disposing)
                {
                    GC.SuppressFinalize(this);
                }
                disposed = true;
            }
        }
    }
}
