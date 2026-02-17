using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using UnityEditor;
using UnityEngine;
using static SLZ.SLZTextureProcessor.TextureImporterMirror;
using static SLZ.SLZTextureProcessor.TextureUtilMirror;

namespace SLZ.SLZTextureProcessor
{
    internal class TextureUtilMirror
    {

        internal delegate void d_SetTexture2DStreamingMipmapsPriority(Texture2D textureRef, int priority);
        internal static d_SetTexture2DStreamingMipmapsPriority s_SetTexture2DStreamingMipmapsPriority;
        internal static d_SetTexture2DStreamingMipmapsPriority SetTexture2DStreamingMipmapsPriority
        {
            get
            {
                if (s_SetTexture2DStreamingMipmapsPriority == null)
                {
                    Assembly editorAssembly = typeof(TextureImporter).Assembly;
                    Type texUtilType = editorAssembly.GetType("UnityEditor.TextureUtil");
                    if (texUtilType == null)
                    {
                        Debug.LogError("TextureUtilMirror: Failed to find TextureUtil in assembly of TextureImporter!");
                        return null;
                    }
                    MethodInfo mi = texUtilType.GetMethod("SetTexture2DStreamingMipmapsPriority", BindingFlags.Static | BindingFlags.Public);
                    if (mi == null)
                    {
                        Debug.LogError("TextureUtilMirror: Failed to reflect method SetTexture2DStreamingMipmapsPriority from TextureUtil!");
                        return null;
                    }
                    s_SetTexture2DStreamingMipmapsPriority = (d_SetTexture2DStreamingMipmapsPriority)mi.CreateDelegate(typeof(d_SetTexture2DStreamingMipmapsPriority));
                    if (s_SetTexture2DStreamingMipmapsPriority == null)
                    {
                        Debug.LogError("TextureUtilMirror: Failed to create delegate for method SetTexture2DStreamingMipmapsPriority from TextureUtil!");
                    }
                }
                return s_SetTexture2DStreamingMipmapsPriority;
            }
        }

        internal delegate void d_SetTexture2DStreamingMipmaps(Texture2D textureRef, bool streaming);
        internal static d_SetTexture2DStreamingMipmaps s_SetTexture2DStreamingMipmaps;
        internal static d_SetTexture2DStreamingMipmaps SetTexture2DStreamingMipmaps
        {
            get
            {
                if (s_SetTexture2DStreamingMipmaps == null)
                {
                    Assembly editorAssembly = typeof(TextureImporter).Assembly;
                    Type texUtilType = editorAssembly.GetType("UnityEditor.TextureUtil");
                    if (texUtilType == null)
                    {
                        Debug.LogError("TextureUtilMirror: Failed to find TextureUtil in assembly of TextureImporter!");
                        return null;
                    }
                    MethodInfo mi = texUtilType.GetMethod("SetTexture2DStreamingMipmaps", BindingFlags.Static | BindingFlags.Public);
                    if (mi == null)
                    {
                        Debug.LogError("TextureUtilMirror: Failed to reflect method SetTexture2DStreamingMipmaps from TextureUtil!");
                        return null;
                    }
                    s_SetTexture2DStreamingMipmaps = (d_SetTexture2DStreamingMipmaps)mi.CreateDelegate(typeof(d_SetTexture2DStreamingMipmaps));
                    if (s_SetTexture2DStreamingMipmaps == null)
                    {
                        Debug.LogError("TextureUtilMirror: Failed to create delegate for method SetTexture2DStreamingMipmaps from TextureUtil!");
                    }
                }
                return s_SetTexture2DStreamingMipmaps;
            }
        }
    }
}
