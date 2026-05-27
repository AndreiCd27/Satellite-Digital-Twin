#pragma once

#include "precompile.h"
#include "Texture.h"
#include "ComputeShader.h"
#include "Request.h"

#include <memory>

// External Libs: stb & tinytiffwriter //////

#include <stb_image_write.h>
#include <stb_image.h>

#include <tinytiffreader.h>
#include <tinytiffwriter.h>

#include <filesystem>

////////////////////////////////////////////

// Supported image formats
enum class IMG_TYPE { PNG, TIFF };

class ImageError : public std::runtime_error {
public:
	ImageError(const std::string& MSG) : std::runtime_error(MSG) {}
};

// Main Image Interface
class Image {
	static int IMG_CNT;
	int IMG_ID;

protected:
	int width, height;
	std::string filepath = "";

public:
	// Helper Function for Image Factory & Adapter
	virtual void SetRawData(const std::vector<unsigned char>& Bytes) const = 0;

	virtual IMG_TYPE GetType() const = 0;

	Image(int width, int height, const std::string& _filepath);

	int GetGlobalID() const{ return IMG_ID; }

	int GetWidth() const;
	int GetHeight() const;
	std::string GetFilepath() const;

	// Moved method to Image Factory, only it interacts with OS
	//virtual void SaveToFile(const std::string& filepath) const = 0;

	// Default constructor, file deletion is handled in Image Factory
	virtual ~Image() = default;
};

class TIFF : public Image {
public:

	void SetRawData(const std::vector<unsigned char>& Bytes) const override;

	IMG_TYPE GetType() const override;

	TIFF(int width, int height, const std::string& _filepath);
};

class PNG : public Image {
public:

	void SetRawData(const std::vector<unsigned char>& Bytes) const override;

	IMG_TYPE GetType() const override;

	PNG(int width, int height, const std::string& _filepath);
};

class ImageService;

// Adapter design pattern
class ImageAdapter {

	ImageAdapter();
	~ImageAdapter();

	static ImageAdapter* adapter;

	// This simple Compute Shader is used to resize images
	// Converts any image into a gl_Texture and writes to another one
	// GPU automatically does hardware interpolation between pixels
	// We read back the pixels on the CPU and call SetRawBytes()
	ComputeShader sampler;

public:

	static ImageAdapter* GetAdapter();

	// Resample TEXTURE - TO - TEXTURE
	void Resample(const Texture& FROM, const Texture& TO, bool flip);
	// Resample IMAGE - TO - TEXTURE
	void Resample(const Image& FROM, const Texture& TO, ImageService* ImgService);
	// Resample IMAGE - TO - IMAGE
	void Resample(const Image& FROM, const Image& TO, ImageService* ImgService);

	// Convert IMAGE - TO - TEXTURE (size match)
	void Convert(const Image& FROM, const Texture& TO, ImageService* ImgService);
	// Convert TEXTURE - TO - IMAGE (size match)
	void Convert(const Texture& FROM, const Image& TO);
	
	// Image To Image Conversion uses:
	// 1) Image -> Texture conversion
	// 2) Upsamling / Downsampling via Compute Shader
	// 3) Texture -> Image conversion
	void Convert(const Image& FROM, const Image& TO);
};


struct ImageData {
	std::vector<unsigned char> pixels;
	int width, height;
};

// Factory design pattern
class ImageService {
	ImageService() = default;
	~ImageService();

	static ImageService* service;

	void GetRelativePath(const std::string& path);

	void TruncatedPixelCopy(unsigned char* rawBytes, 
		std::vector<unsigned char>& Bytes, int w, int h, int pw, int ph);

public:

	ImageAdapter* TexAdapter = ImageAdapter::GetAdapter();

	static ImageService* GetService();

	std::pair<int, int> GetImageDimensions(IMG_TYPE IMG, const std::string& filepath);

	std::vector<unsigned char> ExtractPixelData(IMG_TYPE IMG, const std::string& filepath);

	std::vector<unsigned char> ExtractPixelData(IMG_TYPE IMG, 
		int width, int height, const std::string& filepath);

	// Creates a blank image to disk with the correct file format
	std::unique_ptr<Image> CreateImage(IMG_TYPE IMG, const std::string& filepath);
	std::unique_ptr<Image> CreateImage(IMG_TYPE IMG, int width, int height, const std::string& filepath);

	// Copies an image to another image, uses ImageAdapter if file formats are different
	// If the dimensions of the image don't match, it does upsampling/downsampling in a compute shader
	std::unique_ptr<Image> CopyImageTo(const Image& FROM, const Image& TO);

	void Screenshot(IMG_TYPE IMG, int Width, int Height, int ScreenWidth, int ScreenHeight, 
		const std::string& filepath);

	Request<IMG_TYPE, int, int, int, int, std::string>* GetScreenshotRequest(
		IMG_TYPE IMG, int Width, int Height, int ScreenWidth, int ScreenHeight, const std::string& filepath);

	void Delete(IMG_TYPE IMG, const Image& I);

	friend class Image;
};