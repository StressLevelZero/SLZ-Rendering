using System;
using System.Runtime.InteropServices;
using UnityEngine;
using AOT;
using static SLZ.SLZTextureProcessor.NativeShared;
using UnityEditor;

namespace SLZ.SLZTextureProcessor
{
    public class SLZTexProcPluginLogger
    {
        const string PLUGIN_NAME = "SLZTextureProcessor";



        [DllImport(PLUGIN_NAME)]
        private static extern void TxpSetLogger(IntPtr logMethodPtr);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void DebugLogDelegate(int level, string message);

        static DebugLogDelegate dlDelegate;

#if UNITY_EDITOR
        [InitializeOnLoadMethod]
#else
[RuntimeInitializeOnLoadMethod]
#endif
        public static void InitializeLogger()
        {
            dlDelegate = new DebugLogDelegate(DebugLog);
            IntPtr methodPtr = Marshal.GetFunctionPointerForDelegate(dlDelegate);
            TxpSetLogger(methodPtr);
        }

        [MonoPInvokeCallback(typeof(DebugLogDelegate))]
        static void DebugLog(int level, string message)
        {
            switch (level)
            {
                case (int)UnityLogLevel.UNITY_LOG_LEVEL_LOG:
                    Debug.Log(message);
                    break;
                case (int)UnityLogLevel.UNITY_LOG_LEVEL_WARN:
                    Debug.LogWarning(message);
                    break;
                case (int)UnityLogLevel.UNITY_LOG_LEVEL_ERROR:
                    Debug.LogError(message);
                    break;
                default:
                    Debug.Log(message);
                    break;
            }
        }
    }
}