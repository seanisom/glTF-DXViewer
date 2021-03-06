// GLBGLTFConverter.cpp : Defines the entry point for the console application.
//
#pragma once

#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "stdafx.h"

#include <codecvt>

#include "GLBGLTFConverter.h"
#include "GLTFSDK/ExtensionHandlers.h"
#include <GLTFSDK/RapidJsonUtils.h>
#include <strsafe.h>

#include <iostream>
#include <fstream>
#include <filesystem>

using namespace GLTFParser;
//using namespace std::experimental::filesystem::v1;
using namespace std::filesystem;

class GLBStreamReader : public IStreamReader
{
public:
	GLBStreamReader(shared_ptr<istream> wrapped, const string& baseUri) : m_stream(wrapped)
	{
		path pth(baseUri);
		auto buri = pth.remove_filename().c_str();
		auto base = wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(buri);
		auto sep = wstring_convert<codecvt_utf8<wchar_t>>().to_bytes(path::preferred_separator);
		//_baseUri = base + sep;
		_baseUri = base;
	}
	/*
	shared_ptr<istream> GetInputStream(const string&) const override
	{
		// Here we need to translate the string passed into a stream and that is where 
		// we can't progress as we would need arbitrary access to the file system...

		return m_stream;
	}
	*/
	shared_ptr<istream> GetInputStream(const string& uri) const override
	{
		auto path = _baseUri + uri;
		auto file = make_shared<ifstream>(path);
		if (file->fail())
		{
			return m_stream;
		}
		return file;
	}

private:
	shared_ptr<istream> m_stream;
	string _baseUri;
};

const wchar_t *newline = L"\n";

void Out(LPCTSTR sFormat, ...)
{
#ifndef NO_OUTPUT
	va_list argptr;
	va_start(argptr, sFormat);
	wchar_t buffer[2000];
	HRESULT hr = StringCbVPrintf(buffer, sizeof(buffer), sFormat, argptr);
	if (STRSAFE_E_INSUFFICIENT_BUFFER == hr || S_OK == hr)
	{
		wcsncat_s(buffer, newline, (rsize_t)sizeof(buffer));
		OutputDebugString(buffer);
	}
	else
		OutputDebugString(L"StringCbVPrintf error.");
#endif
}

using namespace GLTFParser;
using namespace std;

struct Component
{
	const char *Type;
	int NumBytes;
};

Component NumComponentsByType[] =
{
	{ "SCALAR", 1 },
	{ "VEC2", 2 },
	{ "VEC3", 3 },
	{ "VEC4", 4 },
	{ "MAT2", 4 },
	{ "MAT3", 9 },
	{ "MAT4", 16 }
};

constexpr const char TestExtensionName[] = "MSFT_texture_dds";

struct TestExtension : Extension
{
	TestExtension(int flag) : source(flag) {}

	std::unique_ptr<Extension> Clone() const override
	{
		return std::make_unique<TestExtension>(*this);
	}

	bool IsEqual(const Extension& rhs) const override
	{
		bool isEqual = false;

		if (auto other = dynamic_cast<const TestExtension*>(&rhs))
		{
			isEqual = other->source == source;
		}

		return isEqual;
	}

	int source;
};

std::unique_ptr<Extension> DeserializeDDSExtension(const std::string& json)
{
	rapidjson::Document documentExtension = RapidJsonUtils::CreateDocumentFromString(json);

	return std::make_unique<TestExtension>(documentExtension["source"].GetInt());
}

void GLTFFileData::Read(shared_ptr<istream> file, const string& baseUri, IStreamReader& gltfStreamReader)
{
	//shared_ptr<IStreamReader> streamReader;
	shared_ptr<const IStreamReader> streamReader;
	string jsonStr;
	unique_ptr<GLTFResourceReader> pReader;
	shared_ptr<const IStreamReader> stream_ptr;
	try
	{
		//streamReader = std::make_shared<GLBStreamReader>(file, baseUri);
		//streamReader = std::make_shared<GLTFStreamReader>(file, baseUri);
		streamReader = shared_ptr<const IStreamReader>(&gltfStreamReader, [](IStreamReader*) {});
		auto reader = make_unique<GLBResourceReader>(streamReader, file);
		jsonStr = reader->GetJson();
		pReader.reset(reader.release());
	}
	catch (GLTFException ex)
	{
		file->seekg(0);
		auto it = istreambuf_iterator<char>(*(file.get()));
		jsonStr = string(it, {});
		stream_ptr = shared_ptr<const IStreamReader>(&gltfStreamReader, [](IStreamReader*) {});
		auto reader = make_unique<GLTFResourceReader>(stream_ptr);
		pReader.reset(reader.release());
	}

	ExtensionDeserializer extensionDeserializer;
	extensionDeserializer.AddHandler<TestExtension>(TestExtensionName,
		[](const std::string& json)
	{
		return DeserializeDDSExtension(json);
	});
	
	auto doc = Deserialize(jsonStr, extensionDeserializer);
	auto& cb = EventHandlers();
	ParserContext context(doc, cb, *pReader);

	stream_ptr.reset();
	
	CheckExtensions(doc);
	ParseDocument(context);
}

