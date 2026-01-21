using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.RenderGraphModule;
using UnityEngine.Rendering.Universal;
using Unity.Mathematics;
using UnityEngine.Experimental.Rendering;


#if UNITY_EDITOR
using UnityEditor;
#endif

namespace UnityEngine.Rendering.Universal.Internal
{
    public class PopulateShadingRatePass : ScriptableRenderPass
    {
        static ComputeShader s_populateShader;

        /// <summary>
        /// Creates a new <c>PopulateShadingRatePass</c> instance.
        /// </summary>
        /// <param name="evt">The <c>RenderPassEvent</c> to use.</param>
        /// <param name="renderQueueRange">The <c>RenderQueueRange</c> to use for creating filtering settings that control what objects get rendered.</param>
        /// <param name="layerMask">The layer mask to use for creating filtering settings that control what objects get rendered.</param>
        /// <seealso cref="RenderPassEvent"/>
        /// <seealso cref="RenderQueueRange"/>
        /// <seealso cref="LayerMask"/>
        public PopulateShadingRatePass(RenderPassEvent evt)
        {
            profilingSampler = new ProfilingSampler("Populate Shading Rate Image");
            renderPassEvent = evt;

#if UNITY_EDITOR
            if (s_populateShader == null)
            {
                s_populateShader = AssetDatabase.LoadAssetAtPath<ComputeShader>(AssetDatabase.GUIDToAssetPath("387556093d452bd429d02eb20a1316ab"));
            }
#endif
#if URP_COMPATIBILITY_MODE
                useNativeRenderPass = false;
                m_PassData = new PassData();
#endif
        }

        private class PassData
        {
            public ProfilingSampler profilingSampler;
            public ComputeShader populateShader;
            public int3 shadingRateTexDimensions;
            public TextureHandle shadingRateTex;
            public Vector4 eyeCenterCoords;
        }

        private static void ExecutePass(ComputeCommandBuffer cmd, PassData passData)
        {
            using (new ProfilingScope(cmd, passData.profilingSampler))
            {

                int kernel = 0;
                bool arrayed = (passData.shadingRateTexDimensions.z > 1);
                cmd.SetKeyword(passData.populateShader, new LocalKeyword(passData.populateShader, "ARRAY_TARGET"), arrayed);
                cmd.SetComputeTextureParam(passData.populateShader, kernel, "Result", passData.shadingRateTex);
                cmd.SetComputeVectorParam(passData.populateShader, "_EyeCenters", passData.eyeCenterCoords);
                int3 threadGroups = (passData.shadingRateTexDimensions + new int3(7, 7, 0)) / new int3(8, 8, 1);
                cmd.DispatchCompute(passData.populateShader, kernel, threadGroups.x, threadGroups.y, threadGroups.z);
            }
        }

        public static Vector2 CalculateEyeCenter(Matrix4x4 projection)
        {
            float w = (projection[3, 3] - 1.6f * projection[3, 2]);
            float x_center = (projection[0, 3] - 1.6f * projection[0, 2]) / w;
            float y_center = (projection[1, 3] - 1.6f * projection[1, 2]) / w;
            return new Vector2(0.5f * x_center + 0.5f, 0.5f * y_center + 0.5f);
        }


        internal void Render(RenderGraph renderGraph, ContextContainer frameData, in TextureHandle shadingRateTexture)
        {
            if (!shadingRateTexture.IsValid())
            {
                return;
            }
            UniversalRenderingData renderingData = frameData.Get<UniversalRenderingData>();
            UniversalCameraData cameraData = frameData.Get<UniversalCameraData>();
            UniversalLightData lightData = frameData.Get<UniversalLightData>();

            using (var builder = renderGraph.AddComputePass<PassData>(passName, out var passData, profilingSampler))
            {
                builder.AllowGlobalStateModification(true);
                passData.populateShader = s_populateShader;
                passData.profilingSampler = profilingSampler;
                passData.shadingRateTex = shadingRateTexture;
                TextureDesc srTexDesc = renderGraph.GetTextureDesc(shadingRateTexture);
                passData.shadingRateTexDimensions = new int3(srTexDesc.width, srTexDesc.height, math.max(1,srTexDesc.slices));
                int rightEyeIndex = cameraData.xr.singlePassEnabled ? 1 : 0;
                Matrix4x4 p_left = cameraData.GetProjectionMatrix(0);
                Matrix4x4 p_right = cameraData.GetProjectionMatrix(rightEyeIndex);
                Vector2 centerLeft = CalculateEyeCenter(p_left);
                Vector2 centerRight = CalculateEyeCenter(p_right);
                passData.eyeCenterCoords = new Vector4(centerLeft.x, centerLeft.y, centerRight.x, centerRight.y);
                //Debug.Log($"EyeCenters: {passData.eyeCenterCoords}");
                /// SLZ MODIFIED - Use fragment shading rate image when available
                if (shadingRateTexture.IsValid())
                {
                    builder.UseTexture(shadingRateTexture, AccessFlags.ReadWrite);
                }


                builder.SetRenderFunc((PassData data, ComputeGraphContext context) =>
                {
                    ExecutePass(context.cmd, passData);
                });
            }
        }
    }
}
