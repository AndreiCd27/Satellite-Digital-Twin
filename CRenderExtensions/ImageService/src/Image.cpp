
#include "Image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// IMAGE ABSTRACT CLASS ////////////////////////////////////////////////////////////////////////////////////////////

int Image::IMG_CNT = 0;

Image::Image(int width, int height, const std::string& filepath) : width(width), height(height), filepath(filepath) {};

int Image::GetWidth() const {
    return width;
}
int Image::GetHeight() const {
    return height;
}
std::string Image::GetFilepath() const {
    return filepath;
}

// PNG IMAGE ////////////////////////////////////////////////////////////////////////////////////////////

PNG::PNG(int width, int height, const std::string& filepath)
    : Image(width, height, filepath) {

    std::filesystem::path fp = filepath;
    std::string extension = fp.extension().string();
    if (extension != ".png") throw ImageError("Invalid Image Format, input a valid .png file!");
}

void PNG::SetRawData(const std::vector<unsigned char>& Bytes) const {
    if (filepath == "") return;
    // set to 4 channels by default
    stbi_write_png(filepath.c_str(), width, height, 4, Bytes.data(), width * 4);
}

IMG_TYPE PNG::GetType() const {
    return IMG_TYPE::PNG;
}

// TIFF FILE ////////////////////////////////////////////////////////////////////////////////////////////

TIFF::TIFF(int width, int height, const std::string& filepath)
    : Image(width, height, filepath) {

    std::filesystem::path fp = filepath;
    std::string extension = fp.extension().string();
    if (extension != ".tiff" && extension != ".tif") throw ImageError("Invalid Image Format, input a valid .tiff file!");
}

void TIFF::SetRawData(const std::vector<unsigned char>& Bytes) const {
    TinyTIFFWriterFile* tiff = TinyTIFFWriter_open(filepath.c_str(), 8, TinyTIFFWriter_UInt, 4, width, height, TinyTIFFWriter_RGBA);

    if (tiff) {
        TinyTIFFWriter_writeImage(tiff, Bytes.data());
        TinyTIFFWriter_close(tiff);
    }
}

IMG_TYPE TIFF::GetType() const {
    return IMG_TYPE::TIFF;
}

// IMAGE ADAPTER ////////////////////////////////////////////////////////////////////////////////////////////

ImageAdapter* ImageAdapter::adapter = nullptr;

const std::string path = "CRenderExtensions/ImageService";

ImageAdapter::ImageAdapter() {
    // Initialize Compute Shader
    sampler.Setup((path + "/ComputeShaders/sampler.comp").c_str());
}
ImageAdapter::~ImageAdapter() {
    sampler.Delete();
    delete adapter;
}

ImageAdapter* ImageAdapter::GetAdapter() {
    if (adapter == nullptr) adapter = new ImageAdapter();
    return adapter;
}

void ImageAdapter::Resample(const Texture& FROM, const Texture& TO, bool flip) {

    sampler.Activate();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, FROM.GetTexID());

    sampler.SetInt("SrcTex", 0);

    glBindImageTexture(1, TO.GetTexID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    sampler.SetInt("DstTex", 1);
    sampler.SetInt("flip", flip ? 1 : 0);

    int toWidth = TO.GetWidth();
    int toHeight = TO.GetHeight();

    // Destination Texture Resolution
    sampler.SetUniformVector3("DstRes", AVector3((float)toWidth, (float)toHeight, 0.0f));

    GLuint numGroupsX = (toWidth + 15) / 16;
    GLuint numGroupsY = (toHeight + 15) / 16;

    // Run Compute Shader
    glDispatchCompute(numGroupsX, numGroupsY, 1);
    // Texture Barrier, wait for GPU to complete computations
    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
}

void ImageAdapter::Resample(const Image& FROM, const Texture& TO, ImageService* ImgService) {

    int fromWidth = FROM.GetWidth();
    int fromHeight = FROM.GetHeight();

    std::vector<unsigned char> fromDATA = ImgService->ExtractPixelData(
        FROM.GetType(), FROM.GetFilepath()
    );

    Texture IMG_TEX(fromWidth, fromHeight);

    if ((int)fromDATA.size() < fromWidth * fromHeight * 4) {
        std::cout << "VECTOR TOO SMALL ||||||||||||||||||||||||||||||\n";
        fromDATA.resize(fromWidth * fromHeight * 4, 0);
    }

    void* dataPTR = static_cast<void*>(fromDATA.data());
    IMG_TEX.SetupTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 
        GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, dataPTR);

    glBindTexture(GL_TEXTURE_2D, TO.GetTexID());
    TO.MinMagFilter(GL_NEAREST, GL_NEAREST);
    
    Resample(IMG_TEX, TO, false);

    IMG_TEX.Delete();
}

