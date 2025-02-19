// Copyright 2019 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tests/DawnTest.h"

#include "common/Assert.h"
#include "common/Math.h"
#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/TextureFormatUtils.h"
#include "utils/WGPUHelpers.h"

#include <cmath>
#include <type_traits>

// An expectation for float buffer content that can correctly compare different NaN values and
// supports a basic tolerance for comparison of finite values.
class ExpectFloatWithTolerance : public detail::Expectation {
  public:
    ExpectFloatWithTolerance(std::vector<float> expected, float tolerance)
        : mExpected(std::move(expected)), mTolerance(tolerance) {
    }

    testing::AssertionResult Check(const void* data, size_t size) override {
        ASSERT(size == sizeof(float) * mExpected.size());

        const float* actual = static_cast<const float*>(data);

        for (size_t i = 0; i < mExpected.size(); ++i) {
            float expectedValue = mExpected[i];
            float actualValue = actual[i];

            if (!FloatsMatch(expectedValue, actualValue)) {
                testing::AssertionResult result = testing::AssertionFailure()
                                                  << "Expected data[" << i << "] to be close to "
                                                  << expectedValue << ", actual " << actualValue
                                                  << std::endl;
                return result;
            }
        }
        return testing::AssertionSuccess();
    }

  private:
    bool FloatsMatch(float expected, float actual) {
        if (std::isnan(expected)) {
            return std::isnan(actual);
        }

        if (std::isinf(expected)) {
            return std::isinf(actual) && std::signbit(expected) == std::signbit(actual);
        }

        if (mTolerance == 0.0f) {
            return expected == actual;
        }

        float error = std::abs(expected - actual);
        return error < mTolerance;
    }

    std::vector<float> mExpected;
    float mTolerance;
};

// An expectation for float16 buffers that can correctly compare NaNs (all NaNs are equivalent).
class ExpectFloat16 : public detail::Expectation {
  public:
    ExpectFloat16(std::vector<uint16_t> expected) : mExpected(std::move(expected)) {
    }

    testing::AssertionResult Check(const void* data, size_t size) override {
        ASSERT(size == sizeof(uint16_t) * mExpected.size());

        const uint16_t* actual = static_cast<const uint16_t*>(data);

        for (size_t i = 0; i < mExpected.size(); ++i) {
            uint16_t expectedValue = mExpected[i];
            uint16_t actualValue = actual[i];

            if (!Floats16Match(expectedValue, actualValue)) {
                testing::AssertionResult result = testing::AssertionFailure()
                                                  << "Expected data[" << i << "] to be "
                                                  << expectedValue << ", actual " << actualValue
                                                  << std::endl;
                return result;
            }
        }
        return testing::AssertionSuccess();
    }

  private:
    bool Floats16Match(float expected, float actual) {
        if (IsFloat16NaN(expected)) {
            return IsFloat16NaN(actual);
        }

        return expected == actual;
    }

    std::vector<uint16_t> mExpected;
};

class TextureFormatTest : public DawnTest {
  protected:
    void SetUp() {
        DawnTest::SetUp();
    }

    // Structure containing all the information that tests need to know about the format.
    struct FormatTestInfo {
        wgpu::TextureFormat format;
        uint32_t texelByteSize;
        wgpu::TextureComponentType type;
        uint32_t componentCount;
    };

    // Returns a reprensentation of a format that can be used to contain the "uncompressed" values
    // of the format. That the equivalent format with all channels 32bit-sized.
    FormatTestInfo GetUncompressedFormatInfo(FormatTestInfo formatInfo) {
        switch (formatInfo.type) {
            case wgpu::TextureComponentType::Float:
                return {wgpu::TextureFormat::RGBA32Float, 16, formatInfo.type, 4};
            case wgpu::TextureComponentType::Sint:
                return {wgpu::TextureFormat::RGBA32Sint, 16, formatInfo.type, 4};
            case wgpu::TextureComponentType::Uint:
                return {wgpu::TextureFormat::RGBA32Uint, 16, formatInfo.type, 4};
            default:
                UNREACHABLE();
        }
    }

