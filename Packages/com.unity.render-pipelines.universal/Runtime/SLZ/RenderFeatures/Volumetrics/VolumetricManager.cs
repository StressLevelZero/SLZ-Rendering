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
using System.Linq;
using System.Runtime.CompilerServices;
using System.Diagnostics;
using Debug = UnityEngine.Debug;

namespace SLZRendering.Runtime
{
    [BurstCompile]
    public static class VolumetricManager
    {
        public static List<BakedVolumetricArea> bakedVolumetricAreas;
        public static NativeArray<BvHNode> BvHTree;
        public static NativeArray<MinMaxAABB> areaBounds;
        public static NativeArray<ushort> areaIdxBuffer;
        public static ushort BvHTreeNodeCount { get; private set; }

        [StructLayout(LayoutKind.Sequential)]
        public struct BvHNode
        {
            public MinMaxAABB bounds;
            public ushort branchIdx0;
            public ushort branchIdx1;
            public ushort areaIdxStart;
            public ushort areaIdxEnd;
        }

        public static List<BakedVolumetricArea> GetVolumesBruteForce(float3 position, float clipSize)
        {
            MinMaxAABB clipAABB = MinMaxAABB.CreateFromCenterAndExtents(position, clipSize);
            NativeArray<ushort> collisions = new NativeArray<ushort>(areaBounds.Length, Allocator.TempJob);
            Stopwatch timer = new Stopwatch();
            timer.Start();
            int numCollisions = BruteForceCollisionCheck(ref clipAABB, ref areaBounds, ref collisions);
            timer.Stop();
            Debug.Log($"Brute force took {timer.ElapsedTicks}");
            List<BakedVolumetricArea> volumes = new List<BakedVolumetricArea>(numCollisions);
            for (int i = 0; i < numCollisions; i++)
            {
                volumes.Add(bakedVolumetricAreas[collisions[i]]);
            }
            collisions.Dispose();
            return volumes;
        }

