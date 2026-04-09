// PBR Pixel Shader for Diligent Engine (HLSL/SPIR-V compatible)
cbuffer MaterialConstants : register(b1)
{
    float3 u_Material_albedo;
    float u_Material_metallic;
    float u_Material_roughness;
    float u_Material_ao;
    float3 u_Material_emissive;
    float u_HeightScale;
    int u_UsePackedMetalRough;
    int u_HasAlbedoMap;
    int u_HasNormalMap;
    int u_HasMetallicMap;
    int u_HasRoughnessMap;
    int u_HasAOMap;
    int u_HasHeightMap;
};

cbuffer LightConstants : register(b2)
{
    float3 u_CameraPos;
    float padding1;
    float3 u_AmbientLight_color;
    float u_AmbientLight_intensity;
    int u_HasDirectionalLight;
    int u_PointLightCount;
    int u_SpotLightCount;
    int u_HasShadows;
    int u_HasIBL;
    float u_MaxReflectionLod;
    int u_CascadeCount;
    float2 padding2;
    
    float4x4 u_LightSpaceMatrices[3];
    float4 u_CascadeSplits[3];
    float4x4 u_View;
    
    // Directional light
    float3 u_DirectionalLight_direction;
    float u_DirectionalLight_intensity;
    float3 u_DirectionalLight_color;
    float padding3;
    
    // Point lights (simplified - would need structured buffer for full implementation)
    float4 u_PointLights[32]; // position.xyz, intensity.w
    float4 u_PointLightColors[32]; // color.rgb, radius.w
    float4 u_PointLightParams[32]; // constant.x, linear.y, quadratic.z, padding.w
    
    // Spot lights (simplified)
    float4 u_SpotLights[16]; // position.xyz, intensity.w
    float4 u_SpotLightDirs[16]; // direction.xyz, innerCutoff.w
    float4 u_SpotLightColors[16]; // color.rgb, outerCutoff.w
    float4 u_SpotLightParams[16]; // constant.x, linear.y, quadratic.z, padding.w
};

cbuffer IBLConstants : register(b3)
{
    // IBL textures will be bound as shader resource views
};

// Texture bindings
Texture2D u_AlbedoMap : register(t0);
Texture2D u_NormalMap : register(t1);
Texture2D u_MetallicMap : register(t2);
Texture2D u_RoughnessMap : register(t3);
Texture2D u_AOMap : register(t4);
Texture2D u_HeightMap : register(t5);
Texture2DArray u_ShadowMap : register(t6);
TextureCube u_IrradianceMap : register(t7);
TextureCube u_PrefilterMap : register(t8);
Texture2D u_BRDFLUT : register(t9);

// Samplers
SamplerState SamplerLinear : register(s0);
SamplerState SamplerShadow : register(s1);

struct PSInput
{
    float4 Position : SV_POSITION;
    float3 v_WorldPos : WORLD_POS;
    float3 v_Normal : NORMAL;
    float2 v_TexCoords : TEXCOORD0;
    float3x3 v_TBN : TBN;
    float3 v_TangentViewDir : TANGENT_VIEW_DIR;
    float4 v_Color : COLOR;
};

struct PSOutput
{
    float4 FragColor : SV_TARGET0;
    float3 gNormal : SV_TARGET1;
};

// PBR functions
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265359 * denom * denom;
    
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ShadowCalculation(float3 worldPos, float3 normal, float3 lightDir)
{
    if (u_HasShadows == 0) return 0.0;

    float4 viewPos = mul(u_View, float4(worldPos, 1.0));
    float depthValue = -viewPos.z;

    int cascade = u_CascadeCount - 1;
    for (int i = 0; i < u_CascadeCount; i++) {
        if (depthValue < u_CascadeSplits[i].x) {
            cascade = i;
            break;
        }
    }

    float4 lightSpacePos = mul(u_LightSpaceMatrices[cascade], float4(worldPos, 1.0));
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);

    float shadow = 0.0;
    // Simplified PCF - would need texture size for proper implementation
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float2 offset = float2(x, y) * 0.001; // Simplified texel size
            float pcfDepth = u_ShadowMap.Sample(SamplerShadow, float3(projCoords.xy + offset, float(cascade))).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

