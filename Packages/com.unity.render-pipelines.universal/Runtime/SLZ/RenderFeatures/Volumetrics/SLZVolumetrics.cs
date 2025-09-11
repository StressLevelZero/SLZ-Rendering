using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using UnityEngine.Rendering.RenderGraphModule;
using UnityEditor;
using Unity.Mathematics;
using Unity.Mathematics.Geometry;
using System.Runtime.InteropServices;
using Unity.Collections;
using System.Reflection;
using UnityEngine.UI;
using UnityEngine.UIElements;

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

            static int[] ID_ClipGenVolumes = new int[8] {
                Shader.PropertyToID("_VolumeMap0"),
                Shader.PropertyToID("_VolumeMap1"),
                Shader.PropertyToID("_VolumeMap2"),
                Shader.PropertyToID("_VolumeMap3"),
                Shader.PropertyToID("_VolumeMap4"),
                Shader.PropertyToID("_VolumeMap5"),
                Shader.PropertyToID("_VolumeMap6"),
                Shader.PropertyToID("_VolumeMap7"),
                };

            private class ClipmapPassData
            {
                public VolumetricsRenderFeatureSettings rfSettings;
                public VolumetricData volData;
                public TextureHandle clipNear;
                public TextureHandle clipFar;
                public bool update;
                public List<MinMaxAABB> volumeAABBs;
                public List<TextureHandle> volumes;
                public List<float> volumeMipLevels;
                public ClipmapHistory clipmapHistory;
            }

            VolumetricsRenderFeatureSettings rfSettings;
            VolumetricData volData;


            internal VolumetricsRenderPass(VolumetricsRenderFeatureSettings settings, VolumetricData data)
            {
                rfSettings = settings;
                volData = data;
            }

            static FieldInfo fuckyou = typeof(BaseCommandBuffer).GetField("m_WrappedCommandBuffer", BindingFlags.Instance | BindingFlags.NonPublic);
            private static void ExecuteClipmapPass(ClipmapPassData data, ComputeGraphContext context)
            {
                ComputeCommandBuffer cmd = context.cmd;
                CommandBuffer realCmd = (CommandBuffer)fuckyou.GetValue(cmd);
                if (data.update)
                {
                    if (data.clipmapHistory.fenceSet)
                    {
                        realCmd.WaitOnAsyncGraphicsFence(data.clipmapHistory.previousFrameFence);
                    }

                    ComputeShader clipmapCompute = data.rfSettings.ClipmapCompute;

                    cmd.SetComputeTextureParam(clipmapCompute, 0, "Clipmap", data.clipFar);

                    int clipmapResolution = data.volData.ClipMapResolution;
                    float4 oldClipMin_ClipResolution = new float4((float3)data.clipmapHistory.prevClipPos - (0.5f * data.volData.ClipmapScale2), data.volData.ClipMapResolution);
                    float4 newClipMin_ClipSize = new float4((float3)data.clipmapHistory.clipPos - (0.5f * data.volData.ClipmapScale2), data.volData.ClipmapScale2);
                    var clipConsts = new ClipmapHistory.ClipmapConstants[1] { new ClipmapHistory.ClipmapConstants() { oldClipMin_ClipResolution = oldClipMin_ClipResolution, newClipMin_ClipSize = newClipMin_ClipSize } };
                    cmd.SetBufferData(data.clipmapHistory.clipmapConsts, clipConsts);
                    cmd.SetComputeConstantBufferParam(clipmapCompute, "clipmapGenConstants", data.clipmapHistory.clipmapConsts, 0, data.clipmapHistory.clipmapConsts.count * data.clipmapHistory.clipmapConsts.stride);
                    int numVolumes = data.volumeAABBs.Count;
                    int numVolumes8 = (data.volumeAABBs.Count + 7) / 8;

                    float4[] volumeConsts = new float4[17];
                    volumeConsts[0] = new float4(math.asfloat(numVolumes), math.asfloat(1), 0, 0);
                    for (int gIdx = 0; gIdx < numVolumes8; gIdx++)
                    {
                        for (int hIdx = 0; hIdx < 8; hIdx++)
                        {
                            int vIdx = gIdx * 8 + hIdx;
                            if (vIdx >= numVolumes)
                            {
                                cmd.SetComputeTextureParam(data.rfSettings.ClipmapCompute, 0, ID_ClipGenVolumes[hIdx], data.volumes[0]);
                            }
                            else
                            {
                                volumeConsts[hIdx + 1] = math.float4(data.volumeAABBs[vIdx].Min, data.volumeMipLevels[vIdx]);
                                volumeConsts[hIdx + 9] = math.float4(math.rcp(data.volumeAABBs[vIdx].Max - data.volumeAABBs[vIdx].Min), 0);
                                cmd.SetComputeTextureParam(data.rfSettings.ClipmapCompute, 0, ID_ClipGenVolumes[hIdx], data.volumes[vIdx]);
                            }
                        }
                        cmd.SetBufferData(data.clipmapHistory.volumeConsts, volumeConsts);
                        cmd.SetComputeConstantBufferParam(clipmapCompute, "clipmapGenPerVolume", data.clipmapHistory.volumeConsts, 0, data.clipmapHistory.volumeConsts.count * data.clipmapHistory.volumeConsts.stride);
                        cmd.DispatchCompute(clipmapCompute, 0, (clipmapResolution + 7) / 8, (clipmapResolution + 7) / 8, clipmapResolution);
                    }
                    data.clipmapHistory.previousFrameFence = realCmd.CreateGraphicsFence(GraphicsFenceType.AsyncQueueSynchronisation, SynchronisationStageFlags.ComputeProcessing);
                }
            }

            public override void RecordRenderGraph(RenderGraph renderGraph, ContextContainer frameData)
            {

                if (disposed) return;

                const string passName = "SLZ Volumetrics";
                UniversalCameraData cameraData = frameData.Get<UniversalCameraData>();
                UniversalCameraHistory history = cameraData.historyManager;
                if (history == null)
                {
                    //Debug.LogError($"Camera {cameraData.camera.name} has no history?");
                    return;
                }
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
                bool freshClipmaps = clipInfo.Update(ref clipDesc, ref clipDesc);
                Vector3 cameraPos = cameraData.camera.transform.position;
                
                if (true || freshClipmaps || clipInfo.ClipmapNeedsUpdate(cameraData.camera.transform.position, volData.ClipmapResampleThreshold))
                {
                    clipInfo.SetClipmapPosition(cameraPos);
                    // This adds a raster render pass to the graph, specifying the name and the data type that will be passed to the ExecutePass function.
                    using (IComputeRenderGraphBuilder builder = renderGraph.AddComputePass<ClipmapPassData>(passName, out var passData))
                    {                    
                        builder.AllowPassCulling(false);
                        builder.EnableAsyncCompute(true);

                       
                        clipInfo.GetCurrentTextures(out RTHandle clipNear, out RTHandle clipFar, out Vector3 clipPos);
                        TextureHandle clipNearHandle = renderGraph.ImportTexture(clipNear);
                        TextureHandle clipFarHandle = renderGraph.ImportTexture(clipFar);
                        builder.UseTexture(clipNearHandle, AccessFlags.ReadWrite);
                        builder.UseTexture(clipFarHandle, AccessFlags.ReadWrite);
                        NativeArray<ushort> idxBuffer = new NativeArray<ushort>(VolumetricManager.volumeCount, Allocator.Temp);
                        int volumeCount = VolumetricManager.GetVolumesBruteForce(cameraPos, volData.ClipmapScale2, ref idxBuffer);
                        List<TextureHandle> volumes = new List<TextureHandle>(volumeCount);
                        List<MinMaxAABB> volAABBs = new List<MinMaxAABB>(volumeCount);
                        List<float> volumeMipLevels = new List<float>(volumeCount);
                        ImportResourceParams param = new ImportResourceParams() { clearOnFirstUse = false, clearColor = Color.black, discardOnLastUse = false };
                        string debugMsg = $"Updating clipmap for camera '{cameraData.camera}', Clipmap intersects {volumeCount} volumes:\n";
                        float clip2Dv = volData.ClipMapResolution / volData.ClipmapScale2;
                        float3 clip2Dv3 = new float3(clip2Dv, clip2Dv, clip2Dv);
                        for (int vIdx = 0; vIdx < volumeCount; vIdx++)
                        {
                            int index = idxBuffer[vIdx];
                            TextureHandle volTex = renderGraph.ImportTexture(VolumetricManager.volumeTexHandles[index], param);
                            Texture3D volumeTexture = VolumetricManager.volumes[index].bakedTexture;
                            float3 volumeRes = new float3(volumeTexture.width, volumeTexture.height, volumeTexture.depth);
                            float3 volumeSize = VolumetricManager.volumeBounds[index].Max - VolumetricManager.volumeBounds[index].Min;
                            float3 volumeDv = volumeRes / volumeSize;
                            float3 mip3D = math.log2(volumeDv / clip2Dv3);
                            volumeMipLevels.Add(math.max(math.max(0, mip3D.x), math.max(mip3D.y, mip3D.z)));
                            builder.UseTexture(volTex, AccessFlags.Read);
                            volumes.Add(volTex);
                            volAABBs.Add(VolumetricManager.volumeBounds[index]);
                            debugMsg += $"{VolumetricManager.volumes[index].gameObject.name}\n";
                        }
                        Debug.Log(debugMsg);
                        idxBuffer.Dispose();
                        passData.rfSettings = rfSettings;
                        passData.clipNear = clipNearHandle;
                        passData.clipFar = clipFarHandle;
                        passData.update = true;
                        passData.volData = volData;
                        passData.volumeAABBs = volAABBs;
                        passData.volumes = volumes;
                        passData.volumeMipLevels = volumeMipLevels;
                        passData.clipmapHistory = clipInfo;

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
