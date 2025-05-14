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
float GeometrySchlickGGX(float normalDotView, float roughness)
{
    float roughnessFactor = (roughness + 1.0);
    float roughnessRedistribution = (roughnessFactor * roughnessFactor) / 8.0;

    float numerator = normalDotView;
    float denominator = normalDotView * (1.0 - roughnessRedistribution) + roughnessRedistribution;

    return numerator / denominator;
}

// Smith's method combining geometry obstruction for both view and light directions
float GeometrySmith(vec3 surfaceNormal, vec3 viewDirection, vec3 lightDirection, float roughness)
{
    float normalDotView = max(dot(surfaceNormal, viewDirection), 0.0);
    float normalDotLight = max(dot(surfaceNormal, lightDirection), 0.0);
    float geometryView = GeometrySchlickGGX(normalDotView, roughness);
    float geometryLight = GeometrySchlickGGX(normalDotLight, roughness);

    return geometryView * geometryLight;
}