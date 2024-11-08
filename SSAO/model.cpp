#include "main.h"
#include <fstream>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <locale>
#include <codecvt>
#include <string>
#include <array>
#include <locale> 

bool LoadModel(char* filename, std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices)
{
	int m_vertexCount, m_indexCount;

	std::ifstream fin;
	char input;
	int i;


	// Open the model file.  If it could not open the file then exit.
	fin.open(filename);
	if (fin.fail())
	{
		return false;
	}

	// Read up to the value of vertex count.
	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}

	// Read in the vertex count.
	fin >> m_vertexCount;

	// Set the number of indices to be the same as the vertex count.
	m_indexCount = m_vertexCount;

	// Read up to the beginning of the data.
	fin.get(input);
	while (input != ':')
	{
		fin.get(input);
	}
	fin.get(input);
	fin.get(input);

	// Read in the vertex data.
	for (i = 0; i<m_vertexCount; i++)
	{
		VertexPositionNormalTexture v;
		fin >> v.position.x >> v.position.y >> v.position.z;
		fin >> v.textureCoordinate.x >> v.textureCoordinate.y;
		fin >> v.normal.x >> v.normal.y >> v.normal.z;

		_vertices.push_back(v);

		_indices.push_back(i);
	}

	// Close the model file.
	fin.close();

	return true;
}


template<typename T>
void CreateBuffer(_In_ ID3D11Device* device, T const& data, D3D11_BIND_FLAG bindFlags, _Outptr_ ID3D11Buffer** pBuffer)
{
	assert(pBuffer != 0);

	D3D11_BUFFER_DESC bufferDesc = { 0 };

	bufferDesc.ByteWidth = (UINT)data.size() * sizeof(T::value_type);
	bufferDesc.BindFlags = bindFlags;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA dataDesc = { 0 };

	dataDesc.pSysMem = data.data();

	device->CreateBuffer(&bufferDesc, &dataDesc, pBuffer);

	//SetDebugObjectName(*pBuffer, "DirectXTK:GeometricPrimitive");
}

DirectX::ModelMeshPart* CreateModelMeshPart(ID3D11Device* device, std::function<void(std::vector<VertexPositionNormalTexture> & _vertices, std::vector<unsigned int> & _indices)> createGeometry){
	std::vector<VertexPositionNormalTexture> vertices;
	std::vector<unsigned int> indices;

	createGeometry(vertices, indices);

	size_t nVerts = vertices.size();

	DirectX::ModelMeshPart* modelMeshPArt = new DirectX::ModelMeshPart();

	modelMeshPArt->indexCount = indices.size();
	modelMeshPArt->startIndex = 0;
	modelMeshPArt->vertexOffset = 0;
	modelMeshPArt->vertexStride = sizeof(VertexPositionNormalTexture);
	modelMeshPArt->primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	modelMeshPArt->indexFormat = DXGI_FORMAT_R32_UINT;
	modelMeshPArt->vbDecl = std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>(
		new std::vector<D3D11_INPUT_ELEMENT_DESC>(
		&VertexPositionNormalTexture::InputElements[0],
		&VertexPositionNormalTexture::InputElements[VertexPositionNormalTexture::InputElementCount]
		)
		);

	CreateBuffer(device, vertices, D3D11_BIND_VERTEX_BUFFER, modelMeshPArt->vertexBuffer.ReleaseAndGetAddressOf());

	CreateBuffer(device, indices, D3D11_BIND_INDEX_BUFFER, modelMeshPArt->indexBuffer.ReleaseAndGetAddressOf());

	return modelMeshPArt;
}

SceneNode::~SceneNode(){
	for (int i = 0; i < children.size(); i++)
		delete children[i];
	for (int i = 0; i < mesh.size(); i++)
		delete mesh[i];
}

