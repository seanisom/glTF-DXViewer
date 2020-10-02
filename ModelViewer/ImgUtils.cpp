#include "pch.h"
#include "ImgUtils.h"
#include "Common\DirectXHelper.h"
//#include "dds_reader.h"

#include "DirectXTex.h"

#include <algorithm>
#include <filesystem>

#ifdef _DEBUG
#pragma comment(lib, "lx64/Debug/DirectXTex.lib")
#else
#pragma comment(lib, "lx64/Release/DirectXTex.lib")
#endif


using namespace DirectX;
namespace fs = std::filesystem;

vector<uint8_t> ImgUtils::LoadRGBAImage(void *imgFileData, size_t imgFileDataSize, uint32_t& width, uint32_t& height, 
	bool jpg, const wchar_t *filename)
{
	ComPtr<IWICImagingFactory> wicFactory;
	ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory)));

	IWICStream *pIWICStream;
	// Create a WIC stream to map onto the memory.
	ThrowIfFailed(wicFactory->CreateStream(&pIWICStream));

	// Initialize the stream with the memory pointer and size.
	ThrowIfFailed(pIWICStream->InitializeFromMemory(reinterpret_cast<BYTE*>(imgFileData), static_cast<DWORD>(imgFileDataSize)));

	bool isDDS = false;
	if (filename != nullptr)
	{
		// Check for DDS
		fs::path filePath = filename;
		
		if (filePath.extension() == ".DDS")
			isDDS = true;
	}
	else if (*(uint32_t*)imgFileData == 542327876) // "DDS "
	{
		isDDS = true;
	}

	// if DDS, short circuit, no need for WIC
	if (isDDS)
	{
		/*
		byte* pixels = reinterpret_cast<byte*>(ddsRead(static_cast<const unsigned char*>(imgFileData), DDS_READER_ARGB, 0));
		width = ddsGetWidth(static_cast<const unsigned char*>(imgFileData));
		height = ddsGetHeight(static_cast<const unsigned char*>(imgFileData));
		uint32_t outsize = width * height * 4;
		vector<uint8_t> image;
		image.resize(size_t(outsize));
		if (pixels != nullptr)
		{
			std::copy(pixels, pixels + outsize, reinterpret_cast<BYTE*>(image.data()));
		}
		ddsFree(pixels);
		*/

		//
		TexMetadata metadata;
		//GetMetadataFromDDSMemory(imgFileData, imgFileDataSize, DDS_FLAGS_NONE, metadata);
		//auto outsize = metadata.width * metadata.height * 4;

		ScratchImage scratch;
		ScratchImage destImage;
		LoadFromDDSMemory(imgFileData, imgFileDataSize, DDS_FLAGS_NONE, &metadata, scratch);

		//auto pixels_size = scratch.GetPixelsSize();
		width = metadata.width;
		height = metadata.height;
		//auto outsize = width * height * 4;
		Decompress(scratch.GetImages(), scratch.GetImageCount(), scratch.GetMetadata(), DXGI_FORMAT_UNKNOWN, destImage ); // No mipmaps for now
		auto pixels_size = destImage.GetImage(0, 0, 0)->slicePitch;
		auto pixels = destImage.GetImage(0, 0, 0)->pixels;
		vector<uint8_t> image(pixels_size);
		if (pixels != nullptr)
		{
			//std::copy(pixels, pixels + pixels_size, reinterpret_cast<BYTE*>(image.data()));
			std::copy(pixels, pixels + pixels_size, reinterpret_cast<BYTE*>(image.data()));
		}
		return image;
	}
	
	ComPtr<IWICBitmapDecoder> decoder;
	if (jpg)
	{
		ThrowIfFailed(wicFactory->CreateDecoderFromFilename(filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf()));
		//ThrowIfFailed(wicFactory->CreateDecoder(GUID_ContainerFormatJpeg, nullptr, decoder.GetAddressOf()));
		//decoder->Initialize(pIWICStream, WICDecodeMetadataCacheOnLoad);
	}
	else
	{
		ThrowIfFailed(wicFactory->CreateDecoderFromStream(pIWICStream, nullptr, WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf()));
	}

	ComPtr<IWICBitmapFrameDecode> frame;
	ThrowIfFailed(decoder->GetFrame(0, frame.GetAddressOf()));

	ThrowIfFailed(frame->GetSize(&width, &height));

	WICPixelFormatGUID pixelFormat;
	ThrowIfFailed(frame->GetPixelFormat(&pixelFormat));

	uint32_t rowPitch = width * sizeof(uint32_t);
	uint32_t imageSize = rowPitch * height;

	vector<uint8_t> image;
	image.resize(size_t(imageSize));

	if (memcmp(&pixelFormat, &GUID_WICPixelFormat32bppRGBA, sizeof(GUID)) == 0)
	{
		ThrowIfFailed(frame->CopyPixels(0, rowPitch, imageSize, reinterpret_cast<BYTE*>(image.data())));
	}
	else
	{
		ComPtr<IWICFormatConverter> formatConverter;
		ThrowIfFailed(wicFactory->CreateFormatConverter(formatConverter.GetAddressOf()));

		BOOL canConvert = FALSE;
		ThrowIfFailed(formatConverter->CanConvert(pixelFormat, GUID_WICPixelFormat32bppRGBA, &canConvert));
		if (!canConvert)
		{
			throw exception("CanConvert");
		}

		ThrowIfFailed(formatConverter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
			WICBitmapDitherTypeErrorDiffusion, nullptr, 0, WICBitmapPaletteTypeMedianCut));

		ThrowIfFailed(formatConverter->CopyPixels(0, rowPitch, imageSize, reinterpret_cast<BYTE*>(image.data())));
	}

	return image;
}
