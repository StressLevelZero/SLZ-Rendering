#if !defined(DXC_VULKAN_EXTENSIONS)
#define DXC_VULKAN_EXTENSIONS

// min16float is normally relaxed precision, becomes float16_t if float16 support is requested
// None of these functions have explicit precision equivalents in current spir-v, fall back to full float
#if defined(UNITY_COMPILER_DXC) && defined(SHADER_API_VULKAN)
#define INLINE_SPIRV
#endif

#if defined(INLINE_SPIRV)
[[vk::ext_instruction(/*OpConstantTrue*/ 41)]]
#endif
bool InlineSPIRVEnabled()
{
	return false;
}

#if defined(INLINE_SPIRV) 
[[vk::ext_instruction(62, "GLSL.std.450")]]
#endif
min16float2 UnpackHalf2x16(uint packed)
{
	return min16float2(f16tof32(packed), f16tof32(packed << 16));
}

#if defined(INLINE_SPIRV) 
[[vk::ext_instruction(58, "GLSL.std.450")]]
#endif
uint PackHalf2x16(min16float2 toPack)
{
	return (f32tof16(toPack.x) | (f32tof16(toPack.y) << 16));
}

#if defined(INLINE_SPIRV) 
[[vk::ext_instruction(58, "GLSL.std.450")]]
#endif
uint PackHalf2x16(float2 toPack)
{
	return (f32tof16(toPack.x) | (f32tof16(toPack.y) << 16));
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


static uint2 s_FragSizeExt = uint2(0, 0);
#define SLZ_FRAG_SIZE s_FragSizeExt

#if defined(INLINE_SPIRV)
	// Read only, only valid in the fragment stage. This cannot go into the interpolator struct output by the vert function.
	// This should be added as an extra parameter to the frag program as the last parameter. Note the leading comma!
	// HLSL doesn't allow trailing commas in parameter lists, so if SLZ_DECLARE_FRAG_SIZE is defined to be empty the comma separating it needs to disappear too
    #if defined(SHADER_API_MOBILE) // FragSizeEXT provided by both fragment density map and nvidia variable rate shading extensions
        #define SLZ_DECLARE_FRAG_SIZE     , [[vk::ext_decorate(/*Builtin*/ 11, /*FragSizeEXT*/ 5292)]] uint2 FragSizeEXT : FRAGSIZE
    	#define SLZ_SETUP_FRAG_SIZE(screenCoords) s_FragSizeExt = FragSizeEXT;
    #else //  
        #define SLZ_DECLARE_FRAG_SIZE     , [[vk::ext_decorate(/*Builtin*/ 11, /*ShadingRateKHR*/ 4444)]] uint ShadingRateKHR : FRAGSIZE
    	#define SLZ_SETUP_FRAG_SIZE(screenCoords) s_FragSizeExt = uint2(1 << (ShadingRateKHR & 3), 1 << ((ShadingRateKHR >> 2) & 3));
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
//     UnityMacroInterface macros 
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
//     RequestVkBarycentrics(); // Adds the SPV_KHR_fragment_shader_barycentric extension and capability to this program
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

struct UnityMacroInterface
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
    #define BUILTIN_BARY_COORD_NO_PERSP_KHR [[vk::ext_decorate(11, /*BaryCoordNoPerspKHR*/5287)]]
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

#endif // DXC_VULKAN_EXTENSIONS