void ImageAdapter::Resample(const Image& FROM, const Image& TO, ImageService* ImgService) {

    int fromWidth = FROM.GetWidth();
    int fromHeight = FROM.GetHeight();

    int toWidth = TO.GetWidth();
    int toHeight = TO.GetHeight();

    if (fromWidth == toWidth && fromHeight == toHeight) {
        TO.SetRawData(
            ImgService->ExtractPixelData(
                FROM.GetType(), FROM.GetFilepath()
            )
        );
        return;
    }

    Texture tempTex(toWidth, toHeight);
    tempTex.SetupTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
        GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, nullptr);

    Resample(FROM, tempTex, ImgService);

    Convert(tempTex, TO);
}

void ImageAdapter::Convert(const Texture& FROM, const Image& TO) {

    int fromWidth = FROM.GetWidth();
    int fromHeight = FROM.GetHeight();

    if (fromWidth <= 0 || fromHeight <= 0) {
        std::cout << "Texture has no valid dimensions!\n";
        return;
    }

    std::vector<unsigned char> pixels(fromWidth * fromHeight * 4, 0);

    glBindTexture(GL_TEXTURE_2D, FROM.GetTexID());

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLint texWidth = 0, texHeight = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);

    std::cout << "GPU Texture Size: " << texWidth << "x" << texHeight << "\n";

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cout << "OpenGL Error during fetch: " << err << "\n";
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    TO.SetRawData(pixels);
}

void ImageAdapter::Convert(const Image& FROM, const Texture& TO, ImageService* ImgService) {
    Resample(FROM, TO, ImgService);
}

// IMAGE SERVICE ////////////////////////////////////////////////////////////////////////////////////////////

void ImageService::TruncatedPixelCopy(unsigned char* rawBytes,
    std::vector<unsigned char>& Bytes, int w, int h, int pw, int ph) {

    if (rawBytes == nullptr) {
        std::cout << "Can't truncate unidentified image! \n";
        return;
    }

    int min_w = (pw < w) ? pw : w;
    int min_h = (ph < h) ? ph : h;

    for (int y = 0; y < min_h; y++) {
        for (int x = 0; x < min_w; x++) {
            int isrc = (y * pw + x) * 4;
            int idst = (y * w + x) * 4;

            Bytes[idst] = rawBytes[isrc]; // R
            Bytes[idst + 1] = rawBytes[isrc + 1]; // G
            Bytes[idst + 2] = rawBytes[isrc + 2]; // B
            Bytes[idst + 3] = rawBytes[isrc + 3]; // A
        }
    }
}

std::pair<int, int> ImageService::GetImageDimensions(IMG_TYPE IMG, const std::string& filepath) {
    if (IMG == IMG_TYPE::PNG) {

        int png_width, png_height, channels;
        if (stbi_info(filepath.c_str(), &png_width, &png_height, &channels)) {
            return std::pair<int, int>(png_width, png_height);
        } else {
            std::cout << "Could not read PNG image\n";
            return std::pair<int, int>(-1,-1);
        }
    }
    if (IMG == IMG_TYPE::TIFF) {
        TinyTIFFReaderFile* tiff = TinyTIFFReader_open(filepath.c_str());
        if (tiff == nullptr) {
            std::cout << "Could not read TIFF image\n";
            return std::pair<int, int>(-1,-1);
        }

        int tiff_width = TinyTIFFReader_getWidth(tiff);
        int tiff_height = TinyTIFFReader_getHeight(tiff);
        TinyTIFFReader_close(tiff);
        return std::pair<int, int>(tiff_width, tiff_height);
    }
    return std::pair<int, int>(-1, -1);
}

