#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>
#include <chrono>

#include "Platform/HmckWindow.h"
#include "HmckDevice.h"
#include "HmckRenderer.h"
#include "Systems/HmckUserInterface.h"
#include "HmckCamera.h"
#include "Controllers/KeyboardMovementController.h"
#include "HmckBuffer.h"
#include "HmckDescriptors.h"
#include "Utils/HmckLogger.h"
#include "IApp.h"
#include "HmckEntity.h"
#include "HmckScene.h"


#ifndef MODELS_DIR
#define MODELS_DIR "../../Resources/Models/"
#endif // !MODELS_DIR

#ifndef MATERIALS_DIR
#define MATERIALS_DIR "../../Resources/Materials/"
#endif // !MATERIALS_DIR


namespace Hmck 
{
	class VolumetricRenderingApp : public IApp
	{
		// There is so much code in this class 
		// TODO make more abstraction layers over the IApp

	public:

		Binding sceneBinding = 0;
		Binding textureBinding = 1;
		Binding transformBinding = 2;
		Binding materialPropertyBinding = 3;

		VolumetricRenderingApp();

		// Inherited via IApp
		virtual void run() override;
		virtual void load() override;

		std::unique_ptr<DescriptorPool> descriptorPool{};
		std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{};

		std::unique_ptr<Scene> scene{};
	};

}