    // Return a pipeline that can be used in a full-texture draw to sample from the texture in the
    // bindgroup and output its decompressed values to the render target.
    wgpu::RenderPipeline CreateSamplePipeline(FormatTestInfo sampleFormatInfo,
                                              FormatTestInfo renderFormatInfo) {
        utils::ComboRenderPipelineDescriptor desc(device);

        wgpu::ShaderModule vsModule = utils::CreateShaderModuleFromWGSL(device, R"(
            [[builtin(vertex_index)]] var<in> VertexIndex : u32;
            [[builtin(position)]] var<out> Position : vec4<f32>;

            [[stage(vertex)]] fn main() -> void {
                const pos : array<vec2<f32>, 3> = array<vec2<f32>, 3>(
                    vec2<f32>(-3.0, -1.0),
                    vec2<f32>( 3.0, -1.0),
                    vec2<f32>( 0.0,  2.0));

                Position = vec4<f32>(pos[VertexIndex], 0.0, 1.0);
            })");

        // Compute the WGSL type of the texture's data.
        const char* type = utils::GetWGSLColorTextureComponentType(sampleFormatInfo.format);

        std::ostringstream fsSource;
        fsSource << "[[group(0), binding(0)]] var myTexture : texture_2d<" << type << ">;\n";
        fsSource << "[[builtin(frag_coord)]] var<in> FragCoord : vec4<f32>;\n";
        fsSource << "[[location(0)]] var<out> fragColor : vec4<" << type << ">;\n";
        fsSource << "[[stage(fragment)]] fn main() -> void {\n";
        fsSource << "    fragColor = textureLoad(myTexture, vec2<i32>(FragCoord.xy), 0);\n";
        fsSource << "}";

        wgpu::ShaderModule fsModule =
            utils::CreateShaderModuleFromWGSL(device, fsSource.str().c_str());

        desc.vertexStage.module = vsModule;
        desc.cFragmentStage.module = fsModule;
        desc.cColorStates[0].format = renderFormatInfo.format;

        return device.CreateRenderPipeline(&desc);
    }