int ComponentTypeToNumBytes(int compType)
{
	switch (compType)
	{
	case 5120:
	case 5121:
		return 1;
	case 5122:
	case 5123:
		return 2;
	case 5125:
	case 5126:
		return 4;
	}
	return 4;
}

void GLTFFileData::LoadScene(const ParserContext& parser, const Scene& scene)
{
	for (auto node : scene.nodes)
	{
		auto sceneNode = parser.document().nodes[node];
		string name = sceneNode.name.length() > 0 ? sceneNode.name : "node-" + sceneNode.id;
		Out(L"Loading %S", name.c_str());

		// This call will recurse down the scene hierarchy and load the whole tree..
		LoadSceneNode(parser, sceneNode, atoi(sceneNode.id.c_str()), -1);
	}
}

void GLTFFileData::LoadSceneNode(const ParserContext& parser, const Node& sceneNode, 
	int nodeIndex, int parentIndex)
{
	SceneNodeData data;
	data.nodeIndex = nodeIndex;
	data.parentIndex = parentIndex;

	string name = sceneNode.name.length() > 0 ? sceneNode.name : "node-" + sceneNode.id;
	data.Name = name.c_str();

	if (sceneNode.meshId.length() > 0)
	{
		data.isMesh = true;
		parser.callbacks().SceneNode(data);

		// Can it have children too?
		LoadMeshNode(parser, sceneNode);
	}
	else if (sceneNode.children.size() > 0)
	{
		data.isMesh = false;
		parser.callbacks().SceneNode(data);

		LoadTransform(parser, sceneNode);

		for (auto child : sceneNode.children)
		{
			auto childNode = parser.document().nodes[child];
			LoadSceneNode(parser, childNode, atoi(childNode.id.c_str()), nodeIndex);
		}
	}
}

void GLTFFileData::LoadMaterialNode(const ParserContext& parser, const Material& mNode)
{
	MaterialData data;
	string matName = mNode.name.length() > 0 ? mNode.name : "unnamed-material" + mNode.id;
	data.MaterialName = matName.c_str();

	data.emmissiveFactor[0] = mNode.emissiveFactor.r;
	data.emmissiveFactor[1] = mNode.emissiveFactor.g;
	data.emmissiveFactor[2] = mNode.emissiveFactor.b;
	data.Emissivetexture = atoi(mNode.emissiveTexture.textureId.c_str());
	data.Occlusiontexture = atoi(mNode.occlusionTexture.textureId.c_str());
	data.Normaltexture = atoi(mNode.normalTexture.textureId.c_str());

	data.Pbrmetallicroughness_Basecolortexture = atoi(mNode.metallicRoughness.baseColorTexture.textureId.c_str());
	data.Pbrmetallicroughness_Metallicroughnesstexture = atoi(mNode.metallicRoughness.metallicRoughnessTexture.textureId.c_str());

	data.baseColourFactor[0] = mNode.metallicRoughness.baseColorFactor.r;
	data.baseColourFactor[1] = mNode.metallicRoughness.baseColorFactor.g;
	data.baseColourFactor[2] = mNode.metallicRoughness.baseColorFactor.b;
	data.baseColourFactor[3] = mNode.metallicRoughness.baseColorFactor.a;

	data.metallicFactor = mNode.metallicRoughness.metallicFactor;
	data.roughnessFactor = mNode.metallicRoughness.roughnessFactor;

	if (mNode.extensions.find("ASOBO_material_invisible") != mNode.extensions.end())
	{
		//rapidjson::Document documentExtension = RapidJsonUtils::CreateDocumentFromString(mNode.extras);
		//if (documentExtension.HasMember(""))
		//{
			data.invisible = true;
		//}
	}

	parser.callbacks().Materials(data);

	LoadMaterialTextures(parser, mNode);
}

