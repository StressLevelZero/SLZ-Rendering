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
