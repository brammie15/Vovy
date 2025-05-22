float CalculateEV100FromPhysicalCamera(float apature, float shutterTime, float iso)
{
    // Calculate EV100 from physical camera settings
    // EV100 = log2((aperture^2) / (shutterSpeed * iso))
    return log2(pow(apature, 2) / shutterTime * 100 / iso);
}

float ConvertEV100ToExposure(float EV100){
    const float maxLuminance = 1.2f * pow(2.f, EV100);
    return 1.f / max(maxLuminance, 0.0001f);
}

float CalculateEV100FromAverageLuminance(float averageLuminance) {
    const float K = 12.5f;
    return log2((averageLuminance * 100.f) / K);
}