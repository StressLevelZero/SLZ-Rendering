using System.Collections;
using System.IO;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEditor;
using UnityEngine.Experimental.Rendering;
using Unity.Collections;
using UnityEditor.AssetImporters;
using static SLZ.SLZTextureProcessor.NativeShared;
using static SLZ.SLZTextureProcessor.NativeBindings;

using Debug = UnityEngine.Debug;
using UnityEditor.Build;
using Unity.Mathematics;
using System.Reflection;
using UnityEditor.ProjectWindowCallback;
using UnityEditorInternal;

namespace SLZ.SLZTextureProcessor
{
    public static class AysxUtil
    {
        public delegate UnityEngine.Object d_CreateScriptAssetWithContent(string pathName, string templateContent);
        static d_CreateScriptAssetWithContent s_CreateScriptAssetWithContent;
        public static d_CreateScriptAssetWithContent CreateScriptAssetWithContent
        {
            get
            {
                if (s_CreateScriptAssetWithContent == null)
                {
                    MethodInfo mi = typeof(ProjectWindowUtil).GetMethod("CreateScriptAssetWithContent", BindingFlags.Static | BindingFlags.NonPublic);
                    if (mi == null)
                    {
                        throw new System.Exception("CreateScriptAssetWithContent not found in ProjectWindowUtil");
                    }
                    s_CreateScriptAssetWithContent = (d_CreateScriptAssetWithContent)mi.CreateDelegate(typeof(d_CreateScriptAssetWithContent));
                }
                return s_CreateScriptAssetWithContent;
            }
        }

        public static string GetAysxNameFromNormalMap(ReadOnlySpan<char> normalMapName)
        {
            bool hasSuffix = false;
          
            int underscoreIdx = normalMapName.LastIndexOf('_');

            if (underscoreIdx != -1)
            {
                ReadOnlySpan<char> suffix = normalMapName.Slice(underscoreIdx + 1);
                switch (suffix.Length)
                {
                    case 1: hasSuffix = suffix.Equals("n",      StringComparison.InvariantCultureIgnoreCase); break;
                    case 2: hasSuffix = suffix.Equals("nm",     StringComparison.InvariantCultureIgnoreCase); break;
                    case 3: hasSuffix = suffix.Equals("nor",    StringComparison.InvariantCultureIgnoreCase) ||
                                        suffix.Equals("nrm",    StringComparison.InvariantCultureIgnoreCase); break;
                    case 4: hasSuffix = suffix.Equals("norm",   StringComparison.InvariantCultureIgnoreCase); break;
                    case 6: hasSuffix = suffix.Equals("normal", StringComparison.InvariantCultureIgnoreCase); break;
                }
            }
            if (hasSuffix)
            {
                return normalMapName.Slice(0, underscoreIdx).ToString() + "_aysx";
            }
            else
            {
                return normalMapName.ToString() + "_aysx";
            }
        }

        /// <summary>
        /// Creates a new AYSX file with the given contents, and assigns the specified normal and mas maps to the importer
        /// </summary>
        /// <param name="path">Path to save the .aysx file. Must end in '.aysx'!</param>
        /// <param name="fileContent">String to write to the .aysx file. This should be the string returned from <see cref="GetAysxFileContent"/>, but technically it can be anything as it has no effect on the actual imported texture. However it also should not be empty to avoid issues with unity's import loop detection.</param>
        /// <param name="normalMap">Normal map texture</param>
        /// <param name="masMap">MAS texture</param>
        /// <param name="reimport">Reimport immediately after assigning the normal and MAS textures. If false, you must call <see cref="AssetImporter.SaveAndReimport"/> on the returned importer</param>
        /// <returns>Pair containing the <see cref="SlzAysxTextureImporter"/> belonging to the new aysx texture, and a <see cref="UnityEngine.Object"/> which can be used to highlight the asset in the project view with <see cref="ProjectWindowUtil.ShowCreatedAsset"/></returns>
        public static (SlzAysxTextureImporter, UnityEngine.Object) CreateAysx(string path, string fileContent, Texture2D normalMap, Texture2D masMap, bool reimport = true)
        {
            UnityEngine.Object o = AysxUtil.CreateScriptAssetWithContent(path, fileContent);
            SlzAysxTextureImporter importer = (SlzAysxTextureImporter)AssetImporter.GetAtPath(path);
            if (normalMap || masMap)
            {
                importer.normalMapRef = new LazyLoadReference<Texture2D>(normalMap);
                importer.masMapRef = new LazyLoadReference<Texture2D>(masMap);
                EditorUtility.SetDirty(importer);
                if (reimport)
                {
                    importer.SaveAndReimport();
                }
            }
            return (importer, o);
        }

        /// <summary>
        /// Creates an aysx texture at a given path from the given normal and mas textures, and returns the <see cref="SlzAysxTextureImporter"/> belonging to the new texture
        /// </summary>
        /// <param name="path">Path to save the file</param>
        /// <param name="normalMap">normal map to assign to the aysx</param>
        /// <param name="masMap">mas map to assign to the aysx</param>
        /// <param name="reimport">Reimport immediately after assigning the normal and MAS textures. If false, you must call <see cref="AssetImporter.SaveAndReimport"/> on the returned importer</param>
        /// <returns></returns>
        public static SlzAysxTextureImporter CreateAysxAndGetImporter(string path, Texture2D normalMap, Texture2D masMap, bool reimport = true)
        {
            string fileContent = GetAysxFileContent(normalMap, masMap);
            (SlzAysxTextureImporter importer, UnityEngine.Object o) = CreateAysx(path, fileContent, normalMap, masMap, reimport);
            return importer;
        }


        public static string GetAysxFileContent(Texture2D normalMap, Texture2D masMap)
        {
            string normalMapGUID = "";
            long normalMapID = 0;
            string masMapGUID = "";
            long masMapID = 0;
            if (normalMap)
            {
                AssetDatabase.TryGetGUIDAndLocalFileIdentifier(normalMap, out normalMapGUID, out normalMapID);
            }
            if (masMap)
            {
                AssetDatabase.TryGetGUIDAndLocalFileIdentifier(masMap, out masMapGUID, out masMapID);
            }

            return  $"{{\n" +
                    $"  \"normalMap\": {{\n" +
                    $"    \"guid\": \"{normalMapGUID}\",\n" +
                    $"    \"fileID\": {normalMapID}\n" +
                    $"  }},\n" +
                    $"  \"masMap\": {{\n" +
                    $"    \"guid:\": \"{masMapGUID}\",\n" +
                    $"    \"fileID\": {masMapID}\n" +
                    $"  }}\n" +
                    $"}}\n";
        }
    }
   
}