void GLTFFileData::LoadTexture(const ParserContext& parser, const Texture& texture, GLTFTextureType type)
{
	auto& exts = texture.extensions;
	auto source = texture.imageId;
	if (texture.HasExtension<TestExtension>())
	{
		source = std::to_string(texture.GetExtension<TestExtension>().source);
	}
	auto gltfDoc = parser.document();
	auto img = gltfDoc.images[source];

	try
	{
		auto imgdata = parser.resources().ReadBinaryData(gltfDoc, img);

		TextureData data;
		data.pSysMem = imgdata.data();
		data.dataSize = imgdata.size();
		data.idx = atoi(texture.id.c_str());
		data.type = (unsigned int)type;

		parser.callbacks().Textures(data);
	}
	catch (...)
	{
		return;
	}

}

void GLTFFileData::LoadMaterialTextures(const ParserContext& parser, const Material& mNode)
{
	auto gltfDoc = parser.document();

	if (!mNode.metallicRoughness.baseColorTexture.textureId.empty())
	{
		auto baseColorTexture = gltfDoc.textures[mNode.metallicRoughness.baseColorTexture.textureId];
		LoadTexture(parser, baseColorTexture, GLTFTextureType::BaseColour);
	}

	if (!mNode.metallicRoughness.metallicRoughnessTexture.textureId.empty())
	{
		auto metallicRoughnessTexture = gltfDoc.textures[mNode.metallicRoughness.metallicRoughnessTexture.textureId];
		LoadTexture(parser, metallicRoughnessTexture, GLTFTextureType::MetallicRoughness);
	}

	if (!mNode.emissiveTexture.textureId.empty())
	{
		auto emissiveTexture = gltfDoc.textures[mNode.emissiveTexture.textureId];
		LoadTexture(parser, emissiveTexture, GLTFTextureType::Emission);
	}

	if (!mNode.occlusionTexture.textureId.empty())
	{
		auto occlusionTexture = gltfDoc.textures[mNode.occlusionTexture.textureId];
		LoadTexture(parser, occlusionTexture, GLTFTextureType::Occlusion);
	}

	if (!mNode.normalTexture.textureId.empty())
	{
		auto normalTexture = gltfDoc.textures[mNode.normalTexture.textureId];
		LoadTexture(parser, normalTexture, GLTFTextureType::Normal);
	}
}

void GLTFFileData::LoadBufferFromAccessorId(const ParserContext& parser, const string& accessorId, 
											const string& bufferType, const MeshPrimitive& primitive)
{
	if (accessorId.empty() || stoi(accessorId.c_str()) < 0)
		return;
	auto indicesAcc = parser.document().accessors[accessorId];
	auto bufferView = parser.document().bufferViews[indicesAcc.bufferViewId];
	LoadBuffer(parser, bufferView, bufferType, indicesAcc, primitive);
}

void GLTFFileData::LoadBuffer(const ParserContext& parser, const BufferView& bufferView, 
							  const string& bufferType, const Accessor& accessor, const MeshPrimitive& primitive) const
{
	auto bufferData = parser.resources().ReadBinaryData<char>(parser.document(), bufferView);
	int accId = stoi(accessor.id.c_str());

	BufferData data;
	data.subresource.pSysMem = bufferData.data() + accessor.byteOffset;
	data.subresource.SysMemPitch = 0;
	data.subresource.SysMemSlicePitch = 0;
	data.subresource.accessorIdx = accId;
	//data.desc.ByteWidth = bufferView.byteLength - accessor.byteOffset;
	// Handle index buffers here too!
	data.desc.ByteWidth = accessor.count * (bufferView.byteStride == 0 ? 2 : bufferView.byteStride);
	data.desc.BufferContentType = bufferType.c_str();
	data.desc.StructureByteStride = bufferView.byteStride;
	if (primitive.extras != "")
	{
		rapidjson::Document documentExtension = RapidJsonUtils::CreateDocumentFromString(primitive.extras);
		auto count = documentExtension["ASOBO_primitive"]["PrimitiveCount"].GetInt();
		auto vertex_type = documentExtension["ASOBO_primitive"]["VertexType"].GetString();
		if (strcmp(vertex_type, "BLEND4") == 0)
		{
			count = 0;
		}
		Out(L"Vertex Type: %S", vertex_type);
		data.desc.Count = count * 3;
		auto start_index = 0;
		if (documentExtension["ASOBO_primitive"].HasMember("StartIndex"))
		{
			start_index = documentExtension["ASOBO_primitive"]["StartIndex"].GetInt();
		}
		data.desc.MiscFlags = start_index;
		data.desc.BindFlags = stoi(primitive.materialId);
	}
	else
	{
		data.desc.Count = accessor.count;
	}
	

	// Notify that we have a buffer and pass parameters required to create it
	Out(L"Buffer: %S", bufferType.c_str());
	parser.callbacks().Buffers(data);
}

