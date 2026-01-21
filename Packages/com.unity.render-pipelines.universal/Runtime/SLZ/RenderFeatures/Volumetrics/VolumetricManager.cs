using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.CompilerServices;

using Unity.Burst;
using Unity.Collections;

using Unity.Mathematics;
using Unity.Mathematics.Geometry;
using static Unity.Mathematics.math;

using Debug = UnityEngine.Debug;
using UnityEngine.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;


#if UNITY_EDITOR
using UnityEditor;
#endif

namespace SLZRendering.Runtime
{
    [BurstCompile]
    public static class VolumetricManager
    {
        public static List<BakedVolumetricArea> volumes;
        public static List<RTHandle> volumeTexHandles;
        public static NativeList<MinMaxAABB> volumeBounds;
        public static int volumeCount = 0;

        /// <summary>
        /// Handles disposing native arrays on reinitialization/domain reload.
        /// </summary>
        class VolumetricNativeMemoryManager : IDisposable
        {
            public NativeList<MinMaxAABB> areaBounds;

            public bool disposed = false;
            public void Dispose()
            {
                if (areaBounds.IsCreated)
                {
                    areaBounds.Dispose();
                }
                disposed = true;
            }
        }

        static VolumetricNativeMemoryManager nativeMemoryManager;

#if UNITY_EDITOR
        [InitializeOnLoadMethod]
#endif
        [RuntimeInitializeOnLoadMethod]
        static void Initialize()
        {
            InitializeArrays();
            SceneManager.sceneUnloaded += CleanupOnSceneUnload;
#if UNITY_EDITOR
            EditorApplication.playModeStateChanged += CleanupOnPlaymodeChanged;
            AssemblyReloadEvents.beforeAssemblyReload += DisposeNativeMemory;
#endif
        }

        public static void CleanupOnSceneUnload(Scene scene)
        {
            GarbageCollectVolumes();
        }

#if UNITY_EDITOR
        public static void CleanupOnPlaymodeChanged(PlayModeStateChange state)
        {
            InitializeArrays();
        }

        public static void DisposeNativeMemory()
        {
            if (nativeMemoryManager != null)
            {
                nativeMemoryManager.Dispose();
                nativeMemoryManager = null;
            }
        }
#endif

        /// <summary>
        /// Create empty lists to contain the scene volumes and their bounds. Disposes of any previous information contained in the arrays.
        /// </summary>
        public static void InitializeArrays()
        {
            volumes = new List<BakedVolumetricArea>(64);
            volumeTexHandles = new List<RTHandle>(64);
            if (nativeMemoryManager == null || nativeMemoryManager.disposed)
            {
                nativeMemoryManager = new VolumetricNativeMemoryManager();
            }
            if (nativeMemoryManager.areaBounds.IsCreated)
            {
                nativeMemoryManager.areaBounds.Dispose();
            }
            volumeCount = 0;
            nativeMemoryManager.areaBounds = new NativeList<MinMaxAABB>(64, Allocator.Persistent);
            volumeBounds = nativeMemoryManager.areaBounds;
        }

        /// <summary>
        /// Remove all volumes from the manager.
        /// </summary>
        public static void PurgeVolumes()
        {
            volumes.Clear();
            nativeMemoryManager.areaBounds.Clear();
            volumeCount = 0;
        }

        /// <summary>
        /// Register a volumetric area with the manager. If the area is disabled or has no baked texture, it will not be registered.
        /// </summary>
        /// <param name="area">volumetric area to register</param>
        public static void RegisterVolume(BakedVolumetricArea area)
        {
            if (area.bakedTexture == null || !area.isActiveAndEnabled)
            {
                return;
            }
            if (volumes == null || nativeMemoryManager == null || nativeMemoryManager.disposed)
            {
                InitializeArrays();
            }
            if (volumeCount > 0 && volumes.Count > volumeCount)
            {
                volumes[volumeCount] = area;
                volumeBounds[volumeCount] = MinMaxAABB.CreateFromCenterAndExtents(area.transform.position, area.BoxScale);
                volumeTexHandles[volumeCount] = RTHandles.Alloc(area.bakedTexture);
            }
            else
            {
                volumes.Add(area);
                volumeBounds.Add(MinMaxAABB.CreateFromCenterAndExtents(area.transform.position, area.BoxScale));
                volumeTexHandles.Add(RTHandles.Alloc(area.bakedTexture));
            }
            volumeCount++;
        }

        /// <summary>
        /// Remove a volume from the manager.
        /// </summary>
        /// <param name="area">volume to remove</param>
        public static void RemoveVolume(BakedVolumetricArea area)
        {
            if (nativeMemoryManager == null || nativeMemoryManager.disposed) return; 
            int numVolumes = volumeCount;
            for (int vIdx = 0; vIdx < numVolumes; vIdx++)
            {
                // swap area at end of list with area to be deleted, and null out the area at end of list
                if (volumes[vIdx] == area)
                {
                    int lastArea = volumeCount - 1;
                    volumes[vIdx] = volumes[lastArea];
                    volumes[lastArea] = null;

                    volumeTexHandles[vIdx].Release();
                    volumeTexHandles[vIdx] = volumeTexHandles[lastArea];
                    volumeTexHandles[lastArea] = null;

                    volumeBounds[vIdx] = volumeBounds[lastArea];
                    volumeBounds[lastArea] = new MinMaxAABB(float3(float.MaxValue, float.MaxValue, float.MaxValue), float3(float.MaxValue, float.MaxValue, float.MaxValue));
                    volumeCount--;
                    break;
                }
            }
        }

