using System.Runtime.InteropServices;
using Unity.Mathematics;
using UnityEngine;
using UnityEngine.Experimental.Rendering;
using UnityEngine.Rendering;
using UnityEngine.Rendering.RenderGraphModule;
using UnityEngine.Rendering.Universal;

namespace SLZRendering.Runtime
{
    public class ShadingRateImageHistory : CameraHistoryItem
    {
        public struct FoveationSettings
        {
            float scale;
            float power;
        }
        public FoveationSettings settings;
        private int m_ShadingRateImageID;
        private RenderTextureDescriptor m_ShadingRateImageDescriptor;
        private ulong m_SRIDescHash64;

        public static ulong ComputeSRIHashFromCameraTarget(ref TextureDesc cameraTgt)
        {
            Vector2Int tileSize = ShadingRateInfo.imageTileSize;
            return  ((ulong)((cameraTgt.width  + tileSize.x - 1) / tileSize.x) & 0xFFFFUL) | 
                   (((ulong)((cameraTgt.height + tileSize.y - 1) / tileSize.y) & 0xFFFFUL) << 16) |
                   ((ulong)cameraTgt.slices << 32);

        }

        public static ulong ComputeSRIHash(ref RenderTextureDescriptor shadingRateImageDesc)
        {
            return ((ulong)shadingRateImageDesc.width & 0xFFFFUL) |
                   ((ulong)shadingRateImageDesc.height & 0xFFFFUL) << 16 |
                   ((ulong)shadingRateImageDesc.volumeDepth << 32);
        }

        /// <inheritdoc />
        public override void OnCreate(BufferedRTHandleSystem owner, uint typeId)
        {
            base.OnCreate(owner, typeId);
            m_ShadingRateImageID = MakeId(0);

        }

        /// <summary>
        /// Get the current history texture.
        /// Current history might not be valid yet. It is valid only after executing the producing render pass.
        /// </summary>
        /// <param name="clipNear">Near clipmap</param>
        /// <param name="clipFar">Far clipmap</param>
        /// <param name="clipPos">worldspace position where the clipmaps were captured</param>
        public void GetCurrentTexture(out RTHandle shadingRateImage)
        {
            shadingRateImage = GetCurrentFrameRT(m_ShadingRateImageID);
        }


        private bool IsAllocated()
        {
            return GetCurrentFrameRT(m_ShadingRateImageID) != null;
        }

        // True if the desc changed, graphicsFormat etc.
        private bool IsDirty(ref TextureDesc cameraTgtDesc)
        {
            return m_SRIDescHash64 != ComputeSRIHashFromCameraTarget(ref cameraTgtDesc);
        }

        private void Alloc(ref TextureDesc cameraTgtDesc)
        {

            m_ShadingRateImageDescriptor = GetShadingRateImageDesc(ref cameraTgtDesc);
            AllocHistoryFrameRT(m_ShadingRateImageID, 1, ref m_ShadingRateImageDescriptor, FilterMode.Point, "ShadingRateImage");
            m_SRIDescHash64 = ComputeSRIHash(ref m_ShadingRateImageDescriptor);
        }

        public static RenderTextureDescriptor GetShadingRateImageDesc(ref TextureDesc cameraTgtDesc)
        {
            RenderTextureDescriptor newDesc = new RenderTextureDescriptor();

            newDesc.graphicsFormat = ShadingRateInfo.graphicsFormat;
            newDesc.depthStencilFormat = GraphicsFormat.None;
            
            Vector2Int tileSize = ShadingRateInfo.imageTileSize;
            newDesc.width  = (cameraTgtDesc.width  + tileSize.x - 1) / tileSize.x;
            newDesc.height = (cameraTgtDesc.height + tileSize.y - 1) / tileSize.y;
            bool supportsLayeredAttachments = SLZ.GfxPlugin_SLZ.SupportsLayeredShadingRate() != 0;
            newDesc.volumeDepth = supportsLayeredAttachments ? cameraTgtDesc.slices : 1;
            newDesc.dimension   = supportsLayeredAttachments ? cameraTgtDesc.dimension : TextureDimension.Tex2D;
            newDesc.msaaSamples = 1;

            newDesc.useMipMap = false;
            newDesc.autoGenerateMips = false;

            newDesc.enableRandomWrite = true;
            newDesc.enableShadingRate = true;

            return newDesc;
        }

        /// <summary>
        /// Release the history texture(s).
        /// </summary>
        public override void Reset()
        {
            ReleaseHistoryFrameRT(m_ShadingRateImageID);
        }

        // Return true if the RTHandles were reallocated.
        public bool Update(ref TextureDesc cameraTgtDesc)
        {
            if (cameraTgtDesc.width > 0 && cameraTgtDesc.height > 0)
            {

                if (IsDirty(ref cameraTgtDesc))
                {
                    Reset();
                }

                if (!IsAllocated())
                {
                    Alloc(ref cameraTgtDesc);
                    return true;
                }

            }

            return false;
        }
    }
}
