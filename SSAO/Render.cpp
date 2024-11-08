#include "main.h"

#include "DXUTgui.h"
#include "SDKmisc.h"

extern GraphicResources * G;

extern SwapChainGraphicResources * SCG;

extern SceneState scene_state;

extern BlurHandling blur_handling;

extern CDXUTTextHelper*                    g_pTxtHelper;

ID3D11ShaderResourceView* null[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

inline void set_scene_constant_buffer(ID3D11DeviceContext* context){
	G->scene_constant_buffer->SetData(context, scene_state);
};

inline void set_blur_constant_buffer(ID3D11DeviceContext* context){
	G->blur_constant_buffer->SetData(context, blur_handling);
};

void RenderText()
{
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos(2, 0);
	g_pTxtHelper->SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	g_pTxtHelper->DrawTextLine(DXUTGetFrameStats(true && DXUTIsVsyncEnabled()));
	g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());

	g_pTxtHelper->End();
}

void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	Camera::OnFrameMove(fTime, fElapsedTime, pUserContext);
}

void renderSceneIntoGBuffer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext);
void postProccessGBuffer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context);
void postProccessBlur(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context, _In_opt_ std::function<void __cdecl()> setHState, _In_opt_ std::function<void __cdecl()> setVState);

void clearAndSetRenderTarget(ID3D11DeviceContext* context, float ClearColor[], int n, ID3D11RenderTargetView** pRTV, ID3D11DepthStencilView* pDSV){
	for (int i = 0; i < n; i++)
	context->ClearRenderTargetView(pRTV[i], ClearColor);

	context->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	context->OMSetRenderTargets(n, pRTV, pDSV); //renderTargetViewToArray(pRTV) DXUTGetD3D11RenderTargetView
}

void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context,
	double fTime, float fElapsedTime, void* pUserContext)
{
	D3D11_VIEWPORT vp; 
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = scene_state.vFrustumParams.x;
	vp.Height = scene_state.vFrustumParams.y;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	context->RSSetViewports(1, &vp);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	{
		context->PSSetShaderResources(0, 5, null);
	}
	{
		clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(SCG->normalV.Get()), SCG->depthStencilV.Get());

		renderSceneIntoGBuffer(pd3dDevice, context);
	}
	{
		context->PSSetShaderResources(0, 5, null);
	}
	ID3D11RenderTargetView* rt;
	{
		rt = DXUTGetD3D11RenderTargetView();
		rt = SCG->RenderTargetV[0].Get();

		clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(rt), DXUTGetD3D11DepthStencilView());

		postProccessGBuffer(pd3dDevice, context);
	}
	if (rt == SCG->RenderTargetV[0].Get()){
		postProccessBlur(pd3dDevice, context, 
			[=]{
				float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
				clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(SCG->RenderTargetV[1].Get()), DXUTGetD3D11DepthStencilView());
				context->PSSetShaderResources(2, 1, shaderResourceViewToArray(SCG->RenderTargetSRV[0].Get()));
			},
			[=]{
				float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
				clearAndSetRenderTarget(context, clearColor, 1, renderTargetViewToArray(DXUTGetD3D11RenderTargetView()), DXUTGetD3D11DepthStencilView());
				context->PSSetShaderResources(2, 1, shaderResourceViewToArray(SCG->RenderTargetSRV[1].Get()));
			}
		);
	}
	RenderText();
}


void postProccessGBuffer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context){
	/////
	context->PSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));

	context->PSSetSamplers(0, 1, samplerStateToArray(G->render_states->PointWrap()));
	/////

	set_scene_world_matrix(SimpleMath::Matrix::Identity);

	set_scene_constant_buffer(context);

	post_proccess(context, G->quad_effect.get(), 0, [=]{
		context->PSSetShaderResources(1, 4, shaderResourceViewToArray(
			G->samples_kernel->getView().asShaderView(),
			SCG->normalSRV.Get(), 
			SCG->depthStencilSRV.Get(), 
			G->randomNormalSRV.Get())
		);

		context->OMSetBlendState(G->render_states->Opaque(), Colors::White, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullNone());
		context->OMSetDepthStencilState(G->render_states->DepthNone(), 0);
	});
}

void postProccessBlur(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context, _In_opt_ std::function<void __cdecl()> setHState, _In_opt_ std::function<void __cdecl()> setVState){
	/////
	context->PSSetConstantBuffers(0, 2, constantBuffersToArray(*(G->scene_constant_buffer), *(G->blur_constant_buffer)));

	context->PSSetShaderResources(3, 2, shaderResourceViewToArray(SCG->normalSRV.Get(), SCG->depthStencilSRV.Get()));
	/////

	set_scene_world_matrix(SimpleMath::Matrix::Identity);

	set_scene_constant_buffer(context);

	setHState();

	post_proccess(context, G->blur_h_effect.get(), 0, [=]{
		context->OMSetBlendState(G->render_states->Opaque(), Colors::White, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullNone());
		context->OMSetDepthStencilState(G->render_states->DepthNone(), 0);
	});

	setVState();

	post_proccess(context, G->blur_v_effect.get(), 0, [=]{
		context->OMSetBlendState(G->render_states->Opaque(), Colors::White, 0xFFFFFFFF);
		context->RSSetState(G->render_states->CullNone());
		context->OMSetDepthStencilState(G->render_states->DepthNone(), 0);
	});

	context->PSSetConstantBuffers(0, 2, constantBuffersToArray(0, 0));
}

void renderSceneIntoGBuffer(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context)
{
	/////
	context->VSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));
	/////

	if (true){
		scene_draw(context, G->model_effect.get(), G->scene_node_input_layout.Get(), [=](ID3D11ShaderResourceView * texture, DirectX::XMFLOAT4X4 transformation){
			set_scene_world_matrix(SimpleMath::Matrix::Identity);// transformation*SimpleMath::Matrix::CreateScale(100, 100, 100));

			set_scene_constant_buffer(context);

			context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
			context->RSSetState(G->RSState.Get());
			context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);
		});
	}
}