void GLTFFileData::LoadMeshNode(const ParserContext& parser, const Node& mNode)
{
	LoadTransform(parser, mNode);

	auto gltfDoc = parser.document();

	auto meshNode = gltfDoc.meshes[mNode.meshId];
	auto i = 0;
	for (auto primitive : meshNode.primitives)
	{
		auto& exts = primitive.extensions;
		if (exts.find("KHR_draco_mesh_compression") != exts.end())
		{
			// TODO:
			// Read the BufferView and process the attributes
			// maybe factor the processing of attributes into a function
			// taking a parameter to determine whether or not to decompress

		}

		// Load Material (incl. textures)
		auto material = gltfDoc.materials[primitive.materialId];
		LoadMaterialNode(parser, material);

		// Load Index Buffer...
		LoadBufferFromAccessorId(parser, primitive.indicesAccessorId, "INDICES_" + std::to_string(i), primitive);
		LoadBufferFromAccessorId(parser, primitive.attributes["POSITION"], "POSITION_" + std::to_string(i), primitive);
		// TODO - smisom - these are interleaved buffers, so POSITION is actually everything!
		//LoadBufferFromAccessorId(parser, primitive.attributes["NORMAL"], "NORMAL");
		//LoadBufferFromAccessorId(parser, primitive.attributes["TEXCOORD_0"], "TEXCOORD_0");
		//LoadBufferFromAccessorId(parser, ACCESSOR_TANGENT, "TANGENT");
		//LoadBufferFromAccessorId(parser, primitive.uv0AccessorId, "TEXCOORD_0");

		// TODO: Load other buffers....
		++i;
	}
}

void GLTFFileData::LoadTransform(const ParserContext& parser, const Node& mNode)
{
	TransformData tdata;

	TransformationType type = mNode.GetTransformationType();
	tdata.hasMatrix = type == TRANSFORMATION_MATRIX;

	if (tdata.hasMatrix)
	{
		tdata.matrix[0] = mNode.matrix.values[0];
		tdata.matrix[1] = mNode.matrix.values[1];
		tdata.matrix[2] = mNode.matrix.values[2];
		tdata.matrix[3] = mNode.matrix.values[3];

		tdata.matrix[4] = mNode.matrix.values[4];
		tdata.matrix[5] = mNode.matrix.values[5];
		tdata.matrix[6] = mNode.matrix.values[6];
		tdata.matrix[7] = mNode.matrix.values[7];

		tdata.matrix[8] = mNode.matrix.values[8];
		tdata.matrix[9] = mNode.matrix.values[9];
		tdata.matrix[10] = mNode.matrix.values[10];
		tdata.matrix[11] = mNode.matrix.values[11];

		tdata.matrix[12] = mNode.matrix.values[12];
		tdata.matrix[13] = mNode.matrix.values[13];
		tdata.matrix[14] = mNode.matrix.values[14];
		tdata.matrix[15] = mNode.matrix.values[15];
	}
	else
	{
		tdata.rotation[0] = mNode.rotation.x;
		tdata.rotation[1] = mNode.rotation.y;
		tdata.rotation[2] = mNode.rotation.z;
		tdata.rotation[3] = mNode.rotation.w;

		tdata.translation[0] = mNode.translation.x;
		tdata.translation[1] = mNode.translation.y;
		tdata.translation[2] = mNode.translation.z;

		tdata.scale[0] = mNode.scale.x;
		tdata.scale[1] = mNode.scale.y;
		tdata.scale[2] = mNode.scale.z;
	}

	parser.callbacks().Transform(tdata);
}

void GLTFFileData::CheckExtensions(const GLTFDocument& document)
{
	for (auto ext : document.extensionsUsed)
	{
		Out(L"Ext Used: %S", ext.c_str());
	}
	for (auto ext : document.extensionsRequired)
	{
		Out(L"Ext Required: %S", ext.c_str());
	}
}

void GLTFFileData::ParseDocument(const ParserContext& parser)
{
	// Just start with the current scene for now..
	auto currentScene = parser.document().GetDefaultScene();
	Out(L"Loading scene %S", currentScene.id.c_str());
	LoadScene(parser, currentScene);
}
