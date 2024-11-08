#include "GeometricPrimitive.h"
#include "Effects.h"
#include "DirectXHelpers.h"
#include "Model.h"
#include "CommonStates.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "SimpleMath.h"

#include "DirectXMath.h"

#include "DXUT.h"

#include <wrl.h>

#include <map>
#include <algorithm>
#include <array>
#include <memory>
#include <assert.h>
#include <malloc.h>
#include <Exception>

#include "ConstantBuffer.h"

#include "AppBuffers.h"

#include "resourceview.h"

#include "UniformBuffer.h"

using namespace DirectX;

struct EffectShaderFileDef{
	WCHAR * name;
	WCHAR * entry_point;
	WCHAR * shader_ver;
};
class IPostProcess
{
public:
	virtual ~IPostProcess() { }

	virtual void __cdecl Process(_In_ ID3D11DeviceContext* deviceContext, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) = 0;
};

inline ID3D11RenderTargetView** renderTargetViewToArray(ID3D11RenderTargetView* rtv1, ID3D11RenderTargetView* rtv2 = 0, ID3D11RenderTargetView* rtv3 = 0){
	static ID3D11RenderTargetView* rtvs[10];
	rtvs[0] = rtv1;
	rtvs[1] = rtv2;
	rtvs[2] = rtv3;
	return rtvs;
};
inline ID3D11ShaderResourceView** shaderResourceViewToArray(ID3D11ShaderResourceView* rtv1, ID3D11ShaderResourceView* rtv2 = 0, ID3D11ShaderResourceView* rtv3 = 0, ID3D11ShaderResourceView* rtv4 = 0, ID3D11ShaderResourceView* rtv5 = 0){
	static ID3D11ShaderResourceView* srvs[10];
	srvs[0] = rtv1;
	srvs[1] = rtv2;
	srvs[2] = rtv3;
	srvs[3] = rtv4;
	srvs[4] = rtv5;
	return srvs;
};
inline ID3D11Buffer** constantBuffersToArray(ID3D11Buffer* c1, ID3D11Buffer* c2){
	static ID3D11Buffer* cbs[10];
	cbs[0] = c1;
	cbs[1] = c2;
	return cbs;
};
inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<SceneState> &cb){
	static ID3D11Buffer* cbs[10];
	cbs[0] = cb.GetBuffer();
	return cbs;
};
inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<BlurHandling> &cb){
	static ID3D11Buffer* cbs[10];
	cbs[0] = cb.GetBuffer();
	return cbs;
};
inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<SceneState> &c1, DirectX::ConstantBuffer<BlurHandling> &c2){
	static ID3D11Buffer* cbs[10];
	cbs[0] = c1.GetBuffer();
	cbs[1] = c2.GetBuffer();
	return cbs;
};
inline ID3D11SamplerState** samplerStateToArray(ID3D11SamplerState* ss1, ID3D11SamplerState* ss2 = 0){
	static ID3D11SamplerState* sss[10];
	sss[0] = ss1;
	sss[1] = ss2;
	return sss;
};

namespace Camera{
	void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
}
std::unique_ptr<DirectX::IEffect> createHlslEffect(ID3D11Device* device, std::map<const WCHAR*, EffectShaderFileDef>& fileDef);

bool LoadModel(char* filename, std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices);
DirectX::ModelMeshPart* CreateModelMeshPart(ID3D11Device* device, std::function<void(std::vector<VertexPositionNormalTexture> & _vertices, std::vector<unsigned int> & _indices)> createGeometry);

void set_scene_world_matrix(DirectX::XMFLOAT4X4 transformation);
void scene_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void(ID3D11ShaderResourceView * texture, DirectX::XMFLOAT4X4 transformation)> setCustomState = nullptr);

void post_proccess(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState);

struct SceneNode{
	DirectX::XMFLOAT4X4 transformation;
	std::vector<DirectX::ModelMeshPart*> mesh;
	std::vector<SceneNode*> children;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> > texture;
	~SceneNode();
	void draw(_In_ ID3D11DeviceContext* deviceContext, _In_ IEffect* ieffect, _In_ ID3D11InputLayout* iinputLayout,
		_In_opt_ std::function<void(ID3D11ShaderResourceView * texture, DirectX::XMFLOAT4X4 transformation)> setCustomState);
};

struct BlurParams
{
	int Radius;
	int WeightLength;
	float Weights[64 * 2 + 1];

	BlurParams(int radius = 0)
	{
		WeightLength = (Radius = radius) * 2 + 1;
	}
};
BlurParams GaussianBlur(int radius);

class GraphicResources {
public:
	std::unique_ptr<CommonStates> render_states;

	std::unique_ptr<DirectX::IEffect> quad_effect;

	std::unique_ptr<DirectX::IEffect> model_effect;

	std::unique_ptr<DirectX::IEffect> blur_h_effect;

	std::unique_ptr<DirectX::IEffect> blur_v_effect;

	std::unique_ptr<DirectX::ConstantBuffer<SceneState> > scene_constant_buffer;
	std::unique_ptr<DirectX::ConstantBuffer<BlurHandling> > blur_constant_buffer;

	std::unique_ptr<SceneNode> scene;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> scene_node_input_layout;

	std::unique_ptr<framework::UniformBuffer> light_buffer;

	std::unique_ptr<framework::UniformBuffer> samples_kernel;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> randomNormalT;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> randomNormalSRV;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> RSState;
};

class SwapChainGraphicResources {
public:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> depthStencilSRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilT;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalSRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> normalV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> normalT;

	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> > RenderTargetSRV;
	std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView> > RenderTargetV;
	std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D> > RenderTargetT;

};