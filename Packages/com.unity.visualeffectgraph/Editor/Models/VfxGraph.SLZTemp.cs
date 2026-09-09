// SLZ - TEMP FILE, REMOVE THIS! unity graphics repo not yet up to date, master branch does not function on 6.7 alpha
//#define USE_SHADER_AS_SUBASSET
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reflection;
using Unity.Profiling;
using UnityEditor.AssetImporters;
using UnityEditor.ShaderGraph.Internal;
using UnityEditor.VFX.Block;
using UnityEditor.VFX.UI;
using UnityEngine;
using UnityEngine.Profiling;
using UnityEngine.VFX;
using Object = System.Object;
using UnityObject = UnityEngine.Object;

namespace UnityEditor.VFX
{
    static class VisualEffectResourceExtensionsTEMPSLZ
    {
        //This function identifies obj by reference, or by name in the case of a double click in the asset browser while VFXViewWindow opened
        public static int GetShaderIndex(this VisualEffectResource resource, UnityObject obj)
        {
            if (obj == null || resource.asset == null)
                return -1;

            bool isCompute = obj is ComputeShader;
            var targetName = obj is Material m && m.shader != null ? m.shader.name : obj.name;
            if (string.IsNullOrEmpty(targetName))
                return -1;

            int count = resource.GetShaderSourceCount();
            for (int shaderIndex = 0; shaderIndex < count; ++shaderIndex)
            {
                if (isCompute)
                {
                    //if obj is computeShader, no need to access to GetShader: name is always consistent and unique (no ShaderLab declaration)
                    if (resource.GetShaderSourceName(shaderIndex) == targetName)
                        return shaderIndex;
                    continue;
                }

                var processor = resource.GetShader(shaderIndex);
                if (processor == obj)
                    return shaderIndex;
                if (processor is Shader currentShader && currentShader.name == targetName)
                    return shaderIndex;
            }
            return -1;
        }
    }
}
