cbuffer cbuf  
{  
  float4 g_vQuadRect;  
}  
Texture2D g_Texture2D;  
Texture3D g_Texture3D;  
TextureCube g_TextureCube;  
 
SamplerState samLinear{  
    Filter = MIN_MAG_LINEAR_MIP_POINT;  
}; 
 
struct Fragment{  
    float4 Pos : SV_POSITION; 
    float3 Tex : TEXCOORD0; 
}; 
 
Fragment VS( uint vertexId : SV_VertexID ) 
{ 
    Fragment f; 
    f.Tex = float3( 0.f, 0.f, 0.f); 
    if (vertexId == 1) f.Tex.x = 1.f; 
    else if (vertexId == 2) f.Tex.y = 1.f; 
    else if (vertexId == 3) f.Tex.xy = float2(1.f, 1.f); 
     
    f.Pos = float4( g_vQuadRect.xy + f.Tex * g_vQuadRect.zw, 0, 1);    
	return f; 
} 
 
float4 PS(Fragment f) : SV_Target 
{ 
    return g_Texture2D.Sample( samLinear, f.Tex.xy );  
} 
