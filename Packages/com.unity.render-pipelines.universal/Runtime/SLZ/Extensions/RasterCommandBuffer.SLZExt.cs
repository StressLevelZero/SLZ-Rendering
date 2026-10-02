using System;
using System.Diagnostics;
using System.Collections.Generic;
using Unity.Collections;
using UnityEditor;
using UnityEngine.Experimental.Rendering;
using UnityEngine.Rendering.RenderGraphModule;

namespace UnityEngine.Rendering.Universal
{
    public static class RasterCommandBufferSLZExt
    {
        public static CommandBuffer GetWrappedCommandBufferExt(this RasterCommandBuffer rcmd)
        {
            return rcmd.m_WrappedCommandBuffer;
        }
    }
}
