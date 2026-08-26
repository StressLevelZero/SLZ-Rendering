using System;
using System.Diagnostics;
using System.Collections.Generic;
using Unity.Collections;
using UnityEditor;
using UnityEngine.Experimental.Rendering;
using UnityEngine.Rendering.RenderGraphModule;

namespace UnityEngine.Rendering.Universal
{
    public abstract partial class ScriptableRenderer: IDisposable
    {
        public List<ScriptableRendererFeature> GetRendererFeatures()
        {
            return m_RendererFeatures;
        }
    }
}