Shader "Custom/test"
{
SubShader { Pass {
Tags {"Lightmode"="UniversalForward"}
HLSLPROGRAM
#pragma use_dxc
#pragma vertex mainVtx
#pragma fragment mainFrag
template<typename TEST_TYPE> TEST_TYPE TestTemplate(TEST_TYPE value) { return 2 * value; } 
float4 mainVtx (float4 position : POSITION) : SV_Position  { return TestTemplate(position);  }
float4 mainFrag(float4 position : SV_Position) : SV_Target { return TestTemplate(position);  }  
ENDHLSL
}}
}
