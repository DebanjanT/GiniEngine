// PBR Vertex Shader for Diligent Engine (HLSL/SPIR-V compatible)
cbuffer CameraConstants : register(b0)
{
    float4x4 u_Model;
    float4x4 u_View;
    float4x4 u_Projection;
    float4x4 u_NormalMatrix;
    float3 u_CameraPos;
    float padding;
};

struct VSInput
{
    float3 a_Position : POSITION;
    float3 a_Normal : NORMAL;
    float2 a_TexCoords : TEXCOORD0;
    float3 a_Tangent : TANGENT;
    float3 a_Bitangent : BITANGENT;
    float4 a_Color : COLOR;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 v_WorldPos : WORLD_POS;
    float3 v_Normal : NORMAL;
    float2 v_TexCoords : TEXCOORD0;
    float3x3 v_TBN : TBN;
    float3 v_TangentViewDir : TANGENT_VIEW_DIR;
    float4 v_Color : COLOR;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    
    // Transform position to world space
    float4 worldPos = mul(u_Model, float4(input.a_Position, 1.0));
    output.v_WorldPos = worldPos.xyz;
    
    // Transform normal
    output.v_Normal = mul((float3x3)u_NormalMatrix, input.a_Normal);
    
    // Pass through texture coordinates
    output.v_TexCoords = input.a_TexCoords;
    output.v_Color = input.a_Color;
    
    // Calculate TBN matrix for normal mapping
    float3 T = normalize(mul((float3x3)u_NormalMatrix, input.a_Tangent));
    float3 B = normalize(mul((float3x3)u_NormalMatrix, input.a_Bitangent));
    float3 N = normalize(output.v_Normal);
    output.v_TBN = float3x3(T, B, N);
    
    // Calculate tangent-space view direction for parallax mapping
    float3x3 TBN_inv = transpose(output.v_TBN);
    output.v_TangentViewDir = mul(TBN_inv, (u_CameraPos - output.v_WorldPos));
    
    // Transform to clip space
    output.Position = mul(u_Projection, mul(u_View, worldPos));
    
    return output;
}
