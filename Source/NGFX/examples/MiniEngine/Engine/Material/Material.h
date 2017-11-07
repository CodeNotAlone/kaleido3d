#pragma once

#include <unordered_map>

namespace k3d
{
    enum class MaterialChannel
    {
        None,
        BaseColor,
        Metallic,
        Roughness,
        Specular,
//        Constant,
    };

    enum class MaterialParamType
    {
        Texture,
        Constant,
        ConstantVector,
        Sampler,
    };

    struct MaterialParam
    {
        MaterialChannel          Channel;
        MaterialParamType        Type;
        ngfx::Ptr<ngfx::Texture> Texture;
        union
        {
            int         I32;
            uint32_t    U32;
            float       F32;
        };
    };

    class MaterialRenderProxy;

    class K3D_CORE_API Material
    {
    public:
        Material();
        virtual ~Material();

        virtual void SetTextureParam(String const& name, ngfx::Ptr<ngfx::Texture> texture);
        virtual void SetConstantParam(String const& name, float constant);
        virtual void SetVectorParam(String const& name, k3d::math::Vec3f const& vec3);
        
        virtual MaterialRenderProxy* CreateRenderProxy();

    protected:
        std::unordered_map<String, MaterialParam> m_Params;
    };
}