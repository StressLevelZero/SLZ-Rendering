#if !defined(DXC_VULKAN_EXTENSIONS)
#define DXC_VULKAN_EXTENSIONS


// min16float is normally relaxed precision, becomes float16_t if float16 support is requested
// A lot of functions do not have explicit precision equivalents in current spir-v, use full float instead
#if defined(UNITY_DEVICE_SUPPORTS_NATIVE_16BIT)
    #define relaxed_f16t  float
    #define relaxed_f16t2 float2
    #define relaxed_f16t3 float3
    #define relaxed_f16t4 float4
#else
    #define relaxed_f16t  min16float
    #define relaxed_f16t2 min16float2
    #define relaxed_f16t3 min16float3
    #define relaxed_f16t4 min16float4
#endif

#define UNITY_PREPROCESSOR_SKIP(a,b) a##b
#define PREPROCESSOR_HASH #

#if defined(UNITY_COMPILER_DXC) && defined(SHADER_API_VULKAN)
#define INLINE_SPIRV
#endif

namespace VkSPIRV
{

#if defined(INLINE_SPIRV) 
[[vk::ext_instruction(55, "GLSL.std.450")]]
#endif
uint PackUNorm4x8(float4 toPack)
{
    uint4 uFixed = (uint4)round(saturate(toPack) * 255.0);
    return  ( uFixed.x & 0xFF) | 
            ((uFixed.y & 0xFF) << 8) | 
            ((uFixed.z & 0xFF) << 16) | 
            ((uFixed.w & 0xFF) << 24);
}

#if !defined(UNITY_DEVICE_SUPPORTS_NATIVE_16BIT)
#if defined(INLINE_SPIRV) 
[[vk::ext_instruction(55, "GLSL.std.450")]]
#endif
uint PackUNorm4x8(min16float4 toPack)
{
    uint4 uFixed = (uint4)round(saturate(toPack) * min16float(255.0));
    return  ( uFixed.x & 0xFF) | 
            ((uFixed.y & 0xFF) << 8) | 
            ((uFixed.z & 0xFF) << 16) | 
            ((uFixed.w & 0xFF) << 24);
}
#endif

#if defined(INLINE_SPIRV) 
[[vk::ext_instruction(64, "GLSL.std.450")]]
#endif
relaxed_f16t4 UnpackUNorm4x8(uint packed)
{
    uint4 expanded = uint4(
        packed & 0xFF,
        (packed >> 8) & 0xFF,
        (packed >> 16) & 0xFF,
        (packed >> 24) & 0xFF
    );
   
    return relaxed_f16t4(expanded) * relaxed_f16t(1.0 / 255.0);
}

#if defined(INLINE_SPIRV) 
[[vk::ext_instruction(54, "GLSL.std.450")]]
#endif
uint PackSNorm4x8(float4 toPack)
{
    int4 sFixed = (int4)round(clamp(toPack, -1.0, 1.0) * 127.0);
    uint4 uFixed = asuint(sFixed);
    return  ( uFixed.x & 0xFF) | 
            ((uFixed.y & 0xFF) << 8) | 
            ((uFixed.z & 0xFF) << 16) | 
            ((uFixed.w & 0xFF) << 24);
}

#if !defined(UNITY_DEVICE_SUPPORTS_NATIVE_16BIT)
#if defined(INLINE_SPIRV) 
[[vk::ext_instruction(54, "GLSL.std.450")]]
#endif
uint PackSNorm4x8(min16float4 toPack)
{
    int4 sFixed = (int4)round(clamp(toPack, min16float(-1.0), min16float(1.0)) * min16float(127.0));
    uint4 uFixed = asuint(sFixed);
    return  ( uFixed.x & 0xFF) | 
            ((uFixed.y & 0xFF) << 8) | 
            ((uFixed.z & 0xFF) << 16) | 
            ((uFixed.w & 0xFF) << 24);
}
#endif

#if defined(INLINE_SPIRV) 
[[vk::ext_instruction(63, "GLSL.std.450")]]
#endif
relaxed_f16t4 UnpackSNorm4x8(uint packed)
{
    uint4 expanded = uint4(
        packed & 0xFF,
        (packed >> 8) & 0xFF,
        (packed >> 16) & 0xFF,
        (packed >> 24) & 0xFF
    );
    int4 sFixed = asint(expanded);
    return relaxed_f16t4(max(sFixed, -127)) * relaxed_f16t(1.0 / 127.0f);
}


#if defined(INLINE_SPIRV) 
[[vk::ext_instruction(62, "GLSL.std.450")]]
#endif
relaxed_f16t2 UnpackHalf2x16(uint packed)
{
	return relaxed_f16t2(f16tof32(packed), f16tof32(packed << 16));
}

#if defined(INLINE_SPIRV) 
[[vk::ext_instruction(58, "GLSL.std.450")]]
#endif
uint PackHalf2x16(float2 toPack)
{
	return (f32tof16(toPack.x) | (f32tof16(toPack.y) << 16));
}

#if !defined(UNITY_DEVICE_SUPPORTS_NATIVE_16BIT)
    #if defined(INLINE_SPIRV) 
    [[vk::ext_instruction(58, "GLSL.std.450")]]
    #endif
    uint PackHalf2x16(min16float2 toPack)
    {
    	return (f32tof16(toPack.x) | (f32tof16(toPack.y) << 16));
    }
#endif





#if defined(INLINE_SPIRV)
[[vk::ext_instruction(/*OpConstantTrue*/ 41)]]
#endif
bool InlineSPIRVEnabled()
{
	return false;
}

// https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html#Scope_-id-
// Only device (0) and subgroup (3) are valid for fragment shader invocations
#if defined(INLINE_SPIRV)
[[vk::ext_extension("SPV_KHR_shader_clock")]]
[[vk::ext_capability(/*ShaderClockKHR*/ 5055)]]
[[vk::ext_instruction(/*OpReadClockKHR*/ 5056)]]
#endif
uint2 ReadClockUInt2(uint scope = 3) { return (uint2)0; }

// https://developer.nvidia.com/blog/profiling-dxr-shaders-with-timer-instrumentation/
// Gets the delta time from the lower 32 bits of two 64 bit timestamps, handling wrapping
// when the clock crosses a 32 bit threshold
// assumes that the end time is not 2^32 or more clock cycles greater than the start 
uint DeltaClockTime(uint start, uint end)
{
  return end < start ? (~0u - (start - end)) : (end - start);
}

#if defined(INLINE_SPIRV)
    #if defined(SHADER_API_MOBILE) 
        [[vk::ext_extension("SPV_EXT_fragment_invocation_density")]]
        [[vk::ext_capability(/*FragmentDensityEXT*/ 5291)]]
        [[vk::ext_instruction(/*OpNop*/ 0)]]
    #else
        [[vk::ext_extension("SPV_KHR_fragment_shading_rate")]]
        [[vk::ext_capability(/*FragmentShadingRateKHR*/ 4422)]]
        [[vk::ext_instruction(/*OpNop*/ 0)]]
    #endif
#endif
void RequestFragmentDensityEXT() { }


static min16uint2 s_FragSizeExt = min16uint2(1, 1);
#define SLZ_FRAG_SIZE VkSPIRV::s_FragSizeExt

#if defined(INLINE_SPIRV)
	// Read only, only valid in the fragment stage. This cannot go into the interpolator struct output by the vert function.
	// This should be added as an extra parameter to the frag program as the last parameter. Note the leading comma!
	// HLSL doesn't allow trailing commas in parameter lists, so if SLZ_DECLARE_FRAG_SIZE is defined to be empty the comma separating it needs to disappear too