std::vector<unsigned char> ImageService::ExtractPixelData(IMG_TYPE IMG, const std::string& filepath) {

    if (IMG == IMG_TYPE::PNG) {
        int png_channels, png_width, png_height;
        unsigned char* rawBytes = stbi_load(filepath.c_str(), &png_width, &png_height, &png_channels, 4);

        if (rawBytes == nullptr) {
            std::cout << "Could not read the image\n";
            return {};
        }

        std::vector<unsigned char> Bytes(rawBytes, rawBytes + png_width * png_height * 4);
        stbi_image_free(rawBytes);
        return Bytes;
    }
    if (IMG == IMG_TYPE::TIFF) {
        TinyTIFFReaderFile* tiff = TinyTIFFReader_open(filepath.c_str());
        if (tiff == nullptr) {
            std::cout << "Could not read TIFF image\n";
            return {};
        }

        int tiff_width = TinyTIFFReader_getWidth(tiff);
        int tiff_height = TinyTIFFReader_getHeight(tiff);

        std::vector<unsigned char> RawBytes(tiff_width * tiff_height * 4);
        TinyTIFFReader_getSampleData(tiff, RawBytes.data(), 0);
        TinyTIFFReader_close(tiff);
        return RawBytes;
    }
    return {};
}
std::vector<unsigned char> ImageService::ExtractPixelData(IMG_TYPE IMG, int width, int height, const std::string& filepath) {

    if (IMG == IMG_TYPE::PNG) {

        int png_width, png_height, png_channels;

        unsigned char* rawBytes = stbi_load(filepath.c_str(), &png_width, &png_height, &png_channels, 4);

        if (width != png_width || height != png_height) {
            std::vector<unsigned char>Bytes(width * height * 4, 0);
            TruncatedPixelCopy(rawBytes, Bytes, width, height, png_width, png_height);
            return Bytes;
        }

        std::vector<unsigned char> Bytes(rawBytes, rawBytes + png_width * png_height * 4);
        stbi_image_free(rawBytes);
        return Bytes;
    }
    if (IMG == IMG_TYPE::TIFF) {
        
        auto tiff_dim = GetImageDimensions(IMG, filepath);
        int tiff_width = tiff_dim.first;
        int tiff_height = tiff_dim.second;

        TinyTIFFReaderFile* tiff = TinyTIFFReader_open(filepath.c_str());

        std::vector<unsigned char> RawBytes(tiff_width * tiff_height * 4);
        TinyTIFFReader_getSampleData(tiff, RawBytes.data(), 0);

        if (width != tiff_width || height != tiff_height) {
            std::vector<unsigned char> Bytes(width * height * 4, 0);
            ImageService::TruncatedPixelCopy(RawBytes.data(), Bytes, width, height, tiff_width, tiff_height);
            TinyTIFFReader_close(tiff);
            return Bytes;
        }

        TinyTIFFReader_close(tiff);
        return RawBytes;
    }
    return {};
}

std::unique_ptr<Image> ImageService::CreateImage(IMG_TYPE IMG, const std::string& filepath) {

    // If the file does not exist, it choses a predifined size
    bool exists = std::filesystem::exists(filepath);
    
    if (!exists) {
        if (IMG == IMG_TYPE::PNG) {
            auto newIMG = std::make_unique<PNG>(256, 256, filepath);
            std::cout << "Created Blank Image at " << filepath << " (256x256 px)"
                << "\nUse ImageService::Delete(IMG) to delete it before the program finishes!\n";
            std::vector<unsigned char> blankPixels(256 * 256 * 4, 0);
            newIMG->SetRawData(blankPixels);
            return newIMG;
        }
        if (IMG == IMG_TYPE::TIFF) {
            auto newIMG = std::make_unique<TIFF>(256, 256, filepath);
            std::cout << "Created Blank Image at " << filepath << " (256x256 px)"
                << "\nUse ImageService::Delete(IMG) to delete it before the program finishes!\n";
            std::vector<unsigned char> blankPixels(256 * 256 * 4, 0);
            newIMG->SetRawData(blankPixels);
            return newIMG;
        }
        return nullptr;
    }

    auto dim = GetImageDimensions(IMG, filepath);
    int width = dim.first;
    int height = dim.second;

    if (IMG == IMG_TYPE::PNG) {

        // We have an unique image file with a specific file path
        auto newIMG = std::make_unique<PNG>(width, height, filepath);

        newIMG->SetRawData(
            ExtractPixelData(IMG, filepath)
        );

        return newIMG;
    }
    if (IMG == IMG_TYPE::TIFF) {
        // We have an unique image file with a specific file path
        auto newIMG = std::make_unique<TIFF>(width, height, filepath);

        newIMG->SetRawData(
            ExtractPixelData(IMG, filepath)
        );

        return newIMG;
    }
    return nullptr;
}

