
#define PCAT_INTL(TOK1, TOK2) TOK1##TOK2
#define PCAT3_INTL(TOK1, TOK2, TOK3) TOK1##TOK2##TOK3
#define PCAT(TOK1, TOK2) PCAT_INTL(TOK1, TOK2)
#define PCAT3(TOK1, TOK2, TOK3) PCAT3_INTL(TOK1, TOK2, TOK3)

/** @brief Convert a 2 component RG pixel to a floating point vector
 *	@param value pixel to convert
 *  @return 32 bit floating point representation of the pixel
 */
//#pragma omp declare simd
inline vec2s PCAT(PIX_VECU, 2ToFloat2)(const PCAT(PIX_VEC, 2) value)
// static inline vec2s Fixed8v2ToFloat2(const fixed8v2* value)
{
	vec2s output =
	{
		PIX_TO_FLOAT(value.r),
		PIX_TO_FLOAT(value.g)
	};
	return output;
}

/** @brief Convert a 3 component RGB pixel to a floating point vector
 *	@param value pixel to convert
 *  @return 32 bit floating point representation of the pixel
 */
//#pragma omp declare simd
inline vec3s PCAT(PIX_VECU, 3ToFloat3)(const PCAT(PIX_VEC, 3) value)
// static inline vec3s Fixed8v3ToFloat3(const fixed8v3* value)
{
	vec3s output =
	{
		PIX_TO_FLOAT(value.r),
		PIX_TO_FLOAT(value.g),
		PIX_TO_FLOAT(value.b)
	};
	return output;
}

/** @brief Convert a 4 component RGBA pixel to a floating point vector
 *	@param value pixel to convert
 *  @return 32 bit floating point representation of the pixel
 */
//#pragma omp declare simd
inline vec4s PCAT(PIX_VECU, 4ToFloat4)(const PCAT(PIX_VEC, 4) value)
// static inline vec4s Fixed8v4ToFloat4(const fixed8v4* value)
{
	vec4s output =
	{
		PIX_TO_FLOAT(value.r),
		PIX_TO_FLOAT(value.g),
		PIX_TO_FLOAT(value.b),
		PIX_TO_FLOAT(value.a)
	};
	return output;
}

/** @brief Convert a 3 component RGB pixel to a 4 component floating point vector padded with 0's
 *	@param value pixel to convert
 *  @return 4 component 32 bit floating point representation of the pixel
 */
//#pragma omp declare simd
inline vec4s PCAT(PIX_VECU, 3ToFloat4)(const PCAT(PIX_VEC, 3) value)
// static inline vec4s Fixed8v3ToFloat4(const fixed8v3* value)
{
	vec4s output =
	{
		PIX_TO_FLOAT(value.r),
		PIX_TO_FLOAT(value.g),
		PIX_TO_FLOAT(value.b),
		0
	};
	return output;
}

/** @brief Convert a 2 component RGB pixel to a 4 component floating point vector padded with 0's
 *	@param value pixel to convert
 *  @return 4 component 32 bit floating point representation of the pixel
 */
//#pragma omp declare simd
inline vec4s PCAT(PIX_VECU, 2ToFloat4)(const PCAT(PIX_VEC, 2) value)
// static inline vec4s Fixed8v2ToFloat4(const fixed8v2* value)
{
	vec4s output =
	{
		PIX_TO_FLOAT(value.r),
		PIX_TO_FLOAT(value.g),
		0,
		0
	};
	return output;
}

/** @brief Convert a 2 wide floating point vector to a RG pixel struct
 *	@param value vector to convert
 *  @return the RG pixel value
 */
//#pragma omp declare simd
inline PCAT(PIX_VEC, 2) PCAT3(Float2To, PIX_VECU, 2)(const vec2s value)
// static inline fixed8v2 Float2ToFixed8v2(const vec3s* value)
{
	PCAT(PIX_VEC, 2) output =
	{
		FLOAT_TO_PIX(value.x),
		FLOAT_TO_PIX(value.y)
	};
	return output;
}

/** @brief Convert a 3 wide floating point vector to a RGB pixel struct
 *	@param value vector to convert
 *  @return the RGB pixel value
 */
//#pragma omp declare simd
inline PCAT(PIX_VEC, 3) PCAT3(Float3To, PIX_VECU, 3)(const vec3s value)
// static inline fixed8v3 Float3ToFixed8v3(const vec3s* value)
{
	PCAT(PIX_VEC, 3) output =
	{
		FLOAT_TO_PIX(value.r),
		FLOAT_TO_PIX(value.g),
		FLOAT_TO_PIX(value.b)
	};
	return output;
}


/** @brief Convert a 4 wide floating point vector to a RGBA pixel struct
 *	@param value vector to convert
 *  @return the RGBA pixel value
 */
//#pragma omp declare simd
inline PCAT(PIX_VEC, 4) PCAT3(Float4To, PIX_VECU, 4)(const vec4s value)
// static inline fixed8v4 Float4ToFixed8v2(const vec4s* value)
{
	PCAT(PIX_VEC, 4) output =
	{
		FLOAT_TO_PIX(value.r),
		FLOAT_TO_PIX(value.g),
		FLOAT_TO_PIX(value.b),
		FLOAT_TO_PIX(value.a)
	};
	return output;
}

/** @brief Convert the first 3 components of a 4 wide floating point vector to a RGB pixel struct
 *	@param value vector to convert
 *  @return the RGB pixel value
 */
//#pragma omp declare simd
static inline PCAT(PIX_VEC, 3) PCAT3(Float4To, PIX_VECU, 3)(const vec4s value)
// static inline fixed8v3 Float4ToFixed8v2(const vec4s* value)
{
	PCAT(PIX_VEC, 3) output =
	{
		FLOAT_TO_PIX(value.r),
		FLOAT_TO_PIX(value.g),
		FLOAT_TO_PIX(value.b)
	};
	return output;
}

/** @brief Convert the first 2 components of a 4 wide floating point vector to a RG pixel struct
 *	@param value vector to convert
 *  @return the RG pixel value
 */
//#pragma omp declare simd
static inline PCAT(PIX_VEC, 2) PCAT3(Float4To, PIX_VECU, 2)(const vec4s value)
// static inline fixed8v2 Float4ToFixed8v2(const vec4s* value)
{
	PCAT(PIX_VEC, 2) output =
	{
		FLOAT_TO_PIX(value.r),
		FLOAT_TO_PIX(value.g),
	};
	return output;
}


#undef PCAT_INTL
#undef PCAT3_INTL
#undef PCAT
#undef PCAT3