#include "Game.h"
#include "../Physics/PhysicsSystem.h"
#include <SDL2\SDL.h>
#include <chrono>

Game::~Game()
{
	SDL_DestroyWindow(_window);
	SDL_Quit();
}

void Game::initialize()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cerr << "ERROR: SDL could not initialize. SDL_Error:  " << SDL_GetError() << std::endl;
		return;
	}

	// prepare opengl version (4.5) for SDL 
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);	// using core as opposed to compatibility or ES 

	// create window
	_window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (_window == NULL)
	{
		std::cerr << "ERROR: SDL window could not be created. SDL_Error:  " << SDL_GetError() << std::endl;
		return;
	}

	// get window surface (not necessary)
	_screenSurface = SDL_GetWindowSurface(_window);

	// initialize sdl opengl context 
	_context = SDL_GL_CreateContext(_window);
	if (_context == NULL)
	{
		std::cerr << "ERROR: SDL failed to create openGL context. SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}

	// initialize opengl 
	if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
	{
		std::cerr << "ERROR: GLAD failed to initialize opengl function pointers." << std::endl;
		return;
	}
	std::cout << "Vendor:\t" << glGetString(GL_VENDOR) << std::endl
		<< "Renderer:\t" << glGetString(GL_RENDERER) << std::endl
		<< "Version:\t" << glGetString(GL_VERSION) << std::endl;

	// configure opengl 
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

void Game::setActiveScene(Scene* scene)
{
	std::cout << "WARNING: Game::setActiveScene() may produce undefined behaviour" << std::endl;
	// TODO: need to wait for update cycle to finish before changing scene.

	// disable previous sce7ne 
	if (activeScene)
		activeScene->rootEntity->setEnabled(false);

	// enable new scene 
	activeScene = scene;
	scene->rootEntity->setEnabled(true);
}

void Game::addSystem(std::unique_ptr<System> system, bool synchronous)
{
	auto sys_type = std::type_index(typeid(*system));

	// check if system is already added 
	if (_systemList.find(sys_type) != _systemList.end())
	{
		std::cerr << "ERROR: Readding an existing system to engine." << std::endl;
		return;
	}

	// place in correct update list 
	if (synchronous)
	{
		_systems.push_back(system.get());
	}
	else
	{
		_concurrentSystems.push_back(system.get());
		system->startLooping();
	}
	
	// keep track of system 
	_systemList[sys_type] = std::move(system);
}

void Game::addEntity(Entity* entity)
{
	// verify the developer isn't doing funny business 
	const bool is_in = _additionVerification.find(entity->getID()) != _additionVerification.end();
	if (is_in)
	{
		std::cerr << "WARNING: GAME::addEntity - You tried to add an entity to the scene multiple times, please check your code" << std::endl;
		return;
	}

	// prepare to add this entity in the next frame
	_additionList.emplace_back(0, entity);
	_additionVerification.insert(entity->getID());
}

void Game::loop()
{
	_running = true;

	// ===== PERFORMANCE MEASUREMENTS =====
	// This is only to measure CPU performance. For GPU use OpenGLProfiler.
	std::chrono::high_resolution_clock::time_point startTime;
	std::chrono::high_resolution_clock::time_point endTime;


	while (_running)
	{
		// std::cout << "FRAME: " << frame++ << std::endl;

		startTime = std::chrono::high_resolution_clock::now();

		// 1. entity addition & deletion 
		resolveEntities(activeScene->rootEntity.get());
		resolveCleanup();

		// 2. entity update 
		updateEntity(activeScene->rootEntity.get(), 0.016f);

		// 3. system update 
		// concurrent systems 
		_waitingForSystems = _concurrentSystems.size();
		for (int i = 0; i < _concurrentSystems.size(); i++)
		{
			_concurrentSystems[i]->notifyUpdate(0.016f);
		}
		// synchronous systems
		for (int i = 0; i < _systems.size(); i++)
		{
			_systems[i]->update(0.016f);
			_systems[i]->clearComponents();	// cleanup for next iteration
		}

		// 4. wait for systems to update 
		std::unique_lock<std::mutex> lck(_mtx);
		while (_waitingForSystems > 0)
		{
			_cv.wait(lck);
		}
		lck.unlock();

		endTime = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
		// TODO: do stuff with execution duration (like adjust which system gets updates).
		
		SDL_GL_SwapWindow(_window);
	}
}

void Game::stop()
{
	_running = false;
}

// will update all components in an entity, children in entity, and notify systems
void Game::updateEntity(Entity * entity, float dt)
{
	// stop if disabled 
	if (!entity->getEnabled()) return;

	// go thru all children first (remember structure incase of move)
	auto children = entity->getChildren();
	for (int i = 0; i < children.size(); i++)
	{
		updateEntity(children[i], dt);
	}

	// for all components ...
	auto components = entity->getComponents();
	for (int i = 0; i < components.size(); i++)
	{
		// ... ensure it is enabled ...
		if (!components[i]->getEnabled()) continue;
		// ... update it ...
		auto type = std::type_index(typeid(*components[i]));
		components[i]->update(dt);
		// ... and notify systems
		for (int j = 0; j < _systems.size(); j++)
			_systems[j]->addComponent(type, components[i]);
		for (int j = 0; j < _concurrentSystems.size(); j++)
			_concurrentSystems[j]->addComponent(type, components[i]);

	}
}

void Game::resolveEntities(Entity * entity)
{
	int id = entity->getID();

	// check if branch should be deleted
	auto toDelete = _deletionList.find(id);
	if (toDelete != _deletionList.end())
	{
		entity->release();
		_deletionList.erase(toDelete);
		return;
	}

	// check if something should be added 
	for (auto& entry : _additionList)
	{
		if (entry.target == id)
		{
			std::cout << "Adding entity " << entry.entity->getID() << " to parent entity " << id << std::endl;
			entity->addChild(entry.entity);
		}
	}
	// this is pretty efficient apparently. 
	_additionList.erase(
		std::remove_if(_additionList.begin(), _additionList.end(), [id](const EntityAction& e) { return e.target == id; }),
		_additionList.end());	

	// go thru remaining children 
	auto children = entity->getChildren();
	for (int i = 0; i < children.size(); i++)
	{
		resolveEntities(children[i]);
	}
}

void Game::resolveCleanup()
{
	if (_deletionList.size() > 0)
	{
		std::cout << "Game::resolveCleanup() found " << _deletionList.size() << " entities not explicitly deleted. Entity may have been deleted via parent deletion or leaked" << std::endl;
	}
	_deletionList.clear();

	if (_additionList.size() > 0)
	{
		for (auto& entry : _additionList)
		{
			std::cout << "Game::resolveCleanup() could not add entity: " << entry.entity->getID() << " as parent: " << entry.target << " was not found" << std::endl;
			entry.entity->release();
		}
	}
	_additionList.clear();
	_additionVerification.clear();
}
