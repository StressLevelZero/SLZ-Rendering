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

public class SLZGlobals : ScriptableRendererFeature
{
    #region Shader Properties
    public static readonly int id_BlueNoisePack0 = Shader.PropertyToID("_BlueNoise_0");
    public static readonly int id_BlueNoisePack1 = Shader.PropertyToID("_BlueNoise_1");
    public static readonly int id_BlueNoiseRGBA64 = Shader.PropertyToID("_BlueNoise_RGBA64");
    public static readonly int id_BlueNoiseDimBuffer = Shader.PropertyToID("BlueNoiseDim");
    #endregion // Shader Properties

    #region Frame Counter
    public static ulong frameCount { get { return s_FrameCount; } }
    static ulong s_FrameCount = 0u;
    internal static void IncrementFrameCounter(ScriptableRenderContext ctx, List<Camera> cameras) { s_FrameCount++; }

#if UNITY_EDITOR
    [InitializeOnLoadMethod]
#else
    [RuntimeInitializeOnLoadMethod]
#endif
    public static void RegisterFrameCounter()
    {
       
        RenderPipelineManager.beginContextRendering += IncrementFrameCounter;
    }
    #endregion // Frame Counter



    [SerializeField] SLZGlobalsSettings settings;
    SLZGlobalsPass m_Pass;

    SlzRpRuntimeResources slzRpResources;

    /// <inheritdoc/>
    public override void Create()
    {
        slzRpResources = GraphicsSettings.GetRenderPipelineSettings<SlzRpRuntimeResources>();
        m_Pass = new SLZGlobalsPass(settings, slzRpResources);
        
        // Configures where the render pass should be injected.
        m_Pass.renderPassEvent = RenderPassEvent.BeforeRenderingPrePasses;
        // You can request URP color texture and depth buffer as inputs by uncommenting the line below,
        // URP will ensure copies of these resources are available for sampling before executing the render pass.
        // Only uncomment it if necessary, it will have a performance impact, especially on mobiles and other TBDR GPUs where it will break render passes.
        //m_ScriptablePass.ConfigureInput(ScriptableRenderPassInput.Color | ScriptableRenderPassInput.Depth);

        // You can request URP to render to an intermediate texture by uncommenting the line below.
        // Use this option for passes that do not support rendering directly to the backbuffer.
        // Only uncomment it if necessary, it will have a performance impact, especially on mobiles and other TBDR GPUs where it will break render passes.
        //m_ScriptablePass.requiresIntermediateTexture = true;
        
    }

    // Here you can inject one or multiple render passes in the renderer.
    // This method is called when setting up the renderer once per-camera.
    public override void AddRenderPasses(ScriptableRenderer renderer, ref RenderingData renderingData)
    {
        renderer.EnqueuePass(m_Pass);
    }


    protected override void Dispose(bool disposing)
    {
        m_Pass.Dispose(disposing);
    }

    // Use this class to pass around settings from the feature to the pass
    [Serializable, ReloadGroup]
    public class SLZGlobalsSettings
    {
        [Reload("Textures/BlueNoise/BlueNoiseR_32x32x16.asset")]
        public Texture2DArray m_BlueNoiseR_32x32x16;
        [Reload("Textures/BlueNoise/BlueNoiseR_64x64x64.asset")]
        public Texture2DArray m_BlueNoiseR_64x64x64;
        [Reload("Textures/BlueNoise/BlueNoiseRGBA_64x64x64.asset")]
        public Texture2DArray m_BlueNoiseRGBA_64x64x64;
    }

    #region RenderPass

    class SLZGlobalsPass : ScriptableRenderPass, IDisposable
    {
        readonly SLZGlobalsSettings settings;
        readonly SlzRpRuntimeResources slzRpResources;
        public ComputeBuffer blueNoiseConstants;

        [StructLayout(LayoutKind.Explicit, Size = 32)]
        public struct BlueNoiseConstants
        {
            [FieldOffset(0)] public float4 _BlueNoise_0;
            [FieldOffset(0)] public float3 _BlueNoise_dim;
            [FieldOffset(12)] public int _BlueNoise_Frame; 
            [FieldOffset(16)] public float4 _BlueNoise_1;
            [FieldOffset(16)] public float2 _BlueNoise_RandomOffset;
        }

        public NativeArray<BlueNoiseConstants> constArray;


        public SLZGlobalsPass(SLZGlobalsSettings settings, SlzRpRuntimeResources slzRpResources)
        {
            this.settings = settings;
            this.slzRpResources = slzRpResources;
            blueNoiseConstants = new ComputeBuffer(1, Marshal.SizeOf<BlueNoiseConstants>(), ComputeBufferType.Constant);
            //Debug.Log($"blueNoiseConstants stride: {blueNoiseConstants.stride}");
            constArray = new NativeArray<BlueNoiseConstants>(1,Allocator.Persistent, NativeArrayOptions.ClearMemory);
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
                if (blueNoiseConstants != null)
                {
                    blueNoiseConstants.Dispose();
                    blueNoiseConstants = null;
                }
                if (constArray.IsCreated)
                {
                    constArray.Dispose();
                }
            }