    // The sampling test uploads the sample data in a texture with the sampleFormatInfo.format.
    // It then samples from it and renders the results in a texture with the
    // renderFormatInfo.format format. Finally it checks that the data rendered matches
    // expectedRenderData, using the cutom expectation if present.
    void DoSampleTest(FormatTestInfo sampleFormatInfo,
                      const void* sampleData,
                      size_t sampleDataSize,
                      FormatTestInfo renderFormatInfo,
                      const void* expectedRenderData,
                      size_t expectedRenderDataSize,
                      detail::Expectation* customExpectation) {
        // The input data should contain an exact number of texels
        ASSERT(sampleDataSize % sampleFormatInfo.texelByteSize == 0);
        uint32_t width = sampleDataSize / sampleFormatInfo.texelByteSize;

        // The input data must be a multiple of 4 byte in length for WriteBuffer
        ASSERT(sampleDataSize % 4 == 0);
        ASSERT(expectedRenderDataSize % 4 == 0);

        // Create the texture we will sample from
        wgpu::TextureDescriptor sampleTextureDesc;
        sampleTextureDesc.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::Sampled;
        sampleTextureDesc.size = {width, 1, 1};
        sampleTextureDesc.format = sampleFormatInfo.format;
        wgpu::Texture sampleTexture = device.CreateTexture(&sampleTextureDesc);

        wgpu::Buffer uploadBuffer = utils::CreateBufferFromData(device, sampleData, sampleDataSize,
                                                                wgpu::BufferUsage::CopySrc);

        // Create the texture that we will render results to
        ASSERT(expectedRenderDataSize == width * renderFormatInfo.texelByteSize);

        wgpu::TextureDescriptor renderTargetDesc;
        renderTargetDesc.usage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::RenderAttachment;
        renderTargetDesc.size = {width, 1, 1};
        renderTargetDesc.format = renderFormatInfo.format;

        wgpu::Texture renderTarget = device.CreateTexture(&renderTargetDesc);

        // Create the readback buffer for the data in renderTarget
        wgpu::BufferDescriptor readbackBufferDesc;
        readbackBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc;
        readbackBufferDesc.size = expectedRenderDataSize;
        wgpu::Buffer readbackBuffer = device.CreateBuffer(&readbackBufferDesc);

        // Prepare objects needed to sample from texture in the renderpass
        wgpu::RenderPipeline pipeline = CreateSamplePipeline(sampleFormatInfo, renderFormatInfo);
        wgpu::BindGroup bindGroup = utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0),
                                                         {{0, sampleTexture.CreateView()}});

        // Encode commands for the test that fill texture, sample it to render to renderTarget then
        // copy renderTarget in a buffer so we can read it easily.
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        {
            wgpu::BufferCopyView bufferView = utils::CreateBufferCopyView(uploadBuffer, 0, 256);
            wgpu::TextureCopyView textureView =
                utils::CreateTextureCopyView(sampleTexture, 0, {0, 0, 0});
            wgpu::Extent3D extent{width, 1, 1};
            encoder.CopyBufferToTexture(&bufferView, &textureView, &extent);
        }

        utils::ComboRenderPassDescriptor renderPassDesc({renderTarget.CreateView()});
        wgpu::RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDesc);
        renderPass.SetPipeline(pipeline);
        renderPass.SetBindGroup(0, bindGroup);
        renderPass.Draw(3);
        renderPass.EndPass();

        {
            wgpu::BufferCopyView bufferView = utils::CreateBufferCopyView(readbackBuffer, 0, 256);
            wgpu::TextureCopyView textureView =
                utils::CreateTextureCopyView(renderTarget, 0, {0, 0, 0});
            wgpu::Extent3D extent{width, 1, 1};
            encoder.CopyTextureToBuffer(&textureView, &bufferView, &extent);
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);

        // For floats use a special expectation that understands how to compare NaNs and support a
        // tolerance.
        if (customExpectation != nullptr) {
            AddBufferExpectation(__FILE__, __LINE__, readbackBuffer, 0, expectedRenderDataSize,
                                 customExpectation);
        } else {
            EXPECT_BUFFER_U32_RANGE_EQ(static_cast<const uint32_t*>(expectedRenderData),
                                       readbackBuffer, 0,
                                       expectedRenderDataSize / sizeof(uint32_t));
        }
    }

    template <typename Data>
    std::vector<Data> ExpandDataTo4Component(const std::vector<Data>& originalData,
                                             uint32_t originalComponentCount,
                                             const std::array<Data, 4>& defaultValues) {
        std::vector<Data> result;

        for (size_t i = 0; i < originalData.size() / originalComponentCount; i++) {
            for (size_t component = 0; component < 4; component++) {
                if (component < originalComponentCount) {
                    result.push_back(originalData[i * originalComponentCount + component]);
                } else {
                    result.push_back(defaultValues[component]);
                }
            }
        }

        return result;
    }

    // Helper functions used to run tests that convert the typeful test objects to typeless void*

    template <typename TextureData, typename RenderData>
    void DoFormatSamplingTest(FormatTestInfo formatInfo,
                              const std::vector<TextureData>& textureData,
                              const std::vector<RenderData>& expectedRenderData,
                              detail::Expectation* customExpectation = nullptr) {
        FormatTestInfo renderFormatInfo = GetUncompressedFormatInfo(formatInfo);

        // Expand the expected data to be 4 component wide with the default sampling values of
        // (0, 0, 0, 1)
        std::array<RenderData, 4> defaultValues = {RenderData(0), RenderData(0), RenderData(0),
                                                   RenderData(1)};
        std::vector<RenderData> expandedRenderData =
            ExpandDataTo4Component(expectedRenderData, formatInfo.componentCount, defaultValues);

        DoSampleTest(formatInfo, textureData.data(), textureData.size() * sizeof(TextureData),
                     renderFormatInfo, expandedRenderData.data(),
                     expandedRenderData.size() * sizeof(RenderData), customExpectation);
    }

    template <typename TextureData>
    void DoFloatFormatSamplingTest(FormatTestInfo formatInfo,
                                   const std::vector<TextureData>& textureData,
                                   const std::vector<float>& expectedRenderData,
                                   float floatTolerance = 0.0f) {
        // Expand the expected data to be 4 component wide with the default sampling values of
        // (0, 0, 0, 1)
        std::array<float, 4> defaultValues = {0.0f, 0.0f, 0.0f, 1.0f};
        std::vector<float> expandedRenderData =
            ExpandDataTo4Component(expectedRenderData, formatInfo.componentCount, defaultValues);

        // Use a special expectation that understands how to compare NaNs and supports a tolerance.
        DoFormatSamplingTest(formatInfo, textureData, expectedRenderData,
                             new ExpectFloatWithTolerance(expandedRenderData, floatTolerance));
    }

    template <typename TextureData, typename RenderData>
    void DoFormatRenderingTest(FormatTestInfo formatInfo,
                               const std::vector<TextureData>& textureData,
                               const std::vector<RenderData>& expectedRenderData,
                               detail::Expectation* customExpectation = nullptr) {
        FormatTestInfo sampleFormatInfo = GetUncompressedFormatInfo(formatInfo);

        // Expand the sampling texture data to contain garbage data for unused components to check
        // that they don't influence the rendering result.
        std::array<TextureData, 4> garbageValues;
        garbageValues.fill(13);
        std::vector<TextureData> expandedTextureData =
            ExpandDataTo4Component(textureData, formatInfo.componentCount, garbageValues);

        DoSampleTest(sampleFormatInfo, expandedTextureData.data(),
                     expandedTextureData.size() * sizeof(TextureData), formatInfo,
                     expectedRenderData.data(), expectedRenderData.size() * sizeof(RenderData),
                     customExpectation);
    }

    // Below are helper functions for types that are very similar to one another so the logic is
    // shared.

    template <typename T>
    void DoUnormTest(FormatTestInfo formatInfo) {
        static_assert(!std::is_signed<T>::value && std::is_integral<T>::value, "");
        ASSERT(sizeof(T) * formatInfo.componentCount == formatInfo.texelByteSize);
        ASSERT(formatInfo.type == wgpu::TextureComponentType::Float);

        T maxValue = std::numeric_limits<T>::max();
        std::vector<T> textureData = {0, 1, maxValue, maxValue};
        std::vector<float> uncompressedData = {0.0f, 1.0f / maxValue, 1.0f, 1.0f};

        DoFormatSamplingTest(formatInfo, textureData, uncompressedData);
        DoFormatRenderingTest(formatInfo, uncompressedData, textureData);
    }

    template <typename T>
    void DoSnormTest(FormatTestInfo formatInfo) {
        static_assert(std::is_signed<T>::value && std::is_integral<T>::value, "");
        ASSERT(sizeof(T) * formatInfo.componentCount == formatInfo.texelByteSize);
        ASSERT(formatInfo.type == wgpu::TextureComponentType::Float);

        T maxValue = std::numeric_limits<T>::max();
        T minValue = std::numeric_limits<T>::min();
        std::vector<T> textureData = {0, 1, -1, maxValue, minValue, T(minValue + 1), 0, 0};
        std::vector<float> uncompressedData = {
            0.0f, 1.0f / maxValue, -1.0f / maxValue, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f};

        DoFloatFormatSamplingTest(formatInfo, textureData, uncompressedData, 0.0001f / maxValue);
        // Snorm formats aren't renderable because they are not guaranteed renderable in Vulkan
    }

    template <typename T>
    void DoUintTest(FormatTestInfo formatInfo) {
        static_assert(!std::is_signed<T>::value && std::is_integral<T>::value, "");
        ASSERT(sizeof(T) * formatInfo.componentCount == formatInfo.texelByteSize);
        ASSERT(formatInfo.type == wgpu::TextureComponentType::Uint);

        T maxValue = std::numeric_limits<T>::max();
        std::vector<T> textureData = {0, 1, maxValue, maxValue};
        std::vector<uint32_t> uncompressedData = {0, 1, maxValue, maxValue};

        DoFormatSamplingTest(formatInfo, textureData, uncompressedData);
        DoFormatRenderingTest(formatInfo, uncompressedData, textureData);
    }

    template <typename T>
    void DoSintTest(FormatTestInfo formatInfo) {
        static_assert(std::is_signed<T>::value && std::is_integral<T>::value, "");
        ASSERT(sizeof(T) * formatInfo.componentCount == formatInfo.texelByteSize);
        ASSERT(formatInfo.type == wgpu::TextureComponentType::Sint);

        T maxValue = std::numeric_limits<T>::max();
        T minValue = std::numeric_limits<T>::min();
        std::vector<T> textureData = {0, 1, maxValue, minValue};
        std::vector<int32_t> uncompressedData = {0, 1, maxValue, minValue};

        DoFormatSamplingTest(formatInfo, textureData, uncompressedData);
        DoFormatRenderingTest(formatInfo, uncompressedData, textureData);
    }

    void DoFloat32Test(FormatTestInfo formatInfo) {
        ASSERT(sizeof(float) * formatInfo.componentCount == formatInfo.texelByteSize);
        ASSERT(formatInfo.type == wgpu::TextureComponentType::Float);

        std::vector<float> textureData = {+0.0f,   -0.0f, 1.0f,     1.0e-29f,
                                          1.0e29f, NAN,   INFINITY, -INFINITY};

        DoFloatFormatSamplingTest(formatInfo, textureData, textureData);
        DoFormatRenderingTest(formatInfo, textureData, textureData,
                              new ExpectFloatWithTolerance(textureData, 0.0f));
    }

    void DoFloat16Test(FormatTestInfo formatInfo) {
        ASSERT(sizeof(int16_t) * formatInfo.componentCount == formatInfo.texelByteSize);
        ASSERT(formatInfo.type == wgpu::TextureComponentType::Float);

        std::vector<float> uncompressedData = {+0.0f,  -0.0f, 1.0f,     1.01e-4f,
                                               1.0e4f, NAN,   INFINITY, -INFINITY};
        std::vector<uint16_t> textureData;
        for (float value : uncompressedData) {
            textureData.push_back(Float32ToFloat16(value));
        }

        DoFloatFormatSamplingTest(formatInfo, textureData, uncompressedData, 1.0e-5f);

        // Use a special expectation that knows that all Float16 NaNs are equivalent.
        DoFormatRenderingTest(formatInfo, uncompressedData, textureData,
                              new ExpectFloat16(textureData));
    }
};

