#include <cassert>
#include <string>

#include "storm/render/Material.hpp"
#include "storm/render/RenderPipeline.hpp"
#include "storm/render/Shader.hpp"

int main() {
    using namespace storm::render;

    Shader empty;
    assert(!empty.valid());
    assert(empty.description().stage == ShaderStage::Vertex);

    Shader vertex(ShaderDesc{ShaderStage::Vertex}, "void main() {}");
    assert(vertex.valid());
    assert(vertex.description().stage == ShaderStage::Vertex);
    assert(vertex.source() == "void main() {}");

    Material material(MaterialHandle(7), ShaderHandle(3));
    assert(material.valid());
    assert(material.handle() == MaterialHandle(7));
    assert(material.shader() == ShaderHandle(3));
    material.setShader(ShaderHandle(4));
    assert(material.shader() == ShaderHandle(4));

    RenderPipeline emptyPipeline;
    assert(!emptyPipeline.valid());

    RenderPipeline pipeline(ShaderHandle(11), MaterialHandle(12), PrimitiveTopology::TriangleStrip);
    assert(pipeline.valid());
    assert(pipeline.shader() == ShaderHandle(11));
    assert(pipeline.material() == MaterialHandle(12));
    assert(pipeline.topology() == PrimitiveTopology::TriangleStrip);

    DrawCommand source{};
    source.vertexBuffer = BufferHandle(20);
    source.vertexCount = 3;
    source.topology = PrimitiveTopology::Lines;

    const DrawCommand applied = pipeline.apply(source);
    assert(applied.vertexBuffer == source.vertexBuffer);
    assert(applied.vertexCount == source.vertexCount);
    assert(applied.shader == ShaderHandle(11));
    assert(applied.material == MaterialHandle(12));
    assert(applied.topology == PrimitiveTopology::TriangleStrip);

    pipeline.setShader(ShaderHandle(21));
    pipeline.setMaterial(MaterialHandle(22));
    pipeline.setTopology(PrimitiveTopology::Triangles);
    const DrawCommand updated = pipeline.apply(source);
    assert(updated.shader == ShaderHandle(21));
    assert(updated.material == MaterialHandle(22));
    assert(updated.topology == PrimitiveTopology::Triangles);

    return 0;
}
