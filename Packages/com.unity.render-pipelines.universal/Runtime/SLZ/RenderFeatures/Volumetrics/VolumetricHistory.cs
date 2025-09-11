using System.Runtime.InteropServices;
using Unity.Mathematics;
using UnityEngine;
using UnityEngine.Experimental.Rendering;
using UnityEngine.Rendering;

namespace SLZRendering.Runtime
{
    public class ClipmapHistory : CameraHistoryItem
    {
        public bool fenceSet = false;
        private GraphicsFence m_previousFrameFence;
        public GraphicsFence previousFrameFence { get { return m_previousFrameFence; } set { fenceSet = true; m_previousFrameFence = value; } }
        private ComputeBuffer m_ClipmapConsts;
        public ComputeBuffer clipmapConsts 
        { 
            get 
            {
                if (m_ClipmapConsts == null)
                {
                    m_ClipmapConsts = new ComputeBuffer(1, Marshal.SizeOf<ClipmapConstants>(), ComputeBufferType.Constant);
                }
                return m_ClipmapConsts; 
            } 
        }
        private ComputeBuffer m_VolumeConsts;
        public ComputeBuffer volumeConsts
        {
            get
            {
                if (m_VolumeConsts == null)
                {
                    m_VolumeConsts = new ComputeBuffer(Marshal.SizeOf<VolumeConstants>() / (4 * sizeof(float)), 4 * sizeof(float), ComputeBufferType.Constant);
                }
                return m_VolumeConsts;
            }
        }


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
        public Vector3 clipPos { get => m_clipPos; }
        private Vector3 m_prevClipPos = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
        public Vector3 prevClipPos { get => m_prevClipPos; }

        [StructLayout(LayoutKind.Sequential)]
        public struct ClipmapConstants
        {
            // world-space position of the last clipmap's minimum bound, and the resolution of the clipmap (clipmap is assumed to have equal x,y, and z dimensions) 
            public float4 oldClipMin_ClipResolution;
            // world-space position of the current clipmap's minimum bound, and the worldspace width of the clipmap (clipmap is assumed to be cubic)
            public float4 newClipMin_ClipSize;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct VolumeConstants
        {
        	public int4 volumeCount_clearTexture;
            public float4 volBoundsMin_MipLevel0;
            public float4 volBoundsMin_MipLevel1;
            public float4 volBoundsMin_MipLevel2;
            public float4 volBoundsMin_MipLevel3;
            public float4 volBoundsMin_MipLevel4;
            public float4 volBoundsMin_MipLevel5;
            public float4 volBoundsMin_MipLevel6;
            public float4 volBoundsMin_MipLevel7;
            public float4 rcpVolBoundsSize0;
            public float4 rcpVolBoundsSize1;
            public float4 rcpVolBoundsSize2;
            public float4 rcpVolBoundsSize3;
            public float4 rcpVolBoundsSize4;
            public float4 rcpVolBoundsSize5;
            public float4 rcpVolBoundsSize6;
            public float4 rcpVolBoundsSize7;
        }

        /// <inheritdoc />
        public override void OnCreate(BufferedRTHandleSystem owner, uint typeId)
        {
            base.OnCreate(owner, typeId);
            m_NearID = MakeId(0);
            m_FarID = MakeId(1);
            m_clipPos = new Vector3(float.PositiveInfinity, float.PositiveInfinity, float.PositiveInfinity);
            m_ClipmapConsts = new ComputeBuffer(1, Marshal.SizeOf<ClipmapConstants>(), ComputeBufferType.Constant);
            m_VolumeConsts = new ComputeBuffer(Marshal.SizeOf<VolumeConstants>() / (4 * sizeof(float)), 4 * sizeof(float), ComputeBufferType.Constant);
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
            return sqDist > (maxDist * maxDist);
        }

        public void SetClipmapPosition(Vector3 cameraPos)
        {
            m_prevClipPos = m_clipPos;
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
            if (clipmapConsts != null)
            {
                m_ClipmapConsts.Release();
                m_ClipmapConsts = null;
            }
            if (volumeConsts != null)
            {
                m_VolumeConsts.Release();
                m_VolumeConsts = null;
            }
            fenceSet = false;

        }

        // Return true if the RTHandles were reallocated.
        public bool Update(ref RenderTextureDescriptor nearClipDesc, ref RenderTextureDescriptor farClipDesc)
        {
            if (nearClipDesc.width > 0 && nearClipDesc.height > 0 && nearClipDesc.graphicsFormat != GraphicsFormat.None)
            {

                if (IsDirty(ref nearClipDesc, ref farClipDesc))
                {
                    //Debug.Log("Clipmap RT descs dirty");
                    Reset();
                }

                if (!IsAllocated())
                {
                    Alloc(ref nearClipDesc, ref farClipDesc);
                    //Debug.Log("Clipmap RTs were not allocated");
                    return true;
                }

            }

            return false;
        }
    }
}