            if (disposing)
            {
                GC.SuppressFinalize(this);
            }
            disposed = true;
        }



        // This class stores the data needed by the RenderGraph pass.
        // It is passed as a parameter to the delegate function that executes the RenderGraph pass.
        private class PassData
        {
            public uint m_FrameCount;
            public TextureHandle m_BlueNoiseR_32x32x16;
            public TextureHandle m_BlueNoiseR_64x64x64;
            public TextureHandle m_BlueNoiseRGBA_64x64x64;
            public TextureHandle m_FgdGgxLut;
            public ComputeBuffer blueNoiseConstantBuffer;
            public NativeArray<BlueNoiseConstants> constArray;
            public float4 blueNoiseDimFrame;
        }

      
        // This static method is passed as the RenderFunc delegate to the RenderGraph render pass.
        // It is used to execute draw commands.
        static void ExecutePass(PassData data, UnsafeGraphContext context)
        {
            Unity.Mathematics.Random rand = new Unity.Mathematics.Random(data.m_FrameCount + 1u);
            float2 randomOffset = rand.NextFloat2();
            data.constArray[0] = new BlueNoiseConstants
            {
                _BlueNoise_0 = data.blueNoiseDimFrame,
                _BlueNoise_1 = new float4(randomOffset, 0, 0)
            };
            //CommandBuffer cmdReal = (CommandBuffer)fuckyou.GetValue((BaseCommandBuffer)context.cmd);
            context.cmd.SetBufferData(data.blueNoiseConstantBuffer, data.constArray);
            context.cmd.SetGlobalConstantBuffer(data.blueNoiseConstantBuffer, id_BlueNoiseDimBuffer, 0, data.blueNoiseConstantBuffer.count * data.blueNoiseConstantBuffer.stride);
            
            context.cmd.SetGlobalTexture("_BlueNoiseRGBA", data.m_BlueNoiseRGBA_64x64x64);
            context.cmd.SetGlobalTexture("_FgdGgx", data.m_FgdGgxLut);
        }

        // RecordRenderGraph is where the RenderGraph handle can be accessed, through which render passes can be added to the graph.
        // FrameData is a context container through which URP resources can be accessed and managed.
        public override void RecordRenderGraph(RenderGraph renderGraph, ContextContainer frameData)
        {
            if (disposed) return;

            const string passName = "SLZ Globals Pass";

            // This adds a raster render pass to the graph, specifying the name and the data type that will be passed to the ExecutePass function.
            using (var builder = renderGraph.AddUnsafePass<PassData>(passName, out var passData))
            {
                builder.AllowPassCulling(false);

                uint frameCount = (uint)Time.frameCount;
                float blueNoiseFrame = (float)(frameCount % (uint)settings.m_BlueNoiseRGBA_64x64x64.depth);

                //renderGraph.Imp(this.blueNoiseConstants, false);
                passData.m_FrameCount = frameCount;
                passData.m_BlueNoiseR_32x32x16 = renderGraph.ImportTexture(RTHandles.Alloc(settings.m_BlueNoiseR_32x32x16));
                builder.UseTexture(passData.m_BlueNoiseR_32x32x16, AccessFlags.Read);
                passData.m_BlueNoiseR_64x64x64 = renderGraph.ImportTexture(RTHandles.Alloc(settings.m_BlueNoiseR_64x64x64));
                builder.UseTexture(passData.m_BlueNoiseR_64x64x64, AccessFlags.Read);
                passData.m_BlueNoiseRGBA_64x64x64 = renderGraph.ImportTexture(RTHandles.Alloc(settings.m_BlueNoiseRGBA_64x64x64));
                builder.UseTexture(passData.m_BlueNoiseRGBA_64x64x64, AccessFlags.Read);
                passData.m_FgdGgxLut = renderGraph.ImportTexture(RTHandles.Alloc(slzRpResources.fdgGgxLut));
                builder.UseTexture(passData.m_FgdGgxLut, AccessFlags.Read);

                passData.blueNoiseConstantBuffer = this.blueNoiseConstants;
                passData.constArray = this.constArray;
                passData.blueNoiseDimFrame = new float4(settings.m_BlueNoiseRGBA_64x64x64.width, settings.m_BlueNoiseRGBA_64x64x64.height, settings.m_BlueNoiseRGBA_64x64x64.depth, blueNoiseFrame);
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
                builder.SetRenderFunc((PassData data, UnsafeGraphContext context) => ExecutePass(data, context));
            }
        }
    }

    #endregion // Renderpass
}
