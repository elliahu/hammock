struct Constants {
    float4 color;
};

[vk::push_constant]
Constants constants;

[vk::binding(0)]
RWTexture2D image;

[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(uint3 threadId: SV_DispatchThreadID) {
    image[threadId.xy] = constants.color;
}
