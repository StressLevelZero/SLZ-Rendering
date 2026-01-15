using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Experimental.Rendering;

namespace UnityEditor.Rendering.Universal.ShaderGUI
{
    internal class LitDetailGUI
    {
        internal static class Styles
        {
            public static readonly GUIContent detailInputs = EditorGUIUtility.TrTextContent("Detail Inputs",
                "These settings define the surface details by tiling and overlaying additional maps on the surface.");

            public static readonly GUIContent detailMaskText = EditorGUIUtility.TrTextContent("Mask",
                "Select a mask for the Detail map. The mask uses the alpha channel of the selected texture. The Tiling and Offset settings have no effect on the mask.");

            public static readonly GUIContent detailAlbedoMapText = EditorGUIUtility.TrTextContent("Base Map",
                "Select the surface detail texture.The alpha of your texture determines surface hue and intensity.");

            public static readonly GUIContent detailNormalMapText = EditorGUIUtility.TrTextContent("Normal Map",
                "Designates a Normal Map to create the illusion of bumps and dents in the details of this Material's surface.");

            public static readonly GUIContent detailAlbedoMapScaleInfo = EditorGUIUtility.TrTextContent("Setting the scaling factor to a value other than 1 results in a less performant shader variant.");
            public static readonly GUIContent detailAlbedoMapFormatError = EditorGUIUtility.TrTextContent("This texture is not in linear space.");

            /// SLZ MODIFIED - Style for HDRP style detail mask
            public static readonly GUIContent detailMapText = EditorGUIUtility.TrTextContent("Detail Map",
                "Grayscale overlay blend in the red channel, detail normals in the alpha and green, and smoothness multiplier in blue");
            /// END SLZ MODIFIED
        }

        public struct LitProperties
        {
            public MaterialProperty detailMask;
            /// SLZ MODIFIED - SLZ URP Uses HDRP format detail maps, which get a different property name
            public MaterialProperty detailMap;
            /// END SLZ MODIFIED
            public MaterialProperty detailAlbedoMapScale;
            public MaterialProperty detailAlbedoMap;
            public MaterialProperty detailNormalMapScale;
            public MaterialProperty detailNormalMap;

            public LitProperties(MaterialProperty[] properties)
            {
                detailMask = BaseShaderGUI.FindProperty("_DetailMask", properties, false);
                /// SLZ MODIFIED - SLZ URP Uses HDRP format detail maps, which get a different property name
                detailMap = BaseShaderGUI.FindProperty("_DetailMap", properties, false);
                /// END SLZ MODIFIED
                detailAlbedoMapScale = BaseShaderGUI.FindProperty("_DetailAlbedoMapScale", properties, false);
                detailAlbedoMap = BaseShaderGUI.FindProperty("_DetailAlbedoMap", properties, false);
                detailNormalMapScale = BaseShaderGUI.FindProperty("_DetailNormalMapScale", properties, false);
                detailNormalMap = BaseShaderGUI.FindProperty("_DetailNormalMap", properties, false);
            }
        }

        public static void DoDetailArea(LitProperties properties, MaterialEditor materialEditor)
        {
            /// SLZ MODIFIED - SLZ URP Uses HDRP format detail maps, which get a different property name. Still check for and render old URP style properties though
            if (properties.detailMask != null)
            {
                materialEditor.TexturePropertySingleLine(Styles.detailMaskText, properties.detailMask);
            }
            if (properties.detailAlbedoMap != null)
            {
                materialEditor.TexturePropertySingleLine(Styles.detailAlbedoMapText, properties.detailAlbedoMap,
                properties.detailAlbedoMap.textureValue != null ? properties.detailAlbedoMapScale : null);

                if (properties.detailAlbedoMapScale.floatValue != 1.0f)
                {
                    EditorGUILayout.HelpBox(Styles.detailAlbedoMapScaleInfo.text, MessageType.Info, true);
                }
                var detailAlbedoTexture = properties.detailAlbedoMap.textureValue as Texture2D;
                if (detailAlbedoTexture != null && GraphicsFormatUtility.IsSRGBFormat(detailAlbedoTexture.graphicsFormat))
                {
                    EditorGUILayout.HelpBox(Styles.detailAlbedoMapFormatError.text, MessageType.Warning, true);
                }
                materialEditor.TexturePropertySingleLine(Styles.detailNormalMapText, properties.detailNormalMap,
                    properties.detailNormalMap.textureValue != null ? properties.detailNormalMapScale : null);
                materialEditor.TextureScaleOffsetProperty(properties.detailAlbedoMap);
            }
            if (properties.detailMap != null)
            {
                materialEditor.TexturePropertySingleLine(Styles.detailMapText, properties.detailMap,
                properties.detailMap.textureValue != null ? properties.detailAlbedoMapScale : null);

                if (properties.detailAlbedoMapScale.floatValue != 1.0f)
                {
                    EditorGUILayout.HelpBox(Styles.detailAlbedoMapScaleInfo.text, MessageType.Info, true);
                }
                var detailMapTexture = properties.detailMap.textureValue as Texture2D;
                if (detailMapTexture != null && GraphicsFormatUtility.IsSRGBFormat(detailMapTexture.graphicsFormat))
                {
                    EditorGUILayout.HelpBox(Styles.detailAlbedoMapFormatError.text, MessageType.Warning, true);
                }
                materialEditor.TextureScaleOffsetProperty(properties.detailMap);
            }
        }

        public static void SetMaterialKeywords(Material material)
        {
            if (material.HasProperty("_DetailAlbedoMap") && material.HasProperty("_DetailNormalMap") && material.HasProperty("_DetailAlbedoMapScale"))
            {
                bool isScaled = material.GetFloat("_DetailAlbedoMapScale") != 1.0f;
                bool hasDetailMap = material.GetTexture("_DetailAlbedoMap") || material.GetTexture("_DetailNormalMap");
                CoreUtils.SetKeyword(material, "_DETAIL_MULX2", !isScaled && hasDetailMap);
                CoreUtils.SetKeyword(material, "_DETAIL_SCALED", isScaled && hasDetailMap);
            }
        }
    }
}
