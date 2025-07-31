using UnityEngine;
using UnityEngine.Experimental.Rendering;
using UnityEngine.Rendering;

namespace SLZRendering.Runtime
{
    public class ClipmapHistory : CameraHistoryItem
    {
        public GraphicsFence previousFrameFence;
        private int m_NearID;
        private int m_FarID;

        private RenderTextureDescriptor m_DescriptorNear;
        private RenderTextureDescriptor m_DescriptorFar;
        private Hash128 m_DescKeyNear;
        private Hash128 m_DescKeyFar;

        /// <summary>
        /// World-space position where the clipmaps were last captured
        /// </summary>
        private Vector3 m_clipPos = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);

        /// <inheritdoc />
        public override void OnCreate(BufferedRTHandleSystem owner, uint typeId)
        {
            base.OnCreate(owner, typeId);
            m_NearID = MakeId(0);
            m_FarID = MakeId(1);
        }

        /// <summary>
        /// Get the current history texture.
        /// Current history might not be valid yet. It is valid only after executing the producing render pass.
        /// </summary>
        /// <param name="clipNear">Near clipmap</param>
        /// <param name="clipFar">Far clipmap</param>
        /// <param name="clipPos">worldspace position where the clipmaps were captured</param>
        public void GetCurrentTextures(out RTHandle clipNear, out RTHandle clipFar, out Vector3 clipPos)
        {
            clipNear = GetCurrentFrameRT(m_NearID);
            clipFar = GetCurrentFrameRT(m_FarID);
            clipPos = m_clipPos;
        }



        public bool ClipmapNeedsUpdate(Vector3 cameraPos, float maxDist)
        {
            Vector3 cam2clip = cameraPos - m_clipPos;
            float sqDist = Vector3.Dot(cam2clip, cam2clip);
            return sqDist < (maxDist * maxDist);
        }

        public void SetClipmapPosition(Vector3 cameraPos)
        {
            m_clipPos = cameraPos;
        }

        private bool IsAllocated()
        {
            return GetCurrentFrameRT(m_NearID) != null && GetCurrentFrameRT(m_FarID) != null;
        }

        // True if the desc changed, graphicsFormat etc.
        private bool IsDirty(ref RenderTextureDescriptor descNear, ref RenderTextureDescriptor descFar)
        {
            return m_DescKeyNear != Hash128.Compute(ref descNear) || m_DescKeyFar != Hash128.Compute(ref descFar);
        }

        private void Alloc(ref RenderTextureDescriptor descNear, ref RenderTextureDescriptor descFar)
        {
            AllocHistoryFrameRT(m_NearID, 1, ref descNear, "volumeClipNear");
            AllocHistoryFrameRT(m_FarID, 1, ref descFar, "volumeClipFar");

            m_DescriptorNear = descNear;
            m_DescriptorFar = descFar;
            m_DescKeyNear = Hash128.Compute(ref descNear);
            m_DescKeyFar = Hash128.Compute(ref descFar);
        }

        /// <summary>
        /// Release the history texture(s).
        /// </summary>
        public override void Reset()
        {
            ReleaseHistoryFrameRT(m_NearID);
            ReleaseHistoryFrameRT(m_FarID);
        }

        // Return true if the RTHandles were reallocated.
        public bool Update(ref RenderTextureDescriptor nearClipDesc, ref RenderTextureDescriptor farClipDesc)
        {
            if (nearClipDesc.width > 0 && nearClipDesc.height > 0 && nearClipDesc.graphicsFormat != GraphicsFormat.None)
            {

                if (IsDirty(ref nearClipDesc, ref farClipDesc))
                    Reset();

                if (!IsAllocated())
                {
                    Alloc(ref nearClipDesc, ref farClipDesc);
                    return true;
                }

            }

            return false;
        }
    }
}