        /// <summary>
        /// Updates the state of a volume. If the volume has no texture or is inactive, it will be removed. If the volume was not previously registered, it will be registered.
        /// If the volume was registered, the bounds are recalculated.
        /// </summary>
        /// <param name="area">Volume to update</param>
        public static void UpdateVolume(BakedVolumetricArea area)
        {
            if (area.bakedTexture == null || !area.isActiveAndEnabled)
            {
                RemoveVolume(area);
            }
            int numVolumes = volumeCount;
            int vIdx = 0;
            for (; vIdx < numVolumes; vIdx++)
            {
                if (volumes[vIdx] == area)
                {
                    volumeBounds[vIdx] = MinMaxAABB.CreateFromCenterAndExtents(area.transform.position, area.BoxScale);
                    break;
                }
            }
            if (vIdx == numVolumes)
            {
                RegisterVolume(area);
            }
        }

        /// <summary>
        /// Remove all null volumes from the registry. Ideally, this should not be necessary as volumes are supposed to remove themselves on destroy.
        /// </summary>
        public static void GarbageCollectVolumes()
        {
            int start = 0;
            int end = volumes.Count - 1;
            // quicksort null volumes to the end. The RemoveVolume function should have already sorted all deleted volumes to the end, but just to make sure we'll go over the array again.
            while (start <= end)
            {
                if (volumes[start])
                {
                    start++;
                }
                else
                {
                    volumes[start] = volumes[end];
                    volumeTexHandles[start] = volumeTexHandles[end];
                    volumeBounds[start] = volumeBounds[end];
                    end--;
                }

            }
            // trim null volumes
            if (start == volumes.Count)
            {
                return;
            }
            else
            {
                volumes.RemoveRange(start, volumes.Count - start);
                volumeTexHandles.RemoveRange(start, volumes.Count - start);
                volumeBounds.RemoveRange(start, volumes.Count - start);
                volumeCount = start;
            }
        }

        /// <summary>
        /// Gets all volumetric areas touching a cubic clip volume, brute force checking against every volume in every loaded scene
        /// </summary>
        /// <param name="position">Clip volume center</param>
        /// <param name="clipSize">Outer clip volume width</param>
        /// <param name="volumeIndicies">Output array of volume indicies in <see cref="volumes"/>. Must be the same size as <see cref="volumes"/> or larger. </param>
        /// <returns>The number of volumes intersected. The input <paramref name="volumeIndicies"/> array will be populated with the indices of the intersected volumes. </returns>
        public static int GetVolumesBruteForce(float3 position, float clipSize, ref NativeArray<ushort> volumeIndicies)
        {
            MinMaxAABB clipAABB = MinMaxAABB.CreateFromCenterAndExtents(position, clipSize);
            int numCollisions = VolumetricManager.BruteForceCollisionCheck(ref clipAABB, ref volumeBounds, ref volumeIndicies);
            return numCollisions;
        }



        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        static bool BoundsOverlap(in MinMaxAABB a, in MinMaxAABB b)
        {
            float3 aMin = a.Min;
            float3 aMax = a.Max;
            float3 bMin = b.Min;
            float3 bMax = b.Max;
            bool3 mins = bMax > aMin;
            bool3 maxs = aMax > bMin;
            bool3 combined = mins & maxs;
            return all(combined);
        }


        /// <summary>
        /// Check all AABB's in the list <paramref name="volumes"/> for overlaps with the AABB <paramref name="clip"/>, and store the indicies of those that overlap in the array <paramref name="collisions"/>
        /// </summary>
        /// <param name="clip">clip volume's AABB</param>
        /// <param name="volumes">List of the AABBs of every <see cref="BakedVolumetricArea"/> component in all open scenes</param>
        /// <param name="collisions">Output array of indicies in <paramref name="volumes"/> indicating which AABB's in that list overlap with <paramref name="clip"/>. Values are populated sequentially from 0 up to the number of overlaps.</param>
        /// <returns>The number of AABBs in the <paramref name="volumes"/> list that overlap with <paramref name="clip"/>. This value indicates the number of output values in <paramref name="collisions"/></returns>
        [BurstCompile(FloatMode = FloatMode.Fast, OptimizeFor = OptimizeFor.Performance)]
        static int BruteForceCollisionCheck(ref MinMaxAABB clip, ref NativeList<MinMaxAABB> volumes, ref NativeArray<ushort> collisions)
        {
            int numVolumes = volumes.Length;
            int numCollisions = 0;
            for (int i = 0; i < numVolumes; i++)
            {
                bool hasOverlap = BoundsOverlap(volumes[i], in clip);
                if (hasOverlap)
                {
                    collisions[numCollisions] = (ushort)i;
                    numCollisions++;
                }
            }
            return numCollisions;
        }
    }
}
