using System.Reflection;
using System.Xml.Linq;
using UnityEditor;
using UnityEngine;
using static SLZ.SLZTextureProcessor.TextureImporterMirror;

namespace SLZ.SLZTextureProcessor
{
    internal static class TextureImporterMirror
    {
        internal delegate TextureImporterFormat[] d_RecommendedFormatsFromTextureTypeAndPlatform(TextureImporterType textureType, BuildTarget destinationPlatform);
        internal static d_RecommendedFormatsFromTextureTypeAndPlatform s_RecommendedFormatsFromTextureTypeAndPlatform;
        internal static d_RecommendedFormatsFromTextureTypeAndPlatform RecommendedFormatsFromTextureTypeAndPlatform
        {
            get
            {
                if (s_RecommendedFormatsFromTextureTypeAndPlatform == null)
                {
                    MethodInfo mi = typeof(TextureImporter).GetMethod("RecommendedFormatsFromTextureTypeAndPlatform", BindingFlags.Static | BindingFlags.NonPublic);
                    if (mi == null)
                    {
                        Debug.LogError("TextureImporterMirror: Failed to reflect method RecommendedFormatsFromTextureTypeAndPlatform from TextureImporter!");
                        return null;
                    }
                    s_RecommendedFormatsFromTextureTypeAndPlatform = (d_RecommendedFormatsFromTextureTypeAndPlatform)mi.CreateDelegate(typeof(d_RecommendedFormatsFromTextureTypeAndPlatform));
                    if (s_RecommendedFormatsFromTextureTypeAndPlatform == null)
                    {
                        Debug.LogError("TextureImporterMirror: Failed to create delegate for method RecommendedFormatsFromTextureTypeAndPlatform from TextureImporter!");
                    }
                }
                return s_RecommendedFormatsFromTextureTypeAndPlatform;
            }
        }


        internal delegate TextureImporterFormat d_DefaultFormatFromTextureParameters(
                TextureImporterSettings settings,
                TextureImporterPlatformSettings platformSettings,
                bool doesTextureContainAlpha,
                bool sourceWasHDR,
                BuildTarget destinationPlatform);

        internal static d_DefaultFormatFromTextureParameters s_DefaultFormatFromTextureParameters;
        internal static d_DefaultFormatFromTextureParameters DefaultFormatFromTextureParameters
        {
            get
            {
                if (s_DefaultFormatFromTextureParameters == null)
                {
                    MethodInfo mi = typeof(TextureImporter).GetMethod("DefaultFormatFromTextureParameters", BindingFlags.Static | BindingFlags.NonPublic);
                    if (mi == null)
                    {
                        Debug.LogError("TextureImporterMirror: Failed to reflect method DefaultFormatFromTextureParameters from TextureImporter!");
                        return null;
                    }
                    s_DefaultFormatFromTextureParameters = (d_DefaultFormatFromTextureParameters)mi.CreateDelegate(typeof(d_DefaultFormatFromTextureParameters));
                    if (s_DefaultFormatFromTextureParameters == null)
                    {
                        Debug.LogError("TextureImporterMirror: Failed to create delegate for method DefaultFormatFromTextureParameters from TextureImporter!");
                    }
                }
                return s_DefaultFormatFromTextureParameters;
            }
        }

    }
}
