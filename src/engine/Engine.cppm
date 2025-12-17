module;

#include <memory>

export module hammock.engine.engine;

import hammock.engine.editor;


namespace hammock::engine {
    export class Engine final {
    public:
        Engine(EngineMode mode) : editor(std::make_unique<Editor>(mode)) {
        }

        void launch() const {
            editor->launch();
        }

    private:
        std::unique_ptr<Editor> editor;
    };
}