    #if defined(SHADER_API_MOBILE) // FragSizeEXT provided by both fragment density map and nvidia variable rate shading extensions
        #define SLZ_DECLARE_FRAG_SIZE     , [[vk::ext_decorate(/*Builtin*/ 11, /*FragSizeEXT*/ 5292)]] uint2 FragSizeEXT : FRAGSIZE
    	#define SLZ_SETUP_FRAG_SIZE(screenCoords) VkSPIRV::s_FragSizeExt = (min16uint2)FragSizeEXT;
    #else //  
        #define SLZ_DECLARE_FRAG_SIZE     , [[vk::ext_decorate(/*Builtin*/ 11, /*ShadingRateKHR*/ 4444)]] uint ShadingRateKHR : FRAGSIZE
    	#define SLZ_SETUP_FRAG_SIZE(screenCoords) VkSPIRV::s_FragSizeExt = min16uint2(1 << ((ShadingRateKHR >> 2) & 3), 1 << (ShadingRateKHR & 3));
    #endif    

#else

	uint2 FallbackGetFragSize(float2 screenCoords)
	{
		float2 dCoords = float2(ddx(screenCoords.x), ddy(screenCoords.y));
		return uint2(round(dCoords.x), round(dCoords.y));
	}
	
	#define SLZ_DECLARE_FRAG_SIZE
	#define SLZ_SETUP_FRAG_SIZE(screenCoords) s_FragSizeExt = FallbackGetFragSize(screenCoords);
#endif


struct UMulResult
{
    uint low;
    uint high;
};

struct SMulResult
{
    int low;
    int high;
};

// No hlsl equivalent, despite being present in DXBC assembly as far back as SM4.0!
#if defined(INLINE_SPIRV)
[[vk::ext_instruction(151)]]
UMulResult OpUMulExtended(uint a, uint b)
{
    return (UMulResult)0;
}

[[vk::ext_instruction(152)]]
SMulResult OpSMulExtended(uint a, uint b)
{
    return (SMulResult)0;
}
#else

#define OpUMulExtended(a,b) UNITY_PREPROCESSOR_SKIP(PREPROCESSOR_HASH, error OpUMulExtended only supported on Vulkan no HLSL equivalent)
#define OpSMulExtended(a,b) UNITY_PREPROCESSOR_SKIP(PREPROCESSOR_HASH, error OpSMulExtended only supported on Vulkan no HLSL equivalent)

#endif //defined(INLINE_SPIRV)

// Cursed hack to use barycentrics with vulkan and DXC. Currently, DXC has an issue (imposed by the D3D12 spec) that
// requires barycentric interface variables to be declared as nointerpolation. During SPIR-V generation, the compiler 
// attaches both the PerVertexKHR and Flat (no interpolation) decorates to the variable despite PerVertexKHR already implying 
// no interpolation. It seems the NVidia driver takes the Flat decorate as a hint that it only needs to populate the data of 
// the first vertex in the triangle, and trying to get the data of the other two returns 0 or the next attribute in the input 
// layout.
// 
// Fix is to declare all interface variables like they would be in GLSL as arrays with the PerVertexKHR attribute inlined. 
// The arrays in GLSL have no defined size, which is impossible in HLSL. Defining them as 1 long seems to work just fine
// as there are no bounds checks. Trying to make them 3 long causes alignment issues. Additionally, the attributes must
// be DIRECTLY declared as inputs to the fragment program rather than inside a struct. DXC breaks somehow when they
// are part of a struct. 
//
// Cleanest way to handle this I've found is to use some C macro soup. Declare the contents of the vertex output/fragment
// input interface as a macro with inputs for the attribute, array specifier, and line separator:
// ```
// #define COMMA ,
// #define SEMICOLON ;
// #define INTERPOLATORS(atr, arr, sep) \
//     float4 vertex : SV_POSITION    sep \
//     atr float2 uv  arr : TEXCOORD0 sep \
//     atr float2 uv1 arr : TEXCOORD1 sep \
//     atr float2 uv2 arr : TEXCOORD2 sep \
//     VkSPIRV::UnityInstancingAndStereo macros 
// ```
// Notice that we have to nest the unity instancing/stereo macros in a struct as they contain semicolons.
// Declare the vertex program output struct with no input for the attribute and array, and the SEMICOLON macro for the separator:
// ```
// struct v2f
// {
//     INTERPOLATORS( , , SEMICOLON);
// };
// ```
// And declare the fragment program like this:
// ```
// float4 frag(
//     INTERPOLATORS(PER_VERTEX_ATTRIBUTE, PER_VERTEX_ARRAY, COMMA)
//     , BUILTIN_BARY_COORD_KHR float3 baryWeights : BARYCENTRIC_SEMANTIC
//     ) : SV_Target
// {
//     UNITY_SETUP_INSTANCE_ID(macros);
//     UNITY_SETUP_STEREO_EYE_INDEX_POST_VERTEX(macros);
//     VkSPIRV::RequestVkBarycentrics(); // Adds the SPV_KHR_fragment_shader_barycentric extension and capability to this program. Pre DXC 1.8, you can't directly attach extensions/capabilities to the entry point and have to call a function that has them
//     float2 example = GET_ATTRIBUTE_AT_VERTEX(uv, 1);
//     ...
// ```
// This uses several macros defined below to resolve to either the proper HLSL syntax or on Vulkan the inline SPIR-V
// with the interface variables as 1 long arrays. Additionally, do not require barycentrics on vulkan:
// ```
// #if !defined(SHADER_API_VULKAN)
// #pragma require Barycentrics
// #endif
// ```
// Unity assumes barycentrics do not work on vulkan due to an even older DXC bug and will exclude the shader if that is required.

struct UnityInstancingAndStereo
{
    UNITY_VERTEX_INPUT_INSTANCE_ID
    UNITY_VERTEX_OUTPUT_STEREO
};

#if defined(INLINE_SPIRV)
    #define PER_VERTEX_ATTRIBUTE            [[vk::ext_decorate(5285 /*PerVertexKHR*/)]]
    #define GET_ATTRIBUTE_AT_VERTEX(a, i)   a[i]
    #define PER_VERTEX_ARRAY                [1]
    #define ENABLE_FRAGMENT_BARYCENTRIC_KHR [[vk::ext_extension("SPV_KHR_fragment_shader_barycentric")]][[vk::ext_capability(5284)]]
    #define BUILTIN_BARY_COORD_KHR          [[vk::ext_decorate(/*Builtin*/11, /*BaryCoordKHR*/5286)]]
    #define BUILTIN_BARY_COORD_NO_PERSP_KHR [[vk::ext_decorate(/*Builtin*/11, /*BaryCoordNoPerspKHR*/5287)]]
    #define BARYCENTRIC_SEMANTIC            VkDummyBarycentrics
#else
    #define PER_VERTEX_ATTRIBUTE            nointerpolation
    #define GET_ATTRIBUTE_AT_VERTEX(a, i)   GetAttributeAtVertex(a, i)
    #define PER_VERTEX_ARRAY
    #define ENABLE_FRAGMENT_BARYCENTRIC_KHR
    #define BUILTIN_BARY_COORD_KHR
    #define BUILTIN_BARY_COORD_NO_PERSP_KHR noperspective
    #define BARYCENTRIC_SEMANTIC SV_Barycentrics
#endif

#if defined(INLINE_SPIRV)
ENABLE_FRAGMENT_BARYCENTRIC_KHR
[[vk::ext_instruction(/*OpNop*/ 0)]]
#endif
void RequestVkBarycentrics() { }


#undef relaxed_f16t  
#undef relaxed_f16t2
#undef relaxed_f16t2
#undef relaxed_f16t3

} // end namespace vk::spirv

#endif // DXC_VULKAN_EXTENSIONS
