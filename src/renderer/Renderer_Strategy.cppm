module;
export module hammock.renderer.renderer:strategy;

namespace hammock::renderer {

    class IRenderingStrategy {
    public:
        virtual ~IRenderingStrategy() = default;

        virtual void draw() = 0;
    };
}