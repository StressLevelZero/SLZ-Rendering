using System.Collections.Generic;
using Unity.Burst;
using Unity.Collections;
using Unity.Collections.LowLevel;
using UnityEngine;
using Unity.Mathematics;
using Unity.Mathematics.Geometry;
using UnityEngine.Rendering.Universal;
using System.Runtime.InteropServices;

using static Unity.Mathematics.math;

namespace SLZRendering.Runtime
{
    [BurstCompile]
    public static class VolumetricManager
    {
        static List<BakedVolumetricArea> bakedVolumetricAreas;
        static NativeArray<MinMaxAABB> areaBounds;
        static NativeArray<ushort> areaIdxBuffer;

        [StructLayout(LayoutKind.Sequential)]
        struct BvHNode
        {
            public MinMaxAABB bounds;
            public ushort leafIdx0;
            public ushort leafIdx1;
            public ushort areaIdxStart;
            public ushort areaIdxEnd;
        }

        public static void BuildBVH()
        {
            int validVolumeCount = 0;
            foreach (BakedVolumetricArea area in bakedVolumetricAreas)
            {
                if (area != null && area.bakedTexture != null)
                {
                    validVolumeCount++;
                }
            }

            if (areaBounds != null && areaBounds.IsCreated)
            {
                areaBounds.Dispose();
            }
            areaBounds = new NativeArray<MinMaxAABB>(validVolumeCount, Allocator.Persistent);

            if (areaIdxBuffer != null && areaIdxBuffer.IsCreated)
            {
                areaIdxBuffer.Dispose();
            }
            areaIdxBuffer = new NativeArray<ushort>(validVolumeCount, Allocator.Persistent);

            ushort volumeIdx = 0;
            foreach (BakedVolumetricArea area in bakedVolumetricAreas)
            {
                if (area != null && area.bakedTexture != null)
                {
                    areaIdxBuffer[volumeIdx] = volumeIdx;
                    areaBounds[volumeIdx] = new MinMaxAABB(area.Corner, area.Corner + area.NormalizedScale);
                    volumeIdx++;
                }
            }
        }

        [BurstCompile]
        static void BuildBVHRecurse(ref NativeArray<ushort> idxBuffer, ref NativeArray<MinMaxAABB> bounds, ref NativeList<BvHNode> nodes, int thisNodeIdx) 
        {
            BvHNode node = nodes[thisNodeIdx];

            MinMaxAABB thisBounds = bounds[node.areaIdxStart];
            for (int nIdx = node.areaIdxStart + 1; nIdx <= node.areaIdxEnd; nIdx++)
            {
                thisBounds.Encapsulate(bounds[nIdx]);
            }

            if ((node.areaIdxEnd - node.areaIdxStart) < 2)
            {
                
                nodes[thisNodeIdx] = new BvHNode
                {
                    bounds = thisBounds,
                    leafIdx0 = 0xFFFF,
                    leafIdx1 = 0xFFFF,
                    areaIdxStart = node.areaIdxStart,
                    areaIdxEnd = node.areaIdxEnd,
                };
                return;
            }

            float3 boundsSize = thisBounds.Max - thisBounds.Min;
            
        }
    }
}
