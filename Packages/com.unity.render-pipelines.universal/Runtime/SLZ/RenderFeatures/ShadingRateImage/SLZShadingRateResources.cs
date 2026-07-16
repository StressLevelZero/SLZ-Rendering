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
    public class SLZShadingRateResources : ScriptableRendererFeature
    {
        public static ComputeShader s_PopulateShader;
        public static ComputeShader PopulateShader {
            get
            {
                if (s_PopulateShader == null)
                {
                    Debug.LogError("SLZShadingRateResources.PopulateShader is not set, either you are missing a" 
                    + " SLZShadingRateResources render feature or you are trying to access this too early");
                }
                return s_PopulateShader;
            }
            internal set
            {
                s_PopulateShader = value;
            }
        }
        public ComputeShader m_PopulateShader;

        public void Awake()
        {
            PopulateShader = m_PopulateShader;
        }

        public override void Create()
        {
            PopulateShader = m_PopulateShader;
        }

        public override void AddRenderPasses(ScriptableRenderer renderer, ref RenderingData renderingData)
        {

        }
    }
}
