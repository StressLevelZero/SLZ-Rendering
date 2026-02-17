#if __STDC_VERSION__ || __cplusplus 

#define public

#else

#define DOTNET
using System;
using System.Runtime.InteropServices;

namespace SLZ.SLZTextureProcessor
{
    public static class NativeShared
    {
#endif
        public enum TXPErrorCode
#if DOTNET
            : Int32
#endif
        {
            TXP_RETURN_SUCCESS = 0,
            TXP_RETURN_GENERAL_FAILURE,
            TXP_RETURN_ALLOC_FAILED,
            TXP_RETURN_INVALID_ARGS,
            TXP_RETURN_INVALID_TEX_FORMAT,
            TXP_RETURN_ZERO_SIZE_TEX,
            TXP_RETURN_IMG_LIBRARY_FAILED,
            TXP_RETURN_NO_IMG_LIBRARY,
            TXP_RETURN_INVALID_IMG_LIBRARY,
            TXP_RETURN_INVALID_PATH,
            TXP_RETURN_FILE_OPEN_FAILED
        };

        public enum TxpTextureFormat
#if DOTNET
            : Int32
#endif
        {
            FMT_UNKNOWN = 0,
            FMT_R8,
            FMT_RG8,
            FMT_RGB8,
            FMT_RGBA8,
            FMT_R16,
            FMT_RG16,
            FMT_RGB16,
            FMT_RGBA16,
            FMT_RHalf,
            FMT_RGHalf,
            FMT_RGBHalf,
            FMT_RGBAHalf,
            FMT_RFloat,
            FMT_RGFloat,
            FMT_RGBFloat,
            FMT_RGBAFloat
        };

        public enum TxpTextureChannelFmt
#if DOTNET
            : Int32
#endif
        {
            CHANNEL_FMT_UNKNOWN,
            CHANNEL_FMT_FIXED8,
            CHANNEL_FMT_FIXED16,
            CHANNEL_FMT_FLOAT16,
            CHANNEL_FMT_FLOAT32
        };

        public enum UnityLogLevel
#if DOTNET
            : Int32
#endif
        {
            UNITY_LOG_LEVEL_LOG = 0,
            UNITY_LOG_LEVEL_WARN = 1,
            UNITY_LOG_LEVEL_ERROR = 2
        };

#if !DOTNET
        typedef enum TXPErrorCode TXPErrorCode;
        typedef enum TxpTextureFormat TxpTextureFormat;
        typedef enum TxpTextureChannelFmt TxpTextureChannelFmt;
        typedef enum UnityLogLevel UnityLogLevel;
#endif

#if DOTNET
        [StructLayout(LayoutKind.Sequential, Pack=4, Size=32)]
#endif
        public struct TxpTex2D
        {
            public TxpTextureFormat format;
            public Int32 mipCount;
#if DOTNET
            public IntPtr resolution;
            public IntPtr mips;
#else
            int2* resolution;
            void** mips;
#endif
            public Int64 pad0;
        };


#if DOTNET
        [StructLayout(LayoutKind.Sequential)]
        public struct PathString
        {
            public int length;
            public IntPtr s;
        };
#endif

#if DOTNET
        [StructLayout(LayoutKind.Sequential)]
#endif
        public struct ExportImageInfo
        {
            public Int32 width;
            public Int32 height;
            public TxpTextureFormat textureFormat;
            public Int32 unused;
#if DOTNET
            public IntPtr ImageIOHandler;
#else
            void* ImageIOHandler;
#endif
        };

#if !DOTNET
        typedef struct TxpTex2D TxpTex2D;
        typedef struct ExportImageInfo ExportImageInfo;
#endif

#if DOTNET
    }
}
#else
#undef public

#endif