// Test the R8Unorm format
TEST_P(TextureFormatTest, R8Unorm) {
    DoUnormTest<uint8_t>({wgpu::TextureFormat::R8Unorm, 1, wgpu::TextureComponentType::Float, 1});
}

// Test the RG8Unorm format
TEST_P(TextureFormatTest, RG8Unorm) {
    DoUnormTest<uint8_t>({wgpu::TextureFormat::RG8Unorm, 2, wgpu::TextureComponentType::Float, 2});
}

// Test the RGBA8Unorm format
TEST_P(TextureFormatTest, RGBA8Unorm) {
    DoUnormTest<uint8_t>(
        {wgpu::TextureFormat::RGBA8Unorm, 4, wgpu::TextureComponentType::Float, 4});
}

// Test the BGRA8Unorm format
TEST_P(TextureFormatTest, BGRA8Unorm) {
    // TODO(crbug.com/dawn/596): BGRA is unsupported on OpenGL ES; add workaround or validation
    DAWN_SKIP_TEST_IF(IsOpenGLES());
    uint8_t maxValue = std::numeric_limits<uint8_t>::max();
    std::vector<uint8_t> textureData = {maxValue, 1, 0, maxValue};
    std::vector<float> uncompressedData = {0.0f, 1.0f / maxValue, 1.0f, 1.0f};
    DoFormatSamplingTest({wgpu::TextureFormat::BGRA8Unorm, 4, wgpu::TextureComponentType::Float, 4},
                         textureData, uncompressedData);
    DoFormatRenderingTest(
        {wgpu::TextureFormat::BGRA8Unorm, 4, wgpu::TextureComponentType::Float, 4},
        uncompressedData, textureData);
}

