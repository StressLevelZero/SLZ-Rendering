using System;
using SLZ.SLZEditorTools;
using Debug = UnityEngine.Debug;
using Unity.Collections.LowLevel.Unsafe;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering.Universal;
using UnityEngine.Rendering;

#if false
using System.Runtime.InteropServices;
using System.IO;
using System.Diagnostics;
#endif

namespace SLZ.DXCUpdater
{
    internal class DXCUnitTests
    {

        public struct Results
        {
            public bool hasSelect;
            public bool hasTemplates;
        }

        static bool RunShaderUnitTest(string shader)
        {
            bool result = false;
            try
            {
            Shader testShader = ShaderUtil.CreateShaderAsset(null, shader, true);
            if (testShader == null) Debug.Log("no shader created");
            Material mat = new Material(testShader);
            ShaderUtil.CompilePass(mat, 0, true);
            result = !ShaderUtil.ShaderHasError(testShader);
          
            UnityEngine.Object.DestroyImmediate(testShader);
            UnityEngine.Object.DestroyImmediate(mat);
            }
            catch (Exception ex)
            {
                Debug.LogError(ex.Message);
            }
            return result;
        }


        public static Results RunUnitTests()
        {
            Results results = new Results();
            string templateTest =
"Shader \"UNIT_TESTS/TEST-TEMPLATES\"\n" +
"{\n" +
"    SubShader { \n" +
"        Pass {\n" +
"            Tags {\"Lightmode\"=\"UniversalForward\"}\n" +
"            HLSLPROGRAM\n" +
"            #pragma use_dxc\n" +
"            #pragma vertex mainVtx\n" +
"            #pragma fragment mainFrag\n" +
"            template<typename TEST_TYPE> TEST_TYPE TestTemplate(TEST_TYPE value) { return 2 * value; } \n" +
"            float4 mainVtx (float4 position : POSITION) : SV_Position  { return TestTemplate(position);  }\n" +
"            float4 mainFrag(float4 position : SV_Position) : SV_Target { return TestTemplate(position);  }  \n" +
"            ENDHLSL\n" +
"        }\n" +
"    }\n" +
"}\n" 
            ;
            results.hasTemplates = RunShaderUnitTest(templateTest);

            string selectTest =
"Shader \"UNIT_TESTS/TEST-SELECT\"\n" +
"{\n" +
"    SubShader { \n" +
"        Pass {\n" +
"            Tags {\"Lightmode\"=\"UniversalForward\"}\n" +
"            HLSLPROGRAM\n" +
"            #pragma use_dxc\n" +
"            #pragma vertex mainVtx\n" +
"            #pragma fragment mainFrag\n" +
"            float4 mainVtx (float4 position : POSITION) : SV_Position  { return select(position > 0, position, -position);  }\n" +
"            void mainFrag(float4 position : SV_Position) { }  \n" +
"            ENDHLSL\n" +
"        }\n" +
"    }\n" +
"}\n" 
            ;
            results.hasSelect = RunShaderUnitTest(selectTest);
            return results;
        }

        #region UnusedNative
        // Was going to invoke DXC directly, but realized I could use ShaderUtils. Leaving this here in case I do need to invoke DXC directly at some point
        /*
        const Int32 DXC_CP_ACP    = 0;
        const Int32 DXC_CP_UTF8   = 65001;
        const Int32 DXC_CP_UTF16  = 1200;
        const Int32 DXC_CP_UTF32  = 12000;

        delegate int d_DxcCreateInstance(
            in Guid rclsid,
            in Guid riid,
            out void* ppv
        ); 

        private static readonly Guid CLSID_DxcCompiler = new Guid("73e22d93-e6ce-47f3-b5bf-f0664f39c1b0");
        private static readonly Guid IID_IDxcCompiler3 = new Guid("228B4687-5A6A-4730-900C-9702B2203F54");

        [StructLayout(LayoutKind.Sequential)]
        struct DxcBuffer
        {
            public IntPtr   ptr;
            public UIntPtr  size;
            public uint     encoding;
        }

        [ComImport]
        [Guid("228B4687-5A6A-4730-900C-9702B2203F54")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        interface IDxcCompiler3
        {
            [PreserveSig]
            unsafe int Compile(
                in DxcBuffer pSource,
                [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPWStr)] string[] pArguments,
                uint argCount,
                IntPtr pIncludeHandler,
                in Guid riid,
                out void* ppResult
            );
        }
        */
        #endregion
    }
}
