export module hammock.renderer.base_renderer;

namespace hammock::renderer {
    export class BaseRenderer {
    public:
        virtual ~BaseRenderer() = default;
        virtual void drawFrame() = 0;



    };
}