// Parallax Occlusion Mapping
float2 ParallaxMapping(float2 texCoords, float3 viewDir)
{
    const int minLayers = 8;
    const int maxLayers = 32;
    float numLayers = lerp(float(maxLayers), float(minLayers), abs(dot(float3(0.0, 0.0, 1.0), viewDir)));

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    float vz = max(abs(viewDir.z), 0.05);
    float2 P = viewDir.xy / vz * u_HeightScale;
    float2 deltaTexCoords = P / numLayers;

    float2 currentTexCoords = texCoords;
    float currentDepthMapValue = u_HeightMap.Sample(SamplerLinear, currentTexCoords).r;

    for (int i = 0; i < maxLayers; i++) {
        if (currentLayerDepth >= currentDepthMapValue) break;
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = u_HeightMap.Sample(SamplerLinear, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    // Occlusion interpolation for smoother results
    float2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = u_HeightMap.Sample(SamplerLinear, prevTexCoords).r - currentLayerDepth + layerDepth;
    float denom = afterDepth - beforeDepth;
    float weight = abs(denom) > 1e-5 ? (afterDepth / denom) : 0.0;
    return prevTexCoords * weight + currentTexCoords * (1.0 - weight);
}

PSOutput main(PSInput input)
{
    PSOutput output;
    
    // Apply parallax mapping to texture coordinates
    float2 texCoords = input.v_TexCoords;
    if (u_HasHeightMap == 1) {
        float3 tangentViewDir = normalize(input.v_TangentViewDir);
        texCoords = ParallaxMapping(input.v_TexCoords, tangentViewDir);
        // Avoid fragment discard speckle artifacts when sampled coords drift.
        texCoords = clamp(texCoords, float2(0.0, 0.0), float2(1.0, 1.0));
    }

    // Get material properties
    float3 albedo = u_Material_albedo;
    float metallic = u_Material_metallic;
    float roughness = u_Material_roughness;
    float ao = u_Material_ao;
    
    if (u_HasAlbedoMap == 1) {
        // Use albedo texture directly - GLB embedded textures are already in correct color space
        albedo *= u_AlbedoMap.Sample(SamplerLinear, texCoords).rgb;
    } else {
        // Many FBX assets rely on vertex colors when no albedo map exists.
        albedo *= input.v_Color.rgb;
    }
    
    if (u_HasMetallicMap == 1 && u_HasRoughnessMap == 1 && u_UsePackedMetalRough == 1) {
        // glTF metallicRoughness packed texture: G=roughness, B=metallic
        float4 mr = u_RoughnessMap.Sample(SamplerLinear, texCoords);
        roughness = mr.g;
        metallic = mr.b;
    } else {
        if (u_HasMetallicMap == 1) {
            metallic = u_MetallicMap.Sample(SamplerLinear, texCoords).r;
        }
        if (u_HasRoughnessMap == 1) {
            roughness = u_RoughnessMap.Sample(SamplerLinear, texCoords).r;
        }
    }
    if (u_HasAOMap == 1) {
        ao = u_AOMap.Sample(SamplerLinear, texCoords).r;
    }

    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.045, 1.0);
    
    // Normal mapping
    float3 N = normalize(input.v_Normal);
    if (u_HasNormalMap == 1) {
        N = u_NormalMap.Sample(SamplerLinear, texCoords).rgb;
        N = N * 2.0 - 1.0;
        N = normalize(mul(input.v_TBN, N));
    }
    
    float3 V = normalize(u_CameraPos - input.v_WorldPos);
    
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);
    
    float3 Lo = float3(0.0, 0.0, 0.0);
    
    // Directional light
    if (u_HasDirectionalLight == 1) {
        float3 L = normalize(-u_DirectionalLight_direction);
        float3 H = normalize(V + L);
        float3 radiance = u_DirectionalLight_color * u_DirectionalLight_intensity;
        
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denominator;
        
        float3 kS = F;
        float3 kD = float3(1.0, 1.0, 1.0) - kS;
        kD *= 1.0 - metallic;
        
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / 3.14159265359 + specular) * radiance * NdotL;
    }
    
    // Point lights (simplified implementation)
    for (int i = 0; i < u_PointLightCount; i++) {
        float3 L = normalize(u_PointLights[i].xyz - input.v_WorldPos);
        float3 H = normalize(V + L);
        float distance = length(u_PointLights[i].xyz - input.v_WorldPos);
        float attenuation = 1.0 / (u_PointLightParams[i].x + 
                                   u_PointLightParams[i].y * distance + 
                                   u_PointLightParams[i].z * distance * distance);
        float3 radiance = u_PointLightColors[i].rgb * u_PointLights[i].w * attenuation;
        
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denominator;
        
        float3 kS = F;
        float3 kD = float3(1.0, 1.0, 1.0) - kS;
        kD *= 1.0 - metallic;
        
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / 3.14159265359 + specular) * radiance * NdotL;
    }
    
    // Shadow
    float shadow = 0.0;
    if (u_HasDirectionalLight == 1) {
        float3 L = normalize(-u_DirectionalLight_direction);
        shadow = ShadowCalculation(input.v_WorldPos, N, L);
    }
    Lo *= (1.0 - shadow);

    // Ambient / IBL
    float3 ambient;
    if (u_HasIBL == 1) {
        float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        float3 kS = F;
        float3 kD = float3(1.0, 1.0, 1.0) - kS;
        kD *= 1.0 - metallic;

        float3 irradiance = u_IrradianceMap.Sample(SamplerLinear, N).rgb;
        float3 diffuseIBL = irradiance * albedo;

        float3 R = reflect(-V, N);
        float3 prefilteredColor = u_PrefilterMap.SampleLevel(SamplerLinear, R, roughness * u_MaxReflectionLod).rgb;
        float2 brdf = u_BRDFLUT.Sample(SamplerLinear, float2(max(dot(N, V), 0.0), roughness)).rg;
        float3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);

        ambient = (kD * diffuseIBL + specularIBL) * ao;
    } else {
        ambient = u_AmbientLight_color * u_AmbientLight_intensity * albedo * ao;
    }

    // Emissive
    float3 emissive = u_Material_emissive;

    float3 color = ambient + Lo + emissive;
    
    // Ensure base albedo is always visible to prevent white/gray appearance
    if (u_HasAlbedoMap == 1) {
        // Stronger albedo visibility fix to prevent white/gray appearance
        color = lerp(color, albedo * 0.8, 0.5);
    } else {
        // For materials without albedo textures, ensure base color is visible
        color = lerp(color, u_Material_albedo * 0.6, 0.4);
    }

    output.FragColor = float4(color, 1.0);
    output.gNormal = N * 0.5 + 0.5;
    
    return output;
}
