// Derived from https://github.com/keijiro/Akvfx/blob/master/Packages/jp.keijiro.akvfx/Runtime/Internal/Extensions.cs

using UnityEngine;
using System;
using System.Reflection;
using Unity.Collections.LowLevel.Unsafe;

namespace UnityEngine.Rendering.Universal
{
	public static class ComputeBufferExtensions
	{
		// SetData with ReadOnlySpan
		public unsafe static void SetDataSpanExt<T>
		  (this ComputeBuffer buffer, ReadOnlySpan<T> data) where T : unmanaged
		{
			fixed (T* pData = &data.GetPinnableReference())
				buffer.SetDataExt((IntPtr)pData, data.Length, UnsafeUtility.SizeOf<T>());
		}

		public unsafe static void SetDataSpanExt<T>
		  (this GraphicsBuffer buffer, ReadOnlySpan<T> data) where T : unmanaged
		{
			fixed (T* pData = &data.GetPinnableReference())
				buffer.SetDataExt((IntPtr)pData, data.Length, UnsafeUtility.SizeOf<T>());
		}

		public unsafe static void SetBufferDataSpanExt<T>
		  (this CommandBuffer cmd, ComputeBuffer buffer, ReadOnlySpan<T> data) where T : unmanaged
		{
			fixed (T* pData = &data.GetPinnableReference())
				cmd.CmdSetDataExt(buffer, (IntPtr)pData, data.Length, UnsafeUtility.SizeOf<T>());
		}

		public unsafe static void SetBufferDataSpanExt<T>
		  (this CommandBuffer cmd, GraphicsBuffer buffer, ReadOnlySpan<T> data) where T : unmanaged
		{
			fixed (T* pData = &data.GetPinnableReference())
				cmd.CmdSetDataExt(buffer, (IntPtr)pData, data.Length, UnsafeUtility.SizeOf<T>());
		}

		public unsafe static void SetBufferDataSpanExt<T>
		  (this CommandBuffer cmd, ComputeBuffer buffer, ReadOnlySpan<T> data, int dataStartIndex, int bufferStartIndex, int count) where T : unmanaged
		{
			fixed (T* pData = &data.GetPinnableReference())
				SetCmdCBNativeDelegate(cmd, buffer, (IntPtr)pData, dataStartIndex, bufferStartIndex, count, sizeof(T));
		}

		public unsafe static void SetBufferDataSpanExt<T>
		  (this CommandBuffer cmd, GraphicsBuffer buffer, ReadOnlySpan<T> data, int dataStartIndex, int bufferStartIndex, int count) where T : unmanaged
		{
			fixed (T* pData = &data.GetPinnableReference())
				SetCmdGfxNativeDelegate(cmd, buffer, (IntPtr)pData, dataStartIndex, bufferStartIndex, count, sizeof(T));
		}

		public unsafe static void SetBufferDataSpanExt<T>
		  (this UnsafeCommandBuffer cmd, ComputeBuffer buffer, ReadOnlySpan<T> data) where T : unmanaged
		{
			fixed (T* pData = &data.GetPinnableReference())
				cmd.m_WrappedCommandBuffer.CmdSetDataExt(buffer, (IntPtr)pData, data.Length, sizeof(T));
		}

		public unsafe static void SetBufferDataSpanExt<T>
		  (this UnsafeCommandBuffer cmd, GraphicsBuffer buffer, ReadOnlySpan<T> data) where T : unmanaged
		{
			fixed (T* pData = &data.GetPinnableReference())
				cmd.m_WrappedCommandBuffer.CmdSetDataExt(buffer, (IntPtr)pData, data.Length, sizeof(T));
		}

		public unsafe static void SetBufferDataSpanExt<T>
		  (this UnsafeCommandBuffer cmd, ComputeBuffer buffer, ReadOnlySpan<T> data, int dataStartIndex, int bufferStartIndex, int count) where T : unmanaged
		{
			fixed (T* pData = &data.GetPinnableReference())
				SetCmdCBNativeDelegate(cmd.m_WrappedCommandBuffer, buffer, (IntPtr)pData, dataStartIndex, bufferStartIndex, count, sizeof(T));
		}

		public unsafe static void SetBufferDataSpanExt<T>
		  (this UnsafeCommandBuffer cmd, GraphicsBuffer buffer, ReadOnlySpan<T> data, int dataStartIndex, int bufferStartIndex, int count) where T : unmanaged
		{
			fixed (T* pData = &data.GetPinnableReference())
				SetCmdGfxNativeDelegate(cmd.m_WrappedCommandBuffer, buffer, (IntPtr)pData, dataStartIndex, bufferStartIndex, count, sizeof(T));
		}

		// Directly load an unmanaged data array to a compute buffer via an
		// Intptr. This is not a public interface so will be broken one day.
		// DO NOT TRY AT HOME.
		public static void SetDataExt
		  (this ComputeBuffer buffer, IntPtr pointer, int count, int stride)
		{
			/*
			_args5[0] = pointer;
			_args5[1] = 0;      // source offset
			_args5[2] = 0;      // buffer offset
			_args5[3] = count;
			_args5[4] = stride;

			SetNativeData.Invoke(buffer, _args5);
			*/
			SetCBNativeDelegate(buffer, pointer, 0, 0, count, stride);
		}

		public static void SetDataExt
		  (this GraphicsBuffer buffer, IntPtr pointer, int count, int stride)
		{
			SetGfxNativeDelegate(buffer, pointer, 0, 0, count, stride);
		}