        public static List<BakedVolumetricArea> GetVolumesBvH(float3 position, float clipSize)
        {
            MinMaxAABB clipAABB = MinMaxAABB.CreateFromCenterAndExtents(position, clipSize);
            NativeArray<ushort> collisions = new NativeArray<ushort>(areaBounds.Length, Allocator.TempJob);
            NativeArray<ushort> stack = new NativeArray<ushort>( ( bakedVolumetricAreas.Count + 1 ) / 2, Allocator.TempJob);
            Stopwatch timer = new Stopwatch();
            timer.Start();
            int numCollisions = BvHCollisionCheck(ref clipAABB, ref BvHTree, ref areaBounds, ref areaIdxBuffer, ref stack, ref collisions);
            timer.Stop();
            Debug.Log($"BvH Traversal took {timer.ElapsedTicks}");
            List<BakedVolumetricArea> volumes = new List<BakedVolumetricArea>(numCollisions);
            for (int i = 0; i < numCollisions; i++)
            {
                volumes.Add(bakedVolumetricAreas[collisions[i]]);
            }
            collisions.Dispose();
            stack.Dispose();
            return volumes;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        static bool BvHCollision(in MinMaxAABB a, in MinMaxAABB b)
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

        [BurstCompile(FloatMode = FloatMode.Fast, OptimizeFor = OptimizeFor.Performance)]
        static int BruteForceCollisionCheck(ref MinMaxAABB clip, ref NativeArray<MinMaxAABB> volumes, ref NativeArray<ushort> collisions)
        {
            int numVolumes = volumes.Length;
            int numCollisions = 0;
            for (int i = 0; i < numVolumes; i++)
            {
                //float3 vMin = volumes[i].Min;
                //float3 vMax = volumes[i].Max;
                //float3 cMin = clip.Min;
                //float3 cMax = clip.Max;
                //bool3 mins = vMax > cMin;
                //bool3 maxs = cMax > vMin;
                //bool3 combined = mins & maxs;
                //if (all(combined))
                bool hasOverlap = BvHCollision(volumes[i], clip);
                if (hasOverlap)
                {
                    collisions[numCollisions] = (ushort)i;
                    numCollisions++;
                }
            }
            return numCollisions;
        }

        [BurstCompile(FloatMode = FloatMode.Fast, OptimizeFor = OptimizeFor.Performance)]
        static int BvHCollisionCheck(
            ref MinMaxAABB clip, ref NativeArray<BvHNode> BvHTree, ref NativeArray<MinMaxAABB> areaBounds, ref NativeArray<ushort> areaIdxBuffer, ref NativeArray<ushort> stack, ref NativeArray<ushort> collisions
            )
        {
            int numNodes = BvHTree.Length;
            int numCollisions = 0;
            stack[0] = 0;
            int stackPtr = 0;
            for (int i = 0; i < numNodes; i++)
            {
                if (stackPtr < 0) break;
                int index = stack[stackPtr];
                stackPtr--;
                BvHNode node = BvHTree[index];
                MinMaxAABB aabb = node.bounds;
                if (!BvHCollision(aabb, clip))
                {
                    continue;
                }
                
                if (node.branchIdx0 != 0xFFFF)
                {
                    stackPtr++;
                    stack[stackPtr] = node.branchIdx0;
                }

                if (node.branchIdx1 != 0xFFFF)
                {
                    stackPtr++;
                    stack[stackPtr] = node.branchIdx1;
                }

                if (node.branchIdx0 == 0xFFFF || node.branchIdx1 == 0xFFFF)
                {
                    for (int vIdx = node.areaIdxStart; vIdx <= node.areaIdxEnd; vIdx++)
                    {
                        int volIdx = areaIdxBuffer[vIdx];
                        if (BvHCollision(areaBounds[volIdx], clip))
                        {
                            collisions[numCollisions] = (ushort)volIdx;
                            numCollisions++;
                        }
                    }
                }
            }
            return numCollisions;
        }



        public static void Init()
        {
            int validVolumeCount = bakedVolumetricAreas.Count;
            for (int aIdx = validVolumeCount - 1; aIdx > -1; aIdx--)
            {
                BakedVolumetricArea area = bakedVolumetricAreas[aIdx];
                if (area == null || area.bakedTexture == null)
                {
                    validVolumeCount--;
                    bakedVolumetricAreas.RemoveAt(aIdx);
                }
            }

            if (areaBounds != null && areaBounds.IsCreated)
            {
                areaBounds.Dispose();
            }
            areaBounds = new NativeArray<MinMaxAABB>(validVolumeCount, Allocator.Persistent);

            int volumeCount = bakedVolumetricAreas.Count;
            for (int vIdx = 0; vIdx < volumeCount; vIdx++)
            {
                BakedVolumetricArea area = bakedVolumetricAreas[vIdx];
                if (area != null)
                {
                    areaBounds[vIdx] = MinMaxAABB.CreateFromCenterAndExtents(area.transform.position, area.BoxScale);
                }
            }
        }

        public static void BuildBVH()
        {
            Init();
            int validVolumeCount = bakedVolumetricAreas.Count;

            if (BvHTree != null && BvHTree.IsCreated)
            {
                BvHTree.Dispose();
            }
            BvHTree = new NativeArray<BvHNode>(validVolumeCount + 1, Allocator.Persistent);

            if (areaIdxBuffer != null && areaIdxBuffer.IsCreated)
            {
                areaIdxBuffer.Dispose();
            }
            areaIdxBuffer = new NativeArray<ushort>(validVolumeCount, Allocator.Persistent);

            int volumeCount = bakedVolumetricAreas.Count;
            for (int vIdx = 0; vIdx < volumeCount; vIdx++)
            {
                areaIdxBuffer[vIdx] = (ushort)vIdx;
            }

            NativeArray<ushort> stack = new NativeArray<ushort>((validVolumeCount + 1) / 2, Allocator.TempJob);
            BvHTreeNodeCount = BuildBVHBurst((ushort)validVolumeCount, ref areaIdxBuffer, ref stack, ref areaBounds, ref BvHTree);
            stack.Dispose();
        }

        public static void DisposeBvH()
        {
            if (areaBounds != null && areaBounds.IsCreated)
            {
                areaBounds.Dispose();
            }

            if (areaIdxBuffer != null && areaIdxBuffer.IsCreated)
            {
                areaIdxBuffer.Dispose();
            }

            if (BvHTree != null && BvHTree.IsCreated)
            {
                BvHTree.Dispose();
            }
        }

        /// <summary>
        /// builds a simple BvH
        /// </summary>
        /// <param name="numVolumes">Number of volumes, should be bakedVolumetricAreas.Count</param>
        /// <param name="idxBuffer">Input/Output index buffer pointing to volumes in bakedVolumetricAreas. Expected to initially be populated with sequential numbers from 0 to bakedVolumetricAreas.Count.</param>
        /// <param name="stack">Memory for keeping track of unfinished branches of the BvH tree. Length must be 1/2 of the number of volumes</param>
        /// <param name="volumeBounds">Axis-aligned bounding boxes of individual volumes</param>
        /// <param name="nodes">Output BvH tree</param>
        /// <param name="thisNodeIdx"></param>
        [BurstCompile(FloatMode = FloatMode.Fast, OptimizeFor = OptimizeFor.Performance)]
        static ushort BuildBVHBurst(ushort numVolumes, ref NativeArray<ushort> idxBuffer, ref NativeArray<ushort> stack, ref NativeArray<MinMaxAABB> volumeBounds, ref NativeArray<BvHNode> nodes) 
        {
            int idxCount = idxBuffer.Length;
            int volumeCount = volumeBounds.Length;
            ushort BvHTreeNodeCount = 0;
#if UNITY_EDITOR || DEVELOPMENT_BUILD
            if (numVolumes < 1)
            {
                return 0;
            }
            /*
            if (idxCount != numVolumes)
            {
                Debug.LogError("Invalid index buffer, length not equal to the number of volumes");
            }
            if (volumeCount != numVolumes)
            {
                Debug.LogError("Invalid volume bounds buffer, length not equal to the number of volumes");
            }
            if (nodes.Length < numVolumes)
            {
                Debug.LogError("Invalid nodes buffer, length must be greater or equal to the number of volumes");
            }
            if (stack.Length < numVolumes / 2)
            {
                Debug.LogError("Invalid stack array, length must be greater or equal to half the number of volumes");
            }
            */
#endif
            //ushort numVolumesUshort = (ushort)numVolumes;
            //for (ushort i = 0; i < numVolumesUshort; i++)
            //{
            //    idxBuffer[i] = i;
            //}

            stack[0] = 0;
            int stackPtr = 0;
            ushort nodePtr = 0;
            nodes[0] = new BvHNode()
            {
                areaIdxStart = 0,
                areaIdxEnd = (ushort)(numVolumes - 1),
            };

            int axisOffset = 0;
            MinMaxAABB thisBounds = default;
            float3 centerOfMass = default;
            for (int maxIter = 0; maxIter < 3 * volumeCount; maxIter++)
            {
                if (stackPtr < 0) break; // All work has been done
                // pop one off the stack
                int thisNodeIdx = stack[stackPtr];
                stackPtr -= 1;

                BvHNode node = nodes[thisNodeIdx];
               
                // if axisOffset is non-zero, we're reprocessing the last node.
                // Don't recalculate bounds, and don't increment the node count.
                if (axisOffset == 0)
                {
                    BvHTreeNodeCount++;
                    int index = idxBuffer[node.areaIdxStart];
                    thisBounds = volumeBounds[index];
                    float3 vol0Dim = thisBounds.Extents;
                    //float totalMass = vol0Dim.x * vol0Dim.y * vol0Dim.z;
                    //centerOfMass = thisBounds.Center;
                    for (int nIdx = node.areaIdxStart + 1; nIdx <= node.areaIdxEnd; nIdx++)
                    {
                        index = idxBuffer[nIdx];
                        //float3 dim = volumeBounds[index].Extents;
                        //float mass = dim.x * dim.y * dim.z;
                        //float newTotalMass = totalMass + mass;
                        //float3 center = volumeBounds[index].Center;
                        thisBounds.Encapsulate(volumeBounds[index]);
                        //centerOfMass = (centerOfMass * (totalMass / newTotalMass)) + ((mass / newTotalMass) * center);
                        //totalMass = newTotalMass;
                    }
                    centerOfMass = thisBounds.Center;
                    //// Debug.Log($"Node {thisNodeIdx} Bounds: {thisBounds}, Center {centerOfMass}");
                    node.bounds = thisBounds;
                }
                

                if ((node.areaIdxEnd - node.areaIdxStart) < 2)
                {

                    nodes[thisNodeIdx] = new BvHNode
                    {
                        bounds = thisBounds,
                        branchIdx0 = 0xFFFF,
                        branchIdx1 = 0xFFFF,
                        areaIdxStart = node.areaIdxStart,
                        areaIdxEnd = node.areaIdxEnd,
                    };
                    axisOffset = 0;
                    continue;
                }

                // sort the axes by length
                float3 boundsSize = thisBounds.Max - thisBounds.Min;
                int3 axisSort = int3(0, 1, 2);
                if (boundsSize.y > boundsSize.x)
                {
                    axisSort.x = 1;
                    axisSort.y = 0;
                }
                if (boundsSize.z > boundsSize[axisSort.x])
                {
                    axisSort.z = axisSort.x;
                    axisSort.x = 2;
                }
                if (boundsSize[axisSort.z] > boundsSize[axisSort.y])
                {
                    int temp = axisSort.z;
                    axisSort.z = axisSort.y;
                    axisSort.y = temp;
                }

                // prefer to use the longest axis. Choose the next longest axis if we already tried that axis previously and failed to split the node
                int axis = axisSort[axisOffset % 3]; 
                ////Debug.Log($"Axis: {axis}");
                float midPoint = centerOfMass[axis];// 0.5f * boundsSize[axis] + thisBounds.Min[axis];
                ////Debug.Log($"Midpoint: {midPoint}");
                int left = node.areaIdxStart;
                int right = node.areaIdxEnd;
                ////Debug.Log($"Splitting {left}, {right}");
                // Quicksort all volumes completely contained in the "left" side of the bounding box into the lower half of the array
                while (left <= right)
                {
                    ushort leftIdx = idxBuffer[left];
                    float leftCenter = 0.5f * (volumeBounds[leftIdx].Max[axis] + volumeBounds[leftIdx].Min[axis]);
                    ////Debug.Log($"Volume {left} {leftIdx} {bakedVolumetricAreas[leftIdx]}, axis max: {leftCenter}, contained in left side: {leftCenter < midPoint}");
                    if (leftCenter < midPoint)
                    {
                        left += 1;
                    }
                    else
                    {
                        idxBuffer[left] = idxBuffer[right];
                        idxBuffer[right] = leftIdx;
                        right -= 1;
                    }
                   
                }
                // if the left index didn't move, all volumes touch the right side. Try again on a different axis.
                // At least one volume must be touching the right side of the 
                // if we run out of axes, give up and have all volumes contained in this node.
                if (left == node.areaIdxStart)
                {
                    ////Debug.LogError($"Split Failed on node {thisNodeIdx}, {left} ({node.areaIdxStart}, {node.areaIdxEnd})");
                    // switch axes
                    axisOffset += 1;
                    if (axisOffset < 3)
                    {
                        //Move the stack pointer back onto this element and try again with a different axis
                        nodes[thisNodeIdx] = node;
                        stackPtr++;
                    }
                    else // Give up subdividing and include all volumes in this node
                    {
                        ////Debug.LogError($"Node {thisNodeIdx} could not be split");
                        nodes[thisNodeIdx] = new BvHNode
                        {
                            bounds = thisBounds,
                            branchIdx0 = 0xFFFF,
                            branchIdx1 = 0xFFFF,
                            areaIdxStart = node.areaIdxStart,
                            areaIdxEnd = node.areaIdxEnd,
                        };
                        axisOffset = 0;
                    }
                    
                    continue;
                }
                else
                {
                    /*
                    string msg = $"Split Succeeded on axis {axis} {left}, ({node.areaIdxStart}, {node.areaIdxEnd})";
                    for (int nIdx = 0; nIdx < volumeCount; nIdx++)
                    {
                        int index = idxBuffer[nIdx];
                        msg += $"\n {nIdx}: index {index}, {bakedVolumetricAreas[index]}";
                    }
                    Debug.LogError(msg);
                    */
                }
                axisOffset = 0;
                // There are more than one volumes in the left half of the bounds
                if (left > (node.areaIdxStart + 1))
                {
                    BvHNode leftNode = new BvHNode()
                    {
                        areaIdxStart = node.areaIdxStart,
                        areaIdxEnd = (ushort)(left - 1),
                    };
                    nodePtr++;
                    nodes[nodePtr] = leftNode;
                    stackPtr++;
                    stack[stackPtr] = nodePtr;
                    node.branchIdx0 = nodePtr;
                }
                else // Mark left node as leaf, areaIdxStart is the index of the leaf volume
                {
                    
                    node.branchIdx0 = 0xFFFF;
                }

                if (left < (node.areaIdxEnd))
                {
                    BvHNode rightNode = new BvHNode()
                    {
                        areaIdxStart = (ushort)left,
                        areaIdxEnd = node.areaIdxEnd,
                    };
                    nodePtr++;
                    nodes[nodePtr] = rightNode;
                    stackPtr++;
                    stack[stackPtr] = nodePtr;
                    node.branchIdx1 = nodePtr;
                }
                else // Mark left node as leaf, areaIdxEnd is the index of the leaf volume
                {
                    node.branchIdx1 = 0xFFFF;
                    node.areaIdxStart = node.areaIdxEnd;
                }
                nodes[thisNodeIdx] = node;
            }
            return BvHTreeNodeCount;
        }
         
    }
}
