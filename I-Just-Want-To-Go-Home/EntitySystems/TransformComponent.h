#pragma once

#include "Component.h"
#include <SDL2/SDL.h>

class TransformComponent : public Component
{
public:
	TransformComponent() : Component(std::type_index(typeid(TransformComponent))), test(0) { }
	void update(float dt) {}
	int getTest() { return test; }

private:
	int test;
};