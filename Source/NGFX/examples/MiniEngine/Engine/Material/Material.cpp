#include "MiniEngine.h"
#include "Material.h"

namespace k3d
{
    Material::Material()
    {
    }

    Material::~Material()
    {

    }

    void Material::SetTextureParam(String const & name, ngfx::Ptr<ngfx::Texture> texture)
    {
    }

    void Material::SetConstantParam(String const& name, float constant)
    {

    }

    void Material::SetVectorParam(String const & name, k3d::math::Vec3f const & vec3)
    {
    }

    MaterialRenderProxy* Material::CreateRenderProxy()
    {
        return nullptr;
    }

}