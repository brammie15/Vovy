const float PI = 3.14159265359;


// GGX/Trowbridge-Reitz normal distribution function
// Measures the statistical distribution of microfacets
float NormalDistributionGGX(vec3 surfaceNormal, vec3 halfwayVector, float roughness)
{
    float roughnessSquared = roughness * roughness;
    float roughnessFourth = roughnessSquared * roughnessSquared;
    float normalDotHalf = max(dot(surfaceNormal, halfwayVector), 0.0);
    float normalDotHalfSquared = normalDotHalf * normalDotHalf;

    float numerator = roughnessFourth;
    float denominator = (normalDotHalfSquared * (roughnessFourth - 1.0) + 1.0);
    denominator = PI * denominator * denominator;

    return numerator / denominator;
}

// Schlick-GGX geometry function for a single direction
// Approximates the shadowing/masking of microfacets
float GeometrySchlickGGX_Direct(float normalDotView, float roughness)
{
    float roughnessFactor = (roughness + 1.0);
    float roughnessRedistribution = (roughnessFactor * roughnessFactor) / 8.0;

    float numerator = normalDotView;
    float denominator = normalDotView * (1.0 - roughnessRedistribution) + roughnessRedistribution;

    return numerator / denominator;
}

float GeometrySchlickGGX_Indirect(float normalDotView, float roughness)
{
    float a = roughness;
    float k = a * a / 2.0;

    float num = normalDotView;
    float denom = normalDotView * (1.0 - k) + k;

    return num / denom;
}
// Smith's method combining geometry obstruction for both view and light directions
float GeometrySmith(vec3 surfaceNormal, vec3 viewDirection, vec3 lightDirection, float roughness, bool indirectLighting)
{
    float normalDotView = max(dot(surfaceNormal, viewDirection), 0.0);
    float normalDotLight = max(dot(surfaceNormal, lightDirection), 0.0);
    float geometryView = 0.0;
    float geometryLight = 0.0;

    if(indirectLighting){
        geometryView = GeometrySchlickGGX_Indirect(normalDotView, roughness);
        geometryLight = GeometrySchlickGGX_Indirect(normalDotLight, roughness);
    } else {
        geometryView = GeometrySchlickGGX_Direct(normalDotView, roughness);
        geometryLight = GeometrySchlickGGX_Direct(normalDotLight, roughness);
    }

    return geometryView * geometryLight;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
