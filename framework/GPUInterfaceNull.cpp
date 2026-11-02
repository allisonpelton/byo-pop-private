// Sexy::GPUInterface's null implementation (GPUInterface.h): for headless runs. It draws nothing and answers every call
// as the GL one does, so the game takes the same branches: the scene count moves at the same calls, a render target is
// made and set where the GL one would, shaders and buffers get handles numbered the same way.
#include "GPUInterface.h"

#include <vector>

using namespace Sexy;

namespace
{

class GPUInterfaceNull : public GPUInterface
{
public:
	GPUInterfaceNull(const GPUPolicy& thePolicy) : mPolicy(thePolicy) {}

	bool IsNull() override { return true; }
	const GPUPolicy& GetPolicy() override { return mPolicy; }
	void Resize(int, int) override {}
	void ContextLost() override {}
	void ContextRestored() override {}

	bool BeginSceneIfNeeded() override
	{
		if (!mSceneAllowed)
			return false;
		if (!mInScene) {
			mInScene = true;
			mSceneCount++;
		}
		return true;
	}
	void EndScene() override { mInScene = false; }
	void SetSceneAllowed(bool theAllowed) override { mSceneAllowed = theAllowed; }
	int GetSceneCount() override { return mSceneCount; }

	bool CreateRenderTarget(Image*) override { return true; }
	bool SetRenderTarget(Image* theImage) override
	{
		if (!BeginSceneIfNeeded())
			return false;
		mTarget = theImage;
		return true;
	}
	Image* GetRenderTarget() override { return mTarget; }
	void StretchRect(Image*, const Rect&, Image*, const Rect&, GPUTextureFilter) override {}
	void Clear(unsigned long, unsigned long, float) override {}
	void SetViewport(const Rect&, float, float) override {}
	void SetScissorRect(const Rect&) override {}

	bool ReadBack(Image*, MemoryImage*) override { return false; }
	void BitsChanged(Image*) override {}
	void ImageDeleted(Image* theImage) override
	{
		if (mTarget == theImage)
			mTarget = nullptr;
	}

	void SetRenderState(int, unsigned long) override {}
	void SetTextureStageState(int, int, unsigned long) override {}
	void SetSamplerState(int, int, unsigned long) override {}
	void SetTexture(int, Image*) override {}
	void SetTransform(int, const GPUMatrix&) override {}
	void SetLight(int, const GPULight&) override {}
	void LightEnable(int, bool) override {}
	void SetMaterial(const GPUMaterial&) override {}

	GPUVertexShader CreateVertexShader(const std::string&, const char*) override { return ++mShaderCount; }
	GPUPixelShader CreatePixelShader(const std::string&, const char*) override { return ++mShaderCount; }
	void SetVertexShader(GPUVertexShader theShader) override { mVertexShader = theShader; }
	void SetPixelShader(GPUPixelShader theShader) override
	{
		BeginSceneIfNeeded();
		mPixelShader = theShader;
	}
	GPUVertexShader GetVertexShader() override { return mVertexShader; }
	GPUPixelShader GetPixelShader() override { return mPixelShader; }
	void SetVertexShaderConstantF(int, const float*, int) override {}
	void SetPixelShaderConstantF(int, const float*, int) override {}
	std::vector<std::string> GetUsedProgramKeys() override { return {}; }
	void Precompile(const std::vector<std::string>&) override {}

	void DrawPrimitiveUP(GPUPrimitiveType, int thePrimitiveCount, unsigned long, const void*, int) override
	{
		if (thePrimitiveCount > 0)
			BeginSceneIfNeeded();
	}
	GPUBuffer CreateVertexBuffer(unsigned long, const void*, int, int) override { return mNextBuffer++; }
	GPUBuffer CreateIndexBuffer(const unsigned short*, int) override { return mNextBuffer++; }
	void DeleteBuffer(GPUBuffer) override {}
	void DrawIndexedPrimitive(GPUPrimitiveType, GPUBuffer, GPUBuffer, int, int thePrimitiveCount) override
	{
		if (thePrimitiveCount > 0)
			BeginSceneIfNeeded();
	}

private:
	GPUPolicy mPolicy;
	bool mSceneAllowed = true;
	bool mInScene = false;
	int mSceneCount = 0;
	Image* mTarget = nullptr;
	int mShaderCount = 0;
	GPUVertexShader mVertexShader = 0;
	GPUPixelShader mPixelShader = 0;
	int mNextBuffer = 1;
};

}  // namespace

GPUInterface* GPUInterface::CreateNull(const GPUPolicy& thePolicy, int, int)
{
	return new GPUInterfaceNull(thePolicy);
}
