#if !defined(DXC_VULKAN_EXTENSIONS)
#define DXC_VULKAN_EXTENSIONS

// min16float is normally relaxed precision, becomes float16_t if float16 support is requested
// None of these functions have explicit precision equivalents in current spir-v, fall back to full float


#if defined(UNITY_COMPILER_DXC) 
[[vk::ext_instruction(62, "GLSL.std.450")]]
#endif
min16float2 UnpackHalf2x16(uint packed)
{
	return min16float2(f16tof32(packed), f16tof32(packed << 16));
}

#if defined(UNITY_COMPILER_DXC) 
[[vk::ext_instruction(58, "GLSL.std.450")]]
#endif
uint PackHalf2x16(min16float2 toPack)
{
	return (f32tof16(toPack.x) | (f32tof16(toPack.y) << 16));
}

#if defined(UNITY_COMPILER_DXC) 
[[vk::ext_instruction(58, "GLSL.std.450")]]
#endif
uint PackHalf2x16(float2 toPack)
{
	return (f32tof16(toPack.x) | (f32tof16(toPack.y) << 16));
}




#if defined(UNITY_COMPILER_DXC) && !defined(UNITY_DEVICE_SUPPORTS_NATIVE_16BIT) 

[[vk::ext_capability( /* Float16 */9)]]
[[vk::ext_type_def( /* Unique id for type */0, /* OpTypeFloat */22)]]
void createTypeFloat16_t([[vk::ext_literal]] int sizeInBits)
{
}

[[vk::ext_type_def( /* Unique id for type */2, /* OpTypeVector */23)]]
void createTypeFloat16_t2([[vk::ext_reference]] vk::ext_type<0> typeF16,
							[[vk::ext_literal]] int componentCount);

[[vk::ext_type_def( /* Unique id for type */3, /* OpTypeVector */23)]]
void createTypeFloat16_t3([[vk::ext_reference]] vk::ext_type <0>typeF16,
							[[vk::ext_literal]] int componentCount);

[[vk::ext_type_def( /* Unique id for type */4, /* OpTypeVector */23)]]
void createTypeFloat16_t4([[vk::ext_reference]] vk::ext_type <0>typeF16,
							[[vk::ext_literal]] int componentCount);

#define float16_t vk::ext_type<0>

[[vk::ext_instruction(/*OpFConvert*/115)]]
float16_t asF16(float f)
{
	float16_t t;
	return t;
}

[[vk::ext_instruction(/*OpFConvert*/115)]]
float asF32(float16_t f)
{
	return 0;
}


void DeclareInlineF16()
{
	createTypeFloat16_t(16);
	float16_t dummy;
	createTypeFloat16_t2(dummy, 2);
	createTypeFloat16_t3(dummy, 3);
	createTypeFloat16_t4(dummy, 4);
}
#endif // !defined(UNITY_DEVICE_SUPPORTS_NATIVE_16BIT) 

#endif // !defined(DXC_VULKAN_EXTENSIONS)