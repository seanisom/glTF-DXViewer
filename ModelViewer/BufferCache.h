#pragma once

#include "BoundingBox.h"
#include "Common\DirectXHelper.h"

using namespace Microsoft::WRL;
using namespace DX;

class BufferDescriptor
{
public:
	BufferDescriptor(GLTF_BufferData ^ data, shared_ptr<DeviceResources> deviceResources) :
		_data(data),
		_deviceResources(deviceResources)
	{
	}
	shared_ptr<DeviceResources> DevResources() { return _deviceResources; }
	const shared_ptr<DeviceResources> DevResources() const { return _deviceResources; }

	size_t Hash();

	const wchar_t *ContentType() const { return _data->BufferDescription->BufferContentType->Data(); }
	unsigned int AccessorIdx() const { return _data->BufferDescription->accessorIdx; }

	GLTF_BufferData^ Data() { return _data; }

private:
	bool _hashCalculated = false;
	shared_ptr<DeviceResources> _deviceResources;
	size_t _hash;
	unsigned int _accessorIdx;
	GLTF_BufferData ^ _data;
};

template <>
struct hash<BufferDescriptor>
{
	size_t operator()(const BufferDescriptor& descriptor) const
	{
		size_t res = 0;
		hash<const wchar_t *> myHash;
		res ^= myHash(descriptor.ContentType());
		hash<unsigned int> myHash2;
		res ^= myHash2(descriptor.AccessorIdx());
		return res;
	}
};

//class BufferWrapper
//{
//public:
//	BufferWrapper(GLTF_BufferData^ data, ComPtr<ID3D11Buffer> buffer) :
//		_data(data),
//		_buffer(buffer)
//	{
//	}
//	BufferWrapper() {}
//	ComPtr<ID3D11Buffer>& Buffer() { return _buffer; }
//
//	GLTF_BufferData^ Data() { return _data; }
//
//private:
//	GLTF_BufferData ^ _data;
//	ComPtr<ID3D11Buffer> _buffer;
//};

class ID3D11BufferWrapper
{
public:
	static shared_ptr<ID3D11BufferWrapper> Create(BufferDescriptor descriptor)
	{
		auto ret = make_shared<ID3D11BufferWrapper>();

		int bindFlags = 0;
		auto content_type = std::wstring(descriptor.Data()->BufferDescription->BufferContentType->Data());
		if (content_type.rfind(L"POSITION", 0) == 0 ||
			content_type.rfind(L"NORMAL", 0) == 0 ||
			content_type.rfind(L"TEXCOORD_0", 0) == 0 )
		{
			bindFlags = D3D11_BIND_VERTEX_BUFFER;
		}
		else if (content_type.rfind(L"INDICES", 0) == 0)
		{
			bindFlags = D3D11_BIND_INDEX_BUFFER;
		}
		else
		{
			/*throw new std::exception("Unknown Buffer Type");*/
			return ret;
		}

		if (content_type.rfind(L"POSITION", 0) == 0)
		{
			ret->SetBoundingBox(
				BoundingBox<float>::CreateBoundingBoxFromVertexBuffer(
				(void *)descriptor.Data()->BufferDescription->pSysMem, descriptor.Data()->SubResource->ByteWidth)
			);
		}

		// Create the buffers...
		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = (void *)descriptor.Data()->BufferDescription->pSysMem;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;

		CD3D11_BUFFER_DESC vertexBufferDesc(static_cast<unsigned int>(descriptor.Data()->SubResource->ByteWidth), bindFlags);
		vertexBufferDesc.StructureByteStride = static_cast<unsigned int>(descriptor.Data()->SubResource->StructureByteStride);

		auto device = descriptor.DevResources()->GetD3DDevice();
		ThrowIfFailed(device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, ret->AddressOfBuffer()));
		//descriptor.DevResources()->GetD3DDeviceContext()->Flush();
		return ret;
	}

	BoundingBox<float>& GetBoundingBox() { return _bbox; }
	void SetBoundingBox(BoundingBox<float> bbox) { _bbox = bbox; }

	ComPtr<ID3D11Buffer>& Buffer() { return _buffer; }
	ID3D11Buffer **AddressOfBuffer() { return _buffer.GetAddressOf(); }

private:
	BoundingBox<float> _bbox;
	ComPtr<ID3D11Buffer> _buffer;
};

template<class TBufferWrapper>
class BufferCache
{
public:
	BufferCache() {};
	~BufferCache() {};

	shared_ptr<TBufferWrapper> FindOrCreateBuffer(BufferDescriptor descriptor)
	{
		// get the hash value...
		size_t hash = descriptor.Hash();
		map<size_t, shared_ptr<TBufferWrapper>>::iterator res = _buffers.find(hash);
		if (res != _buffers.end())
			return (*res).second;

		auto ret = TBufferWrapper::Create(descriptor);
		_buffers[hash] = ret;
		return ret;
	}

private:
	map<size_t, shared_ptr<TBufferWrapper>> _buffers;
};