		public static void CmdSetDataExt
		(this CommandBuffer cmd, ComputeBuffer buffer, IntPtr pointer, int count, int stride)
		{
			/*
			_args6[0] = buffer;
			_args6[1] = pointer;
			_args6[2] = 0;      // source offset
			_args6[3] = 0;      // buffer offset
			_args6[4] = count;
			_args6[5] = stride;

			CmdSetNativeData.Invoke(cmd, _args6);
			*/
			SetCmdCBNativeDelegate(cmd, buffer, pointer, 0, 0, count, stride);
		}

		public static void CmdSetDataExt
		(this CommandBuffer cmd, GraphicsBuffer buffer, IntPtr pointer, int count, int stride)
		{
			SetCmdGfxNativeDelegate(cmd, buffer, pointer, 0, 0, count, stride);
		}

		static MethodInfo _setNativeData;
		static MethodInfo _cmdSetNativeData;

		static MethodInfo SetNativeData
		  => _setNativeData ?? (_setNativeData = CBGetSetNativeDataMethod());

		static MethodInfo CmdSetNativeData
			=> _cmdSetNativeData ?? (_cmdSetNativeData = GetCmdCBSetNativeDataMethod());

		static MethodInfo CBGetSetNativeDataMethod()
		  => typeof(ComputeBuffer).GetMethod("InternalSetNativeData",
											 BindingFlags.InvokeMethod |
											 BindingFlags.NonPublic |
											 BindingFlags.Instance);
		static MethodInfo GetCmdCBSetNativeDataMethod()
		  => typeof(CommandBuffer).GetMethod("InternalSetComputeBufferNativeData",
											 BindingFlags.InvokeMethod |
											 BindingFlags.NonPublic |
											 BindingFlags.Instance);
		static MethodInfo GfxGetSetNativeDataMethod()
		  => typeof(GraphicsBuffer).GetMethod("InternalSetNativeData",
											 BindingFlags.InvokeMethod |
											 BindingFlags.NonPublic |
											 BindingFlags.Instance);
		static MethodInfo GetCmdGfxSetNativeDataMethod()
		  => typeof(CommandBuffer).GetMethod("InternalSetGraphicsBufferNativeData",
											 BindingFlags.InvokeMethod |
											 BindingFlags.NonPublic |
											 BindingFlags.Instance);

		static Action<ComputeBuffer, IntPtr, int, int, int, int> _setCBNativeDelegate;
		static Action<GraphicsBuffer, IntPtr, int, int, int, int> _setGfxNativeDelegate;
		static Action<CommandBuffer, ComputeBuffer, IntPtr, int, int, int, int> _setCmdCBNativeDelegate;
		static Action<CommandBuffer, GraphicsBuffer, IntPtr, int, int, int, int> _setCmdGfxNativeDelegate;

		static Action<ComputeBuffer, IntPtr, int, int, int, int> GetCBNativeDelegate()
		{
			MethodInfo method = CBGetSetNativeDataMethod();
			return (Action<ComputeBuffer, IntPtr, int, int, int, int>)Delegate.CreateDelegate(typeof(Action<ComputeBuffer, IntPtr, int, int, int, int>), method);
		}

		static Action<CommandBuffer, ComputeBuffer, IntPtr, int, int, int, int> GetCmdCBNativeDelegate()
		{
			MethodInfo method = GetCmdCBSetNativeDataMethod();
			return (Action<CommandBuffer, ComputeBuffer, IntPtr, int, int, int, int>)Delegate.CreateDelegate(typeof(Action<CommandBuffer, ComputeBuffer, IntPtr, int, int, int, int>), method);
		}

		static Action<GraphicsBuffer, IntPtr, int, int, int, int> GetGfxNativeDelegate()
		{
			MethodInfo method = GfxGetSetNativeDataMethod();
			return (Action<GraphicsBuffer, IntPtr, int, int, int, int>)Delegate.CreateDelegate(typeof(Action<GraphicsBuffer, IntPtr, int, int, int, int>), method);
		}

		static Action<CommandBuffer, GraphicsBuffer, IntPtr, int, int, int, int> GetCmdGfxNativeDelegate()
		{
			MethodInfo method = GetCmdGfxSetNativeDataMethod();
			return (Action<CommandBuffer, GraphicsBuffer, IntPtr, int, int, int, int>)Delegate.CreateDelegate(typeof(Action<CommandBuffer, GraphicsBuffer, IntPtr, int, int, int, int>), method);
		}

		static Action<				 ComputeBuffer,  IntPtr, int, int, int, int> SetCBNativeDelegate 	 => _setCBNativeDelegate     ?? (_setCBNativeDelegate = GetCBNativeDelegate());
		static Action<				 GraphicsBuffer, IntPtr, int, int, int, int> SetGfxNativeDelegate 	 => _setGfxNativeDelegate    ?? (_setGfxNativeDelegate = GetGfxNativeDelegate());
		static Action<CommandBuffer, ComputeBuffer,  IntPtr, int, int, int, int> SetCmdCBNativeDelegate	 => _setCmdCBNativeDelegate  ?? (_setCmdCBNativeDelegate = GetCmdCBNativeDelegate());
		static Action<CommandBuffer, GraphicsBuffer, IntPtr, int, int, int, int> SetCmdGfxNativeDelegate => _setCmdGfxNativeDelegate ?? (_setCmdGfxNativeDelegate = GetCmdGfxNativeDelegate());

		//static object[] _args5 = new object[5];
		//static object[] _args6 = new object[6];
	}

}