std::unique_ptr<Image> ImageService::CreateImage(IMG_TYPE IMG, int width, int height, const std::string& filepath) {

    bool exists = std::filesystem::exists(filepath);

    if (IMG == IMG_TYPE::PNG) {
        // We have an unique image file with a specific file path
        auto newIMG = std::make_unique<PNG>(width, height, filepath);

        if (!exists) {
            std::cout << "Created Blank Image at " << filepath 
                << "\nUse ImageService::Delete(IMG) to delete it before the program finishes!\n";
            std::vector<unsigned char> blankPixels(width * height * 4, 0);
            newIMG->SetRawData(blankPixels);
        }
        else {
            newIMG->SetRawData(
                ExtractPixelData(IMG, width, height, filepath)
            );
        }

        return newIMG;
    }
    if (IMG == IMG_TYPE::TIFF) {
        // We have an unique image file with a specific file path
        auto newIMG = std::make_unique<TIFF>(width, height, filepath);

        if (!exists) {
            std::cout << "Created Blank Image at " << filepath 
                << "\nUse ImageService::Delete(IMG) to delete it before the program finishes!\n";
            std::vector<unsigned char> blankPixels(width * height * 4, 0);
            newIMG->SetRawData(blankPixels);
        }
        else {
            newIMG->SetRawData(
                ExtractPixelData(IMG, width, height, filepath)
            );
        }

        return newIMG;
    }
    return nullptr;
}

ImageService* ImageService::service = nullptr;

ImageService* ImageService::GetService() {
    if (service == nullptr) service = new ImageService();
    return service;
}
ImageService::~ImageService() {
    delete service;
}

void ImageService::Screenshot(IMG_TYPE IMG, int Width, int Height, int ScreenWidth, int ScreenHeight, const std::string& filepath) {

    // Temporary Image Object
    Image* img = nullptr;
    if (IMG == IMG_TYPE::PNG) {
        img = new PNG(Width, Height, filepath);
    }
    if (IMG == IMG_TYPE::TIFF) {
        img = new TIFF(Width, Height, filepath);
    }
    if (img == nullptr) return;

    Texture screen(ScreenWidth, ScreenHeight);

    screen.SetupTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
            GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, nullptr);

    glBindTexture(GL_TEXTURE_2D, screen.GetTexID());

    glReadBuffer(GL_FRONT);

    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, ScreenWidth, ScreenHeight);

    glReadBuffer(GL_BACK);

    glFinish();

    Texture IMG_TEX(Width, Height);
    IMG_TEX.SetupTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE,
        GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, nullptr);

    TexAdapter->Resample(screen, IMG_TEX, true);

    TexAdapter->Convert(IMG_TEX, *img);

    delete img;

}

Request<IMG_TYPE, int, int, int, int, std::string>* ImageService::GetScreenshotRequest(
    IMG_TYPE IMG, int Width, int Height, int ScreenWidth, int ScreenHeight, const std::string& filepath) {

    Request<IMG_TYPE, int, int, int, int, std::string>* screenshot_request = new Request<IMG_TYPE, int, int, int, int, std::string>();

    screenshot_request->SetImplementation([this](
        IMG_TYPE imtyp, int w, int h, int sw, int sh, const std::string& f) {

        this->Screenshot(imtyp, w, h, sw, sh, f);
        }, IMG, Width, Height, ScreenWidth, ScreenHeight, filepath
    );
    return screenshot_request;
}

void ImageService::Delete(IMG_TYPE IMG, const Image& I) {
    std::filesystem::path filepath = I.GetFilepath();
    std::string extension = filepath.extension().string();
    // Extension check
    if (IMG == IMG_TYPE::PNG) {
        if (extension != ".png") throw ImageError("Could not delete image, file format not supported!");
    }
    if (IMG == IMG_TYPE::TIFF) {
        if (extension != ".tiff" && extension != ".tif") throw ImageError("Could not delete image, file format not supported!");
    }
    // Delete file
    try {
        if (std::filesystem::remove(filepath)) {
        } else {
            std::cout << "Image File Not Found\n";
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem Error: " << e.what() << "\n";
    }
}