#pragma once

#include "HmckWindow.h"
#include "HmckDevice.h"
#include "HmckGameObject.h"
#include "HmckRenderer.h"
#include "HmckSimpleRenderSystem.h"
#include "HmckModel.h"
#include "HmckCamera.h"


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>

namespace Hmck
{
	class FirstApp
	{
	public:
		static constexpr int WINDOW_WIDTH = 1280;
		static constexpr int WINDOW_HEIGHT = 720;

		FirstApp();
		~FirstApp();

		// delete copy constructor and copy destructor
		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		void run();

	private:
		void loadGameObjects();

		HmckWindow hmckWindow{ WINDOW_WIDTH, WINDOW_HEIGHT, "First Vulkan App" };
		HmckDevice hmckDevice{ hmckWindow };
		HmckRenderer hmckRenderer{ hmckWindow, hmckDevice };

		std::vector<HmckGameObject> gameObjects;
	};

}
