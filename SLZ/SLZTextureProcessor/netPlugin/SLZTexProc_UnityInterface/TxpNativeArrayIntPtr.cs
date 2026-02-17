using System;
using Unity.Collections;
using Unity.Collections.LowLevel.Unsafe;

namespace SLZ.SLZTextureProcessor
{
	internal static unsafe class TxpNativeArrayIntPtr
	{
		public static unsafe IntPtr GetIntPtr<T>(NativeArray<T> array)
			where T : unmanaged
		{
			void* dataPointer = array.GetUnsafePtr();
			return (IntPtr)dataPointer;
		}
	}
}
