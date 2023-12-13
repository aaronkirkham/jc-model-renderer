#ifndef JCMR_RENDER_TEXTURE_H_HEADER_GUARD
#define JCMR_RENDER_TEXTURE_H_HEADER_GUARD

#include "platform.h"

struct ID3D11Resource;
struct ID3D11ShaderResourceView;

namespace jcmr
{
struct Renderer;

struct Texture {
    static Texture* create(const std::string& filename, Renderer& renderer);
    static void     destroy(Texture* instance);

    virtual ~Texture() = default;

    virtual bool load(const ByteArray& buffer) = 0;

    virtual const std::string&        get_filename() const = 0;
    virtual ID3D11Resource*           get_texture() const  = 0;
    virtual ID3D11ShaderResourceView* get_srv() const      = 0;
};
} // namespace jcmr

#endif // JCMR_RENDER_TEXTURE_H_HEADER_GUARD