////
void modelMeshPartDraw(ID3D11DeviceContext* deviceContext, ModelMeshPart* mmp, ID3D11ShaderResourceView* texture, DirectX::XMFLOAT4X4 transformation, IEffect* ieffect, ID3D11InputLayout* iinputLayout, std::function<void(ID3D11ShaderResourceView * texture, DirectX::XMFLOAT4X4 transformation)> setCustomState)
{
	deviceContext->IASetInputLayout(iinputLayout);

	auto vb = mmp->vertexBuffer.Get();
	UINT vbStride = mmp->vertexStride;
	UINT vbOffset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &vb, &vbStride, &vbOffset);

	// Note that if indexFormat is DXGI_FORMAT_R32_UINT, this model mesh part requires a Feature Level 9.2 or greater device
	deviceContext->IASetIndexBuffer(mmp->indexBuffer.Get(), mmp->indexFormat, 0);

	assert(ieffect != 0);
	ieffect->Apply(deviceContext);

	// Hook lets the caller replace our shaders or state settings with whatever else they see fit.
	setCustomState(texture, transformation);

	// Draw the primitive.
	deviceContext->IASetPrimitiveTopology(mmp->primitiveType);

	deviceContext->DrawIndexed(mmp->indexCount, mmp->startIndex, mmp->vertexOffset);
}
////

void SceneNode::draw(_In_ ID3D11DeviceContext* deviceContext, _In_ IEffect* ieffect, _In_ ID3D11InputLayout* iinputLayout,
	_In_opt_ std::function<void(ID3D11ShaderResourceView * texture, DirectX::XMFLOAT4X4 transformation)> setCustomState){
	for (int i = 0; i < children.size(); i++)
		children[i]->draw(deviceContext, ieffect, iinputLayout, setCustomState);
	for (int i = 0; i < mesh.size(); i++)
		//mesh[i]->Draw(deviceContext, ieffect, iinputLayout, setCustomState);
		modelMeshPartDraw(deviceContext, mesh[i], texture[i].Get(), transformation, ieffect, iinputLayout, setCustomState);
}

XMFLOAT4X4& assign(XMFLOAT4X4& output, const aiMatrix4x4& aiMe){
	output._11 = aiMe.a1;
	output._12 = aiMe.a2;
	output._13 = aiMe.a3;
	output._14 = aiMe.a4;

	output._21 = aiMe.b1;
	output._22 = aiMe.b2;
	output._23 = aiMe.b3;
	output._24 = aiMe.b4;

	output._31 = aiMe.c1;
	output._32 = aiMe.c2;
	output._33 = aiMe.c3;
	output._34 = aiMe.c4;

	output._41 = aiMe.d1;
	output._42 = aiMe.d2;
	output._43 = aiMe.d3;
	output._44 = aiMe.d4;

	return output;
}

void collectMeshes(ID3D11Device* device, const aiScene* scene, aiNode * node, SceneNode * sceneNode, DirectX::XMFLOAT4X4 parentTransformation){
	DirectX::XMFLOAT4X4 transformation = assign(XMFLOAT4X4(), node->mTransformation);

	sceneNode->transformation = SimpleMath::Matrix(transformation) * parentTransformation;

	for (int i = 0; i < node->mNumMeshes; i++){
		auto nodeMesh = scene->mMeshes[node->mMeshes[i]];

		auto nodeMaterial = scene->mMaterials[nodeMesh->mMaterialIndex];

		if (nodeMesh->mPrimitiveTypes != 4) throw "";

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneNodeTexture;

		//Material
		if (nodeMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0){
			aiString aiPath;

			if (nodeMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &aiPath, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
				auto path = std::string(aiPath.data);
				
				std::string fullPath = std::string("models\\") + path.replace(path.find("/"), 1, "\\");

				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> string_convertor;

				HRESULT hr = D3DX11CreateShaderResourceViewFromFile(device, string_convertor.from_bytes(fullPath.data()).data(), NULL, NULL, sceneNodeTexture.ReleaseAndGetAddressOf(), NULL);

				if (hr != S_OK)  throw "";
			}

		}

		//Mesh
		auto sceneNodeMesh = CreateModelMeshPart(device, [=](std::vector<VertexPositionNormalTexture> & _vertices, std::vector<unsigned int> & _indices){
			for (int i = 0; i < nodeMesh->mNumVertices; i++){
				VertexPositionNormalTexture v;
				v.position.x = nodeMesh->mVertices[i].x;
				v.position.y = nodeMesh->mVertices[i].y;
				v.position.z = nodeMesh->mVertices[i].z;

				v.normal.x = nodeMesh->mNormals[i].x;
				v.normal.y = nodeMesh->mNormals[i].y;
				v.normal.z = nodeMesh->mNormals[i].z;

				if (nodeMesh->mNumUVComponents[1] != 0) throw "";
				if (nodeMesh->mNumUVComponents[0] != 2) throw "";

				v.textureCoordinate.x = nodeMesh->mTextureCoords[0][i].x;
				v.textureCoordinate.y = nodeMesh->mTextureCoords[0][i].y;

				_vertices.push_back(v);
			}
			for (int i = 0; i < nodeMesh->mNumFaces; i++){
				if (nodeMesh->mFaces[i].mNumIndices != 3) throw "";
				for (int j = 0; j < 3; j++){
					_indices.push_back(nodeMesh->mFaces[i].mIndices[j]);
				};
			}
		});

		sceneNode->texture.push_back(sceneNodeTexture);

		sceneNode->mesh.push_back(sceneNodeMesh);
	}

	for (int i = 0; i < node->mNumChildren; i++){
		auto childrenSceneNode = new SceneNode();

		sceneNode->children.push_back(childrenSceneNode);

		collectMeshes(device, scene, node->mChildren[i], childrenSceneNode, sceneNode->transformation);
	}
}
///collada load
std::unique_ptr<SceneNode> loadSponza(ID3D11Device* device, ID3D11InputLayout** l, DirectX::IEffect *e){
	// тест настроек конвеера и шейдинга с пост обработкой
	if (false){
		auto mesh = CreateModelMeshPart(device, [=](std::vector<VertexPositionNormalTexture> & _vertices, std::vector<unsigned int> & _indices){
			VertexPositionNormalTexture v;
			v.position.x = -0.5;
			v.position.y = -0.5;
			v.position.z = 0.201218;
			v.normal.x = 0.866025403;
			v.normal.y = 0.5;
			v.normal.z = 0;
			_vertices.push_back(v);
			v.position.x = -0.5;
			v.position.y = 0.5;
			v.position.z = 0.548045;
			v.normal.x = 0.707106781;
			v.normal.y = 0.707106781;
			v.normal.z = 0;
			_vertices.push_back(v);
			v.position.x = 0.5;
			v.position.y = -0.5;
			v.position.z = 0.892150;
			v.normal.x = 0;
			v.normal.y = 1;
			v.normal.z = 0;
			_vertices.push_back(v);
			_indices.push_back(0);
			_indices.push_back(1);
			_indices.push_back(2);
		});

		//Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneNodeTexture;

		//D3DX11CreateShaderResourceViewFromFile(device, L"models\\tex\\Celing.bmp", NULL, NULL, sceneNodeTexture.ReleaseAndGetAddressOf(), NULL);

		std::unique_ptr<SceneNode> sceneNode(new SceneNode());

		sceneNode.get()->mesh.push_back(mesh);
		sceneNode.get()->texture.push_back(0);

		//input layouts
		mesh->CreateInputLayout(device, e, l);

		return sceneNode;
	}
	// Create an instance of the Importer class
	Assimp::Importer importer;

	// And have it read the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll 
	// propably to request more postprocessing than we do in this example.
	const aiScene* scene = importer.ReadFile("models\\Sponza.dae", aiProcess_Triangulate);

	auto node = scene->mRootNode;
	
	std::unique_ptr<SceneNode> sceneNode(new SceneNode());

	collectMeshes(device, scene, node, sceneNode.get(), SimpleMath::Matrix::Identity);
	
	//input layouts
	sceneNode->children[0]->mesh[0]->CreateInputLayout(device, e, l);

	return sceneNode;
}
