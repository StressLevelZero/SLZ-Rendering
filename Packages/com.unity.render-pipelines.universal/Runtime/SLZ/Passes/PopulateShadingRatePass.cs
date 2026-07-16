using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.RenderGraphModule;
using UnityEngine.Rendering.Universal;
using Unity.Mathematics;
using UnityEngine.Experimental.Rendering;
using SLZRendering.Runtime;


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

//#if UNITY_EDITOR
            if (s_populateShader == null)
            {
                //s_populateShader = AssetDatabase.LoadAssetAtPath<ComputeShader>(AssetDatabase.GUIDToAssetPath("387556093d452bd429d02eb20a1316ab"));
                s_populateShader = SLZRendering.Runtime.SLZShadingRateResources.PopulateShader;
            }
//#endif
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
            public ShadingRateImageHistory.FoveationSettings fovSettings;
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

        public static Vector2 CalculateEyeCenterParallel(Matrix4x4 projection)
        {
            float w = (projection[3, 3] - 100.0f * projection[3, 2]);
            float x_center = (projection[0, 3] - 1.6f * projection[0, 2]) / w;
            float y_center = (projection[1, 3] - 1.6f * projection[1, 2]) / w;
            return new Vector2(0.5f * x_center + 0.5f, 0.5f * y_center + 0.5f);
        }

/// <summary>
/// Calculates the normalized UV coordinates of the given pair of eye directions.
/// Does not suffer from precision issues like <see cref="CalculateEyeUVCoordinatesWS"/> when far from origin
/// </summary>
/// <param name="eyeDirLeft" >Left eye direction vector in worldspace (use Camera.transform.forward for fixed foveation)</param>
/// <param name="eyeDirRight">Right eye direction vector in worldspace (use Camera.transform.forward for fixed foveation)</param>
/// <param name="viewLeft" >Left eye view matrix</param>
/// <param name="viewRight">Right eye view matrix</param>
/// <param name="projLeft" >Left eye projection matrix</param>
/// <param name="projRight">Right eye projection matrix</param>
/// <returns>Vector4 containing (left eye uv, right eye uv) coordinates of the left and right eye directions</returns>
public static Vector4 CalculateEyeUVCoordinatesDir(Vector3 eyeDirLeft, Vector3 eyeDirRight, float4x4 viewLeft, float4x4 viewRight, float4x4 projLeft, float4x4 projRight)
{
    eyeDirLeft  = eyeDirLeft  * 1000.0f;
    eyeDirRight = eyeDirRight * 1000.0f;

    float3x3 viewLeftNoTranslation  = (float3x3)viewLeft;
    float3x3 viewRightNoTranslation = (float3x3)viewRight;

    float3 eyeCenterView_left  = math.mul(viewLeftNoTranslation, eyeDirLeft);
    float3 eyeCenterView_right = math.mul(viewRightNoTranslation, eyeDirRight);

    float4 eyeCenterProj_left  = math.mul(projLeft,  math.float4(eyeCenterView_left, 1.0f));
    float4 eyeCenterProj_right = math.mul(projRight, math.float4(eyeCenterView_right, 1.0f));

    // Perspective correction
    float2 eyeCenterUV_left  = eyeCenterProj_left.xy  / eyeCenterProj_left.w;
    float2 eyeCenterUV_right = eyeCenterProj_right.xy / eyeCenterProj_right.w;

    // remap from -1,1 to 0,1
    eyeCenterUV_left  = 0.5f * eyeCenterUV_left  + 0.5f; 
    eyeCenterUV_right = 0.5f * eyeCenterUV_right + 0.5f; 

    return new float4(eyeCenterUV_left, eyeCenterUV_right);
}

/// <summary>
/// Calculates the normalized UV coordinates of the given pair of eye directions.
/// Does not suffer from precision issues like <see cref="CalculateEyeUVCoordinatesWS"/> when far from origin
/// </summary>
/// <param name="cameraData">UniversalCameraData, obtain from <see cref="UnityEngine.Rendering.ContextContainer.Get{UnityEngine.Rendering.UniversalCameraData}()"/></param>
/// <param name="eyeDirLeft" >Left eye direction vector in worldspace (use Camera.transform.forward for fixed foveation)</param>
/// <param name="eyeDirRight">Right eye direction vector in worldspace (use Camera.transform.forward for fixed foveation)</param>
/// <param name="isTargetFlipped">Is the rendertarget flipped? True if not rendering directly to the backbuffer. Unity tries to normalize its uv coordinate system to OpenGL style with 0,0 at bottom left, which is upside-down for D3D and Vulkan. If rendering directly to the backbuffer it is forced to use the API defined UV coordinate system.</param>
/// <returns>Vector4 containing (left eye uv, right eye uv) coordinates of the left and right eye directions</returns>
public static Vector4 CalculateEyeUVCoordinatesDir(UniversalCameraData cameraData, Vector3 eyeDirLeft, Vector3 eyeDirRight, bool isTargetFlipped)
{
    int rightEyeIndex = cameraData.xr.singlePassEnabled ? 1 : 0;
    Matrix4x4 viewLeft  = cameraData.GetViewMatrix(0);
    Matrix4x4 viewRight = cameraData.GetViewMatrix(rightEyeIndex);

    Matrix4x4 projLeft  = GL.GetGPUProjectionMatrix(cameraData.GetProjectionMatrixNoJitter(0), isTargetFlipped);
    Matrix4x4 projRight = GL.GetGPUProjectionMatrix(cameraData.GetProjectionMatrixNoJitter(rightEyeIndex), isTargetFlipped);
    return CalculateEyeUVCoordinatesDir(eyeDirLeft, eyeDirRight, viewLeft, viewRight, projLeft, projRight);
}


/// <summary>
/// Calculates the normalized UV coordinates of a given point in world space <paramref name="focalPointWS"/> in the left and right eye. 
/// </summary>
/// <param name="focalPointWS">Focal point in worldspace.</param>
/// <param name="viewLeft">Left eye view matrix</param>
/// <param name="viewRight">Right eye view matrix</param>
/// <param name="projLeft">Left eye projection matrix</param>
/// <param name="projRight">Right eye projection matrix</param>
/// <returns>Vector4 containing (left eye uv, right eye uv) coordinates of <paramref name="focalPointWS"/></returns>
public static Vector4 CalculateEyeUVCoordinatesWS(Vector3 focalPointWS, float4x4 viewLeft, float4x4 viewRight, float4x4 projLeft, float4x4 projRight)
{

    float4 eyeCenterView_left  = math.mul(viewLeft,  math.float4(focalPointWS, 1.0f));
    float4 eyeCenterView_right = math.mul(viewRight, math.float4(focalPointWS, 1.0f));

    float4 eyeCenterProj_left  = math.mul(projLeft,  eyeCenterView_left);
    float4 eyeCenterProj_right = math.mul(projRight, eyeCenterView_right);

    // Perspective correction
    float2 eyeCenterUV_left  = eyeCenterProj_left.xy  / eyeCenterProj_left.w;
    float2 eyeCenterUV_right = eyeCenterProj_right.xy / eyeCenterProj_right.w;

    // remap from -1,1 to 0,1
    eyeCenterUV_left  = 0.5f * eyeCenterUV_left  + 0.5f; 
    eyeCenterUV_right = 0.5f * eyeCenterUV_right + 0.5f; 

    return new float4(eyeCenterUV_left, eyeCenterUV_right);
}

