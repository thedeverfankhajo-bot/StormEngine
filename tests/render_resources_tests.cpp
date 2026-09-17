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
    Shader fragment(ShaderDesc{ShaderStage::Fragment}, "void main() {}");
    assert(vertex.valid());
    assert(fragment.valid());
    assert(vertex.description().stage == ShaderStage::Vertex);
    assert(fragment.description().stage == ShaderStage::Fragment);
    assert(vertex.source() == "void main() {}");

    Material material(MaterialHandle(7), ShaderHandle(3));
    assert(material.valid());
    assert(material.handle() == MaterialHandle(7));
    assert(material.shader() == ShaderHandle(3));
    material.setShader(ShaderHandle(4));
    assert(material.shader() == ShaderHandle(4));

    RenderPipeline emptyPipeline;
    assert(!emptyPipeline.valid());

    RenderPipeline pipeline(ShaderHandle(11), ShaderHandle(12),
                            MaterialHandle(13), PrimitiveTopology::TriangleStrip);
    assert(pipeline.valid());
    assert(pipeline.shader() == ShaderHandle(11));
    assert(pipeline.vertexShader() == ShaderHandle(11));
    assert(pipeline.fragmentShader() == ShaderHandle(12));
    assert(pipeline.material() == MaterialHandle(13));
    assert(pipeline.topology() == PrimitiveTopology::TriangleStrip);

    DrawCommand source{};
    source.vertexBuffer = BufferHandle(20);
    source.vertexCount = 3;
    source.topology = PrimitiveTopology::Lines;

    const DrawCommand applied = pipeline.apply(source);
    assert(applied.vertexBuffer == source.vertexBuffer);
    assert(applied.vertexCount == source.vertexCount);
    assert(applied.shader == ShaderHandle(11));
    assert(applied.fragmentShader == ShaderHandle(12));
    assert(applied.material == MaterialHandle(13));
    assert(applied.topology == PrimitiveTopology::TriangleStrip);

    pipeline.setVertexShader(ShaderHandle(21));
    pipeline.setFragmentShader(ShaderHandle(22));
    pipeline.setMaterial(MaterialHandle(23));
    pipeline.setTopology(PrimitiveTopology::Triangles);
    const DrawCommand updated = pipeline.apply(source);
    assert(updated.shader == ShaderHandle(21));
    assert(updated.fragmentShader == ShaderHandle(22));
    assert(updated.material == MaterialHandle(23));
    assert(updated.topology == PrimitiveTopology::Triangles);

    pipeline.setShader(ShaderHandle(31));
    assert(pipeline.vertexShader() == ShaderHandle(31));

    pipeline.setFragmentShader(ShaderHandle{});
    assert(!pipeline.valid());

    return 0;
}
