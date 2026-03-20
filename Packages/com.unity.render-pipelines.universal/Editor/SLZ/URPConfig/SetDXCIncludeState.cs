using SLZ.SLZEditorTools;
using System;
using System.IO;
using UnityEngine;

namespace SLZ.DXCUpdater
{
    public static class SetDXCIncludeState
    {
        static string packageName = "com.stresslevelzero.urpconfig";
        public static bool Set(bool patched, uint major, uint minor, uint patch, uint build)
        {
            //Debug.Log($"Setting DXCUpdateState");
            URPConfigManager.Initialize();
            string includePath = Path.Combine(URPConfigManager.packagePath, "include", "DXCUpdateState.hlsl");
            //Debug.Log($"DXCUpdateState path: {includePath}");
            if (!File.Exists(includePath))
            {
                Debug.LogError($"Critical shader include file is missing ({includePath})");
                return false;
            }

            string comment = patched ? "" : "//";
            int patchedInt = patched ? 1 : 0;
            uint versionNum = ((major & 0x1FFU) << 22) | ((minor & 0x3FFU) << 12) | (patch & 0xFFFU);
            string file =
            $"#ifndef SLZ_DXC_STATE\n" +
            $"\t#define SLZ_DXC_STATE\n" +
            $"\t{comment}#define SLZ_DXC_UPDATED {patchedInt}\n" +
            $"\t#define SLZ_DXC_VERSION 0x{versionNum.ToString("x08")}\n" +
                $"\t#define SLZ_DXC_VERSION_MAJOR {major}\n" +
                $"\t#define SLZ_DXC_VERSION_MINOR {minor}\n" +
                $"\t#define SLZ_DXC_VERSION_PATCH {patch}\n" +
                $"\t#define SLZ_DXC_VERSION_BUILD {build}\n" +
                $"#endif";
            string original = File.ReadAllText(includePath);
            if (!string.Equals(original, file, System.StringComparison.InvariantCulture))
            {
                Debug.Log($"DXCUpdateState needs to be updated!");
                File.WriteAllText(includePath, file);
            }

            return true;
        }
    }
}