/// <summary>
/// Calculates the normalized UV coordinates of a given point in world space <paramref name="focalPointWS"/> in the left and right eye. 
/// </summary>
/// <param name="cameraData">UniversalCameraData, obtain from <see cref="UnityEngine.Rendering.ContextContainer.Get{UnityEngine.Rendering.UniversalCameraData}()"/></param>
/// <param name="focalPointWS">Focal point in world space</param>
/// <param name="isTargetFlipped">Is the rendertarget flipped? True if not rendering directly to the backbuffer. Unity tries to normalize its uv coordinate system to OpenGL style with 0,0 at bottom left, which is upside-down for D3D and Vulkan. If rendering directly to the backbuffer it is forced to use the API defined UV coordinate system.</param>
/// <returns>Vector4 containing (left eye uv, right eye uv) coordinates of <paramref name="focalPointWS"/></returns>
public static Vector4 CalculateEyeUVCoordinatesWS(UniversalCameraData cameraData, Vector3 focalPointWS, bool isTargetFlipped)
{

    int rightEyeIndex = cameraData.xr.singlePassEnabled ? 1 : 0;
    Matrix4x4 viewLeft  = cameraData.GetViewMatrix(0);
    Matrix4x4 viewRight = cameraData.GetViewMatrix(rightEyeIndex);

    Matrix4x4 projLeft  = GL.GetGPUProjectionMatrix(cameraData.GetProjectionMatrixNoJitter(0), isTargetFlipped);
    Matrix4x4 projRight = GL.GetGPUProjectionMatrix(cameraData.GetProjectionMatrixNoJitter(rightEyeIndex), isTargetFlipped);

    return CalculateEyeUVCoordinatesWS(focalPointWS, viewLeft, viewRight, projLeft, projRight);
}



        internal void Render(RenderGraph renderGraph, ContextContainer frameData, in TextureHandle shadingRateTexture)
        {
            if (!shadingRateTexture.IsValid())
            {
                return;
            }
            UniversalRenderingData renderingData = frameData.Get<UniversalRenderingData>();
            UniversalCameraData cameraData = frameData.Get<UniversalCameraData>();
            if (!cameraData.xr.enabled)
            {
                return;
            }
            UniversalLightData lightData = frameData.Get<UniversalLightData>();
            ShadingRateImageHistory srHistory = cameraData.historyManager.GetHistoryForWrite<ShadingRateImageHistory>();

            using (var builder = renderGraph.AddComputePass<PassData>(passName, out var passData, profilingSampler))
            {
                builder.AllowGlobalStateModification(true);
                passData.populateShader = s_populateShader;
                passData.profilingSampler = profilingSampler;
                passData.shadingRateTex = shadingRateTexture;
                TextureDesc srTexDesc = renderGraph.GetTextureDesc(shadingRateTexture);
                passData.shadingRateTexDimensions = new int3(srTexDesc.width, srTexDesc.height, math.max(1,srTexDesc.slices));
                int rightEyeIndex = cameraData.xr.singlePassEnabled ? 1 : 0;
                /*
                Matrix4x4 p_left = cameraData.GetProjectionMatrix(0);
                Matrix4x4 p_right = cameraData.GetProjectionMatrix(rightEyeIndex);
                Vector2 centerLeft = CalculateEyeCenterParallel(p_left);

                Vector2 centerRight = CalculateEyeCenterParallel(p_right);
                passData.eyeCenterCoords = new Vector4(centerLeft.x, centerLeft.y, centerRight.x, centerRight.y);
                */

                bool yFlip = cameraData.stackAnyPostProcessingEnabled;
                Vector3 focalPoint = cameraData.camera.transform.localToWorldMatrix.MultiplyPoint(cameraData.camera.transform.forward * 200.0f);
                passData.eyeCenterCoords = CalculateEyeUVCoordinatesDir(cameraData, cameraData.camera.transform.forward, cameraData.camera.transform.forward, yFlip);

                builder.UseTexture(shadingRateTexture, AccessFlags.ReadWrite);


                builder.SetRenderFunc((PassData data, ComputeGraphContext context) =>
                {
                    ExecutePass(context.cmd, passData);
                });
            }
        }
    }
}
