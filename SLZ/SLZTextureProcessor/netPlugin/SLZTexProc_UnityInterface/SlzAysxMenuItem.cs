using System;
using System.Collections.Generic;
using UnityEditor.ProjectWindowCallback;
using UnityEditor;
using UnityEngine;

namespace SLZ.SLZTextureProcessor
{
    internal static class SlzAysxMenuItem
    {
        [MenuItem("Assets/Create/Rendering/Fused Normal AO Smoothness (AYSX) Map")]
        internal static void MenuItem()
        {
            UnityEngine.Object[] selection = Selection.objects;
            string defaultName = "texture";
            Texture2D normalMap = null;
            Texture2D masMap = null;

            if (selection.Length > 1 && selection[0] is Texture2D tex0 && selection[1] is Texture2D tex1)
            {
                SerializedObject tex0Serialized = new SerializedObject(tex0);
                SerializedProperty tex0lmFormat = tex0Serialized.FindProperty("m_LightmapFormat");
                if (tex0lmFormat.intValue > 0)
                {
                    //Debug.Log($"{tex0.name} lightmap flags are {tex0lmFormat.intValue}");
                    normalMap = tex0;
                    masMap = tex1;
                }
                else
                {
                    SerializedObject tex1Serialized = new SerializedObject(tex1);
                    SerializedProperty tex1lmFormat = tex1Serialized.FindProperty("m_LightmapFormat");
                    //Debug.Log($"{tex0.name} lightmap flags are {tex0lmFormat.intValue}\n{tex1.name} lightmap flags are {tex1lmFormat.intValue}");
                    if (tex1lmFormat.intValue > 0)
                    {
                        normalMap = tex1;
                        masMap = tex0;
                    }
                    tex1Serialized.Dispose();
                }
                tex0Serialized.Dispose();

                if (normalMap != null && !string.IsNullOrEmpty(normalMap.name))
                {
                    defaultName = AysxUtil.GetAysxNameFromNormalMap(normalMap.name);
                }
            }


            string filename = defaultName + ".aysx";
            DoCreateAysx doCreateAysx = ScriptableObject.CreateInstance<DoCreateAysx>();
            doCreateAysx.normalMap = normalMap;
            doCreateAysx.masMap = masMap;

            doCreateAysx.filecontent = AysxUtil.GetAysxFileContent(normalMap, masMap);
            ProjectWindowUtil.StartNameEditingIfProjectWindowExists(0, doCreateAysx, filename, null, null);
        }

        internal class DoCreateAysx : EndNameEditAction
        {
            public string filecontent;
            public Texture2D normalMap;
            public Texture2D masMap;



            public override void Action(int instanceId, string pathName, string resourceFile)
            {
                /*
                UnityEngine.Object o = AysxUtil.CreateScriptAssetWithContent(pathName, filecontent);
                SlzAysxTextureImporter importer = (SlzAysxTextureImporter)AssetImporter.GetAtPath(pathName);
                importer.normalMapRef = new LazyLoadReference<Texture2D>(normalMap);
                importer.masMapRef = new LazyLoadReference<Texture2D>(masMap);
                EditorUtility.SetDirty(importer);
                importer.SaveAndReimport();
                */
                (SlzAysxTextureImporter importer, UnityEngine.Object o) = AysxUtil.CreateAysx(pathName, filecontent, normalMap, masMap, true);
                ProjectWindowUtil.ShowCreatedAsset(o);
            }
        }
    }
}
