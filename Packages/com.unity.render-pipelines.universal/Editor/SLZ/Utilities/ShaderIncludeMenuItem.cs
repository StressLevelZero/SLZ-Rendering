using System;
using System.IO;
using System.Text;
using System.Reflection;
using System.Collections;
using System.Collections.Generic;
using UnityEditor.ProjectWindowCallback;
using UnityEditor;
using UnityEngine;
using UnityEditorInternal;
using System.Globalization;

namespace SLZ.SLZEditorTools
{
    public static class ShaderIncludeMenuItem
    {
        const string defaultName = "include.hlsl";
        [MenuItem("Assets/Create/Shader/HLSL Include")]
        static void MenuItem()
        {
            DoCreateHLSL action = new();
            ProjectWindowUtil.StartNameEditingIfProjectWindowExists(0, action, defaultName, null, null);
        }

        internal class DoCreateHLSL : EndNameEditAction
        {
            public override void Action(int instanceId, string pathName, string resourceFile)
            {
                string hlslPath = pathName.Substring(0, pathName.LastIndexOf('.')) + ".hlsl";
                string fullHlslPath = Path.GetFullPath(hlslPath);
                string hlslName = Path.GetFileNameWithoutExtension(pathName);
                string incGuardName = FileNameToCStyleDef(hlslName);
                File.WriteAllText(fullHlslPath, 
                $"#if !defined({incGuardName})\n"+
                $"#define {incGuardName}\n"+
                "\n"+
                "#endif\n"
                );
                AssetDatabase.ImportAsset(hlslPath, ImportAssetOptions.ForceUpdate | ImportAssetOptions.ForceSynchronousImport);
                ProjectWindowUtil.ShowCreatedAsset(AssetDatabase.LoadMainAssetAtPath(hlslPath));
            }
        }

        static string FileNameToCStyleDef(ReadOnlySpan<char> name)
        {
            int charCount = name.Length;
            StringBuilder sb = new StringBuilder(name.Length * 2 + 9);
            bool isPrevCharCaps = char.IsUpper(name[0]);
            int splitStart = 0;
            for (int i = 0; i < charCount; i++)
            {
                bool isThisCharCaps = char.IsUpper(name[i]);
                bool isLast = i == (charCount - 1);
                if ((isThisCharCaps && !isPrevCharCaps))
                {
                    int splitCount = i - splitStart;
                    ReadOnlySpan<char> substring = name.Slice(splitStart, splitCount);
                    Span<char> upperSubString = stackalloc char[splitCount];
                    substring.ToUpper(upperSubString, CultureInfo.InvariantCulture);
                    if (splitStart != 0)
                    {
                        sb.Append('_');
                    }
                    sb.Append(upperSubString);
                    splitStart = i;
                }
                isPrevCharCaps = isThisCharCaps;
            }
            //if (splitStart < (charCount - 1))
            {
                int splitCount = charCount - splitStart;
                ReadOnlySpan<char> substring = name.Slice(splitStart, splitCount);
                Span<char> upperSubString = stackalloc char[splitCount];
                substring.ToUpper(upperSubString, CultureInfo.InvariantCulture);
                if (splitStart != 0)
                {
                    sb.Append('_');
                }
                sb.Append(upperSubString);
            }
            sb.Append("_INCLUDED");
            return sb.ToString();
        }

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

    }
}
