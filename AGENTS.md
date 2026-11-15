# Hammock Agent Instructions

These instructions apply to all work in this repository. Follow the established patterns in nearby code and keep changes focused.

## Code quality and style

- Write clean, readable, reusable code with clear component boundaries. Prefer small, cohesive components and existing project abstractions over duplicated logic or oversized classes.
- Follow the repository's C++23 standard and `.clang-format` settings (Google base style, four-space indentation, 110-column limit). Match the naming, include, and formatting conventions of nearby files.
- Use modern, cross-platform C++ for engine and rendering code. Keep platform-specific behavior behind narrow interfaces and avoid platform assumptions in shared code.
- Treat performance as a core requirement, especially in rendering and per-frame paths. Avoid unnecessary allocations, copies, synchronization, and GPU/CPU stalls; measure before adding complexity for an optimization.
- Keep comments sparse. Comments and API documentation should be brief, accurate, and explain intent, constraints, or non-obvious behavior rather than restating the code.
- Preserve ownership and lifetime clarity; prefer RAII and established resource-management patterns. Add or update focused tests when behavior changes.

## Architecture and graphics

- Keep responsibilities aligned with the existing `src/core`, `src/renderer`, `src/app`, `src/ui`, and `src/utils` boundaries. Reuse existing handles, resource managers, and frame/snapshot interfaces where appropriate.
- Use Vulkan-Hpp (`<vulkan/vulkan.hpp>`) in `src/core` for engine-core Vulkan integration. Outside `src/core`, do not include Vulkan headers, use Vulkan types/functions, or expose Vulkan objects through APIs. Use or add engine-owned abstractions instead. Treat existing Vulkan leaks outside core as legacy; do not extend them, and contain or remove them when directly relevant to a change.
- Write engine shaders in Slang (`.slang`). Use Clay for UI layout and follow the existing UI-to-renderer boundary; do not couple UI code to Vulkan objects.
- Consult the relevant primary documentation before choosing or changing graphics/UI APIs:
  - Slang: https://shader-slang.org/docs/
  - Vulkan: https://docs.vulkan.org/
  - Clay: https://github.com/nicbarker/clay

## Feature proposal and approval workflow

For feature implementation requests, do not start implementation immediately:

1. Inspect the relevant code, architecture, constraints, and documentation. Identify requirements that are unclear and all foreseeable blockers.
2. Before preparing a plan, ask the user targeted questions about unresolved requirements or blockers. Wait for their answers; do not guess on decisions that materially affect behavior or architecture.
3. Explore viable implementation options, compare correctness, maintainability, performance, and cross-platform implications, then select the best overall option.
4. Present a short, informed proposition and a detailed implementation plan for the selected option, including affected components, key steps, and validation. Ask for explicit approval and stop.
5. After approval, implement according to the agreed plan. If a new blocker or a material scope/design change arises, explain it and get approval before proceeding beyond the plan.

## Build and validation

- Use the existing CMake presets and test setup. The debug workflow is `cmake --preset debug`, `cmake --build --preset debug`, and `ctest --test-dir build --output-on-failure`.
- Shader compilation is part of the renderer build and requires the Vulkan SDK (`VULKAN_SDK`). For a change, run the smallest relevant build, shader compilation, and/or test command; report any environment blocker explicitly.