// Test the R8Snorm format
TEST_P(TextureFormatTest, R8Snorm) {
    DoSnormTest<int8_t>({wgpu::TextureFormat::R8Snorm, 1, wgpu::TextureComponentType::Float, 1});
}

// Test the RG8Snorm format
TEST_P(TextureFormatTest, RG8Snorm) {
    DoSnormTest<int8_t>({wgpu::TextureFormat::RG8Snorm, 2, wgpu::TextureComponentType::Float, 2});
}

// Test the RGBA8Snorm format
TEST_P(TextureFormatTest, RGBA8Snorm) {
    DoSnormTest<int8_t>({wgpu::TextureFormat::RGBA8Snorm, 4, wgpu::TextureComponentType::Float, 4});
}

// Test the R8Uint format
TEST_P(TextureFormatTest, R8Uint) {
    DoUintTest<uint8_t>({wgpu::TextureFormat::R8Uint, 1, wgpu::TextureComponentType::Uint, 1});
}

// Test the RG8Uint format
TEST_P(TextureFormatTest, RG8Uint) {
    DoUintTest<uint8_t>({wgpu::TextureFormat::RG8Uint, 2, wgpu::TextureComponentType::Uint, 2});
}

// Test the RGBA8Uint format
TEST_P(TextureFormatTest, RGBA8Uint) {
    DoUintTest<uint8_t>({wgpu::TextureFormat::RGBA8Uint, 4, wgpu::TextureComponentType::Uint, 4});
}

// Test the R16Uint format
TEST_P(TextureFormatTest, R16Uint) {
    DoUintTest<uint16_t>({wgpu::TextureFormat::R16Uint, 2, wgpu::TextureComponentType::Uint, 1});
}

// Test the RG16Uint format
TEST_P(TextureFormatTest, RG16Uint) {
    DoUintTest<uint16_t>({wgpu::TextureFormat::RG16Uint, 4, wgpu::TextureComponentType::Uint, 2});
}

// Test the RGBA16Uint format
TEST_P(TextureFormatTest, RGBA16Uint) {
    DoUintTest<uint16_t>({wgpu::TextureFormat::RGBA16Uint, 8, wgpu::TextureComponentType::Uint, 4});
}

// Test the R32Uint format
TEST_P(TextureFormatTest, R32Uint) {
    DoUintTest<uint32_t>({wgpu::TextureFormat::R32Uint, 4, wgpu::TextureComponentType::Uint, 1});
}

