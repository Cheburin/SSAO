#include "main.h"

#include "DXUTgui.h"
#include "SDKmisc.h"

HWND DXUTgetWindow();

GraphicResources * G;

SceneState scene_state;

BlurHandling blur_handling;

BlurParams blurParams;

std::unique_ptr<Keyboard> _keyboard;
std::unique_ptr<Mouse> _mouse;

CDXUTDialogResourceManager          g_DialogResourceManager;
CDXUTTextHelper*                    g_pTxtHelper = NULL;

#include <codecvt>
std::unique_ptr<SceneNode> loadSponza(ID3D11Device* device, ID3D11InputLayout** l, DirectX::IEffect *e );

inline float lerp(float x1, float x2, float t){
	return x1*(1.0 - t) + x2*t;
}

inline float nextFloat(float x1, float x2){
	return lerp(x1, x2, (float)std::rand() / (float)RAND_MAX);
}

HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* device, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	std::srand(unsigned(std::time(0)));

	HRESULT hr;

	ID3D11DeviceContext* context = DXUTGetD3D11DeviceContext();

	G = new GraphicResources();
	G->render_states = std::make_unique<CommonStates>(device);
	G->scene_constant_buffer = std::make_unique<ConstantBuffer<SceneState> >(device);
	G->blur_constant_buffer = std::make_unique<ConstantBuffer<BlurHandling> >(device); 

	_keyboard = std::make_unique<Keyboard>();
	_mouse = std::make_unique<Mouse>();
	HWND hwnd = DXUTgetWindow();
	_mouse->SetWindow(hwnd);

	g_DialogResourceManager.OnD3D11CreateDevice(device, context);
	g_pTxtHelper = new CDXUTTextHelper(device, context, &g_DialogResourceManager, 15);

	//effects
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"Quad.hlsl", L"VS", L"vs_5_0" };
		shaderDef[L"GS"] = { L"Quad.hlsl", L"GS", L"gs_5_0" };
		shaderDef[L"PS"] = { L"Quad.hlsl", L"PS", L"ps_5_0" };

		G->quad_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"Model.hlsl", L"MODEL_VERTEX", L"vs_5_0" };
		shaderDef[L"PS"] = { L"Model.hlsl", L"MODEL_FRAG", L"ps_5_0" };

		G->model_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"blur.hlsl", L"VS", L"vs_5_0" };
		shaderDef[L"GS"] = { L"blur.hlsl", L"GS", L"gs_5_0" };
		shaderDef[L"PS"] = { L"blur.hlsl", L"HB", L"ps_5_0" };

		G->blur_h_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"blur.hlsl", L"VS", L"vs_5_0" };
		shaderDef[L"GS"] = { L"blur.hlsl", L"GS", L"gs_5_0" };
		shaderDef[L"PS"] = { L"blur.hlsl", L"VB", L"ps_5_0" };

		G->blur_v_effect = createHlslEffect(device, shaderDef);
	}

	//models
	{
		G->scene = loadSponza(device, G->scene_node_input_layout.ReleaseAndGetAddressOf(), G->model_effect.get());
	}

	//light buffer
	{
		G->light_buffer = std::make_unique<framework::UniformBuffer>();

		if (!G->light_buffer->initDefaultStructured<LightData>(device, (size_t)1))
			return S_FALSE;

		LightData lightData;
		lightData.ambientColor = SimpleMath::Vector3(0.15f, 0.15f, 0.15f);
		lightData.diffuseColor = SimpleMath::Vector3(1.0f, 1.0f, 1.0f);
		lightData.direction = SimpleMath::Vector3(0.0f, -1.0f, 0.5f);

		G->light_buffer->setElement(0, lightData);

		G->light_buffer->applyChanges(context);
	}

	//samples_kernel buffer
	{
		const int samplesKernelLength = 16;
		G->samples_kernel = std::make_unique<framework::UniformBuffer>();
		if (!G->samples_kernel->initDefaultStructured<DirectX::XMFLOAT3>(device, (size_t)samplesKernelLength))
			return S_FALSE;
		for (int i = 0; i < samplesKernelLength; i++)
		{
			SimpleMath::Vector3 samplesKernelItem;
			samplesKernelItem.x = nextFloat(-0.999999, 0.999999);
			samplesKernelItem.z = nextFloat(-0.999999, 0.999999);
			samplesKernelItem.y = nextFloat(0, 1);

			samplesKernelItem.Normalize();

			float scale = (float)i / (float)samplesKernelLength;

			scale = lerp(0.1f, 1.0f, scale * scale);

			samplesKernelItem *= scale;

			G->samples_kernel->setElement(i, DirectX::XMFLOAT3(samplesKernelItem));
		}
		G->samples_kernel->applyChanges(context);
	}

	//random_normal texture
	{
		const int size = 4;
		DirectX::XMFLOAT4 randomNormalTextureData[size * size];
		for (int i = 0; i < size * size; i++)
		{
			SimpleMath::Vector3 randomNormal;
			randomNormal.x = nextFloat(0, 0.999999);
			randomNormal.y = 1;
			randomNormal.z = nextFloat(0, 0.999999);

			randomNormal.Normalize();

			randomNormalTextureData[i] = SimpleMath::Vector4(randomNormal.x, randomNormal.y, randomNormal.z, 1);
		}
		D3D11_TEXTURE2D_DESC textureDesc;
		textureDesc.Width = size;
		textureDesc.Height = size;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initialData;
		initialData.pSysMem = &(randomNormalTextureData[0]);
		initialData.SysMemPitch = sizeof(DirectX::XMFLOAT4)*size;
		initialData.SysMemSlicePitch = 0;
		hr = device->CreateTexture2D(&textureDesc, &initialData, G->randomNormalT.ReleaseAndGetAddressOf());

		hr = device->CreateShaderResourceView(G->randomNormalT.Get(), nullptr, G->randomNormalSRV.ReleaseAndGetAddressOf());

	}

	//blur paarams
	{
		blurParams = GaussianBlur(4);

		for (int i = 0; i < blurParams.WeightLength; i++)
			blur_handling.Weights[i] = SimpleMath::Vector4(blurParams.Weights[i], 0, 0, 0);

		blur_handling.Radius = 4;
		blur_handling.DepthAnalysis = 1;
		blur_handling.NormalAnalysis = 1;
		blur_handling.DepthAnalysisFactor = 1;

		G->blur_constant_buffer->SetData(context, blur_handling);
	}

	{
		D3D11_RASTERIZER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		desc.CullMode = D3D11_CULL_NONE;
		desc.FillMode = D3D11_FILL_SOLID;
		desc.DepthClipEnable = true;
		desc.MultisampleEnable = false;

		HRESULT hr = device->CreateRasterizerState(&desc, G->RSState.ReleaseAndGetAddressOf());
	}

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	delete g_pTxtHelper;

	g_DialogResourceManager.OnD3D11DestroyDevice();

	_mouse = 0;

	_keyboard = 0;

	delete G;
}