// Test the RG32Uint format
TEST_P(TextureFormatTest, RG32Uint) {
    DoUintTest<uint32_t>({wgpu::TextureFormat::RG32Uint, 8, wgpu::TextureComponentType::Uint, 2});
}

// Test the RGBA32Uint format
TEST_P(TextureFormatTest, RGBA32Uint) {
    DoUintTest<uint32_t>(
        {wgpu::TextureFormat::RGBA32Uint, 16, wgpu::TextureComponentType::Uint, 4});
}

// Test the R8Sint format
TEST_P(TextureFormatTest, R8Sint) {
    DoSintTest<int8_t>({wgpu::TextureFormat::R8Sint, 1, wgpu::TextureComponentType::Sint, 1});
}

// Test the RG8Sint format
TEST_P(TextureFormatTest, RG8Sint) {
    DoSintTest<int8_t>({wgpu::TextureFormat::RG8Sint, 2, wgpu::TextureComponentType::Sint, 2});
}

// Test the RGBA8Sint format
TEST_P(TextureFormatTest, RGBA8Sint) {
    DoSintTest<int8_t>({wgpu::TextureFormat::RGBA8Sint, 4, wgpu::TextureComponentType::Sint, 4});
}

// Test the R16Sint format
TEST_P(TextureFormatTest, R16Sint) {
    DoSintTest<int16_t>({wgpu::TextureFormat::R16Sint, 2, wgpu::TextureComponentType::Sint, 1});
}

// Test the RG16Sint format
TEST_P(TextureFormatTest, RG16Sint) {
    DoSintTest<int16_t>({wgpu::TextureFormat::RG16Sint, 4, wgpu::TextureComponentType::Sint, 2});
}

// Test the RGBA16Sint format
TEST_P(TextureFormatTest, RGBA16Sint) {
    DoSintTest<int16_t>({wgpu::TextureFormat::RGBA16Sint, 8, wgpu::TextureComponentType::Sint, 4});
}

// Test the R32Sint format
TEST_P(TextureFormatTest, R32Sint) {
    DoSintTest<int32_t>({wgpu::TextureFormat::R32Sint, 4, wgpu::TextureComponentType::Sint, 1});
}

// Test the RG32Sint format
TEST_P(TextureFormatTest, RG32Sint) {
    DoSintTest<int32_t>({wgpu::TextureFormat::RG32Sint, 8, wgpu::TextureComponentType::Sint, 2});
}

// Test the RGBA32Sint format
TEST_P(TextureFormatTest, RGBA32Sint) {
    DoSintTest<int32_t>({wgpu::TextureFormat::RGBA32Sint, 16, wgpu::TextureComponentType::Sint, 4});
}

// Test the R32Float format
TEST_P(TextureFormatTest, R32Float) {
    DoFloat32Test({wgpu::TextureFormat::R32Float, 4, wgpu::TextureComponentType::Float, 1});
}

// Test the RG32Float format
TEST_P(TextureFormatTest, RG32Float) {
    DoFloat32Test({wgpu::TextureFormat::RG32Float, 8, wgpu::TextureComponentType::Float, 2});
}

// Test the RGBA32Float format
TEST_P(TextureFormatTest, RGBA32Float) {
    DoFloat32Test({wgpu::TextureFormat::RGBA32Float, 16, wgpu::TextureComponentType::Float, 4});
}

// Test the R16Float format
TEST_P(TextureFormatTest, R16Float) {
    // TODO(https://crbug.com/swiftshader/147) Rendering INFINITY isn't handled correctly by
    // swiftshader
    DAWN_SKIP_TEST_IF(IsVulkan() && IsSwiftshader() || IsANGLE());

    DoFloat16Test({wgpu::TextureFormat::R16Float, 2, wgpu::TextureComponentType::Float, 1});
}

// Test the RG16Float format
TEST_P(TextureFormatTest, RG16Float) {
    // TODO(https://crbug.com/swiftshader/147) Rendering INFINITY isn't handled correctly by
    // swiftshader
    DAWN_SKIP_TEST_IF(IsVulkan() && IsSwiftshader() || IsANGLE());

    DoFloat16Test({wgpu::TextureFormat::RG16Float, 4, wgpu::TextureComponentType::Float, 2});
}

// Test the RGBA16Float format
TEST_P(TextureFormatTest, RGBA16Float) {
    // TODO(https://crbug.com/swiftshader/147) Rendering INFINITY isn't handled correctly by
    // swiftshader
    DAWN_SKIP_TEST_IF(IsVulkan() && IsSwiftshader() || IsANGLE());

    DoFloat16Test({wgpu::TextureFormat::RGBA16Float, 8, wgpu::TextureComponentType::Float, 4});
}

// Test the RGBA8Unorm format
TEST_P(TextureFormatTest, RGBA8UnormSrgb) {
    uint8_t maxValue = std::numeric_limits<uint8_t>::max();
    std::vector<uint8_t> textureData = {0, 1, maxValue, 64, 35, 68, 152, 168};

    std::vector<float> uncompressedData;
    for (size_t i = 0; i < textureData.size(); i += 4) {
        uncompressedData.push_back(SRGBToLinear(textureData[i + 0] / float(maxValue)));
        uncompressedData.push_back(SRGBToLinear(textureData[i + 1] / float(maxValue)));
        uncompressedData.push_back(SRGBToLinear(textureData[i + 2] / float(maxValue)));
        // Alpha is linear for sRGB formats
        uncompressedData.push_back(textureData[i + 3] / float(maxValue));
    }

    DoFloatFormatSamplingTest(
        {wgpu::TextureFormat::RGBA8UnormSrgb, 4, wgpu::TextureComponentType::Float, 4}, textureData,
        uncompressedData, 1.0e-3);
    DoFormatRenderingTest(
        {wgpu::TextureFormat::RGBA8UnormSrgb, 4, wgpu::TextureComponentType::Float, 4},
        uncompressedData, textureData);
}

// Test the BGRA8UnormSrgb format
TEST_P(TextureFormatTest, BGRA8UnormSrgb) {
    // TODO(cwallez@chromium.org): This format doesn't exist in OpenGL, emulate it using
    // RGBA8UnormSrgb and swizzling / shader twiddling
    DAWN_SKIP_TEST_IF(IsOpenGL());
    DAWN_SKIP_TEST_IF(IsOpenGLES());

    uint8_t maxValue = std::numeric_limits<uint8_t>::max();
    std::vector<uint8_t> textureData = {0, 1, maxValue, 64, 35, 68, 152, 168};

    std::vector<float> uncompressedData;
    for (size_t i = 0; i < textureData.size(); i += 4) {
        // Note that R and B are swapped
        uncompressedData.push_back(SRGBToLinear(textureData[i + 2] / float(maxValue)));
        uncompressedData.push_back(SRGBToLinear(textureData[i + 1] / float(maxValue)));
        uncompressedData.push_back(SRGBToLinear(textureData[i + 0] / float(maxValue)));
        // Alpha is linear for sRGB formats
        uncompressedData.push_back(textureData[i + 3] / float(maxValue));
    }

    DoFloatFormatSamplingTest(
        {wgpu::TextureFormat::BGRA8UnormSrgb, 4, wgpu::TextureComponentType::Float, 4}, textureData,
        uncompressedData, 1.0e-3);
    DoFormatRenderingTest(
        {wgpu::TextureFormat::BGRA8UnormSrgb, 4, wgpu::TextureComponentType::Float, 4},
        uncompressedData, textureData);
}

// Test the RGB10A2Unorm format
TEST_P(TextureFormatTest, RGB10A2Unorm) {
    auto MakeRGB10A2 = [](uint32_t r, uint32_t g, uint32_t b, uint32_t a) -> uint32_t {
        ASSERT((r & 0x3FF) == r);
        ASSERT((g & 0x3FF) == g);
        ASSERT((b & 0x3FF) == b);
        ASSERT((a & 0x3) == a);
        return r | g << 10 | b << 20 | a << 30;
    };

    std::vector<uint32_t> textureData = {MakeRGB10A2(0, 0, 0, 0), MakeRGB10A2(1023, 1023, 1023, 1),
                                         MakeRGB10A2(243, 576, 765, 2), MakeRGB10A2(0, 0, 0, 3)};
    // clang-format off
    std::vector<float> uncompressedData = {
       0.0f, 0.0f, 0.0f, 0.0f,
       1.0f, 1.0f, 1.0f, 1 / 3.0f,
        243 / 1023.0f, 576 / 1023.0f, 765 / 1023.0f, 2 / 3.0f,
       0.0f, 0.0f, 0.0f, 1.0f
    };
    // clang-format on

    DoFloatFormatSamplingTest(
        {wgpu::TextureFormat::RGB10A2Unorm, 4, wgpu::TextureComponentType::Float, 4}, textureData,
        uncompressedData, 1.0e-5);
    DoFormatRenderingTest(
        {wgpu::TextureFormat::RGB10A2Unorm, 4, wgpu::TextureComponentType::Float, 4},
        uncompressedData, textureData);
}

// Test the RG11B10Ufloat format
TEST_P(TextureFormatTest, RG11B10Ufloat) {
    constexpr uint32_t kFloat11Zero = 0;
    constexpr uint32_t kFloat11Infinity = 0x7C0;
    constexpr uint32_t kFloat11Nan = 0x7C1;
    constexpr uint32_t kFloat11One = 0x3C0;

    constexpr uint32_t kFloat10Zero = 0;
    constexpr uint32_t kFloat10Infinity = 0x3E0;
    constexpr uint32_t kFloat10Nan = 0x3E1;
    constexpr uint32_t kFloat10One = 0x1E0;

    auto MakeRG11B10 = [](uint32_t r, uint32_t g, uint32_t b) {
        ASSERT((r & 0x7FF) == r);
        ASSERT((g & 0x7FF) == g);
        ASSERT((b & 0x3FF) == b);
        return r | g << 11 | b << 22;
    };

    // Test each of (0, 1, INFINITY, NaN) for each component but never two with the same value at a
    // time.
    std::vector<uint32_t> textureData = {
        MakeRG11B10(kFloat11Zero, kFloat11Infinity, kFloat10Nan),
        MakeRG11B10(kFloat11Infinity, kFloat11Nan, kFloat10One),
        MakeRG11B10(kFloat11Nan, kFloat11One, kFloat10Zero),
        MakeRG11B10(kFloat11One, kFloat11Zero, kFloat10Infinity),
    };

    // This is one of the only 3-channel formats, so we don't have specific testing for them. Alpha
    // should always be sampled as 1
    // clang-format off
    std::vector<float> uncompressedData = {
        0.0f,     INFINITY, NAN,      1.0f,
        INFINITY, NAN,      1.0f,     1.0f,
        NAN,      1.0f,     0.0f,     1.0f,
        1.0f,     0.0f,     INFINITY, 1.0f
    };
    // clang-format on

    DoFloatFormatSamplingTest(
        {wgpu::TextureFormat::RG11B10Ufloat, 4, wgpu::TextureComponentType::Float, 4}, textureData,
        uncompressedData);
    // This format is not renderable.
}

// Test the RGB9E5Ufloat format
TEST_P(TextureFormatTest, RGB9E5Ufloat) {
    // RGB9E5 is different from other floating point formats because the mantissa doesn't index in
    // the window defined by the exponent but is instead treated as a pure multiplier. There is
    // also no Infinity or NaN. The OpenGL 4.6 spec has the best explanation I've found in section
    // 8.25 "Shared Exponent Texture Color Conversion":
    //
    //   red = reduint * 2^(expuint - B - N) = reduint * 2^(expuint - 24)
    //
    // Where reduint and expuint are the integer values when considering the E5 as a 5bit uint, and
    // the r9 as a 9bit uint. B the number of bits of the mantissa (9), and N the offset for the
    // exponent (15).

    float smallestExponent = std::pow(2.0f, -24.0f);
    float largestExponent = std::pow(2.0f, float(31 - 24));

    auto MakeRGB9E5 = [](uint32_t r, uint32_t g, uint32_t b, uint32_t e) {
        ASSERT((r & 0x1FF) == r);
        ASSERT((g & 0x1FF) == g);
        ASSERT((b & 0x1FF) == b);
        ASSERT((e & 0x1F) == e);
        return r | g << 9 | b << 18 | e << 27;
    };

    // Test the smallest largest, and "1" exponents
    std::vector<uint32_t> textureData = {
        MakeRGB9E5(0, 1, 2, 0b00000),
        MakeRGB9E5(2, 1, 0, 0b11111),
        MakeRGB9E5(0, 1, 2, 0b11000),
    };

    // This is one of the only 3-channel formats, so we don't have specific testing for them. Alpha
    // should always be sampled as 1
    // clang-format off
    std::vector<float> uncompressedData = {
        0.0f, smallestExponent, 2.0f * smallestExponent, 1.0f,
        2.0f * largestExponent, largestExponent, 0.0f, 1.0f,
        0.0f, 1.0f, 2.0f, 1.0f,
    };
    // clang-format on

    DoFloatFormatSamplingTest(
        {wgpu::TextureFormat::RGB9E5Ufloat, 4, wgpu::TextureComponentType::Float, 4}, textureData,
        uncompressedData);
    // This format is not renderable.
}

// TODO(cwallez@chromium.org): Add tests for depth-stencil formats when we know if they are copyable
// in WebGPU.

DAWN_INSTANTIATE_TEST(TextureFormatTest,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());
