#pragma once

#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <set>
#include <memory>
#include <chrono>
#include <SDL2\SDL.h>
#include "../AssetLoader.h"
#include "..\EntitySystems\System.h"
#include "Scene.h"
#include "CpuProfiler.h"
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>

//Screen dimension constants
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

struct EntityAction
{
	int target; 
	Entity* entity;
	EntityAction(int parent, Entity* e)
	{
		target = parent;
		entity = e;
	}
};

class Game
{

// singleton pattern (threadsafe: https://stackoverflow.com/questions/1008019/c-singleton-design-pattern)
public:
	static Game& instance()
	{
		static Game _instance;
		return _instance;
	}
	Game(Game const&) = delete;
	void operator=(Game const&) = delete;
private:
	Game() { };
	~Game();

// variables 
public:
	Scene* activeScene = nullptr;
	AssetLoader loader;
	Entity* player;

	//// events 
	//boost::signals2::signal<void(std::type_index, Component*)> componentCreated;
	//boost::signals2::signal<void(std::type_index, Component*)> componentDestroyed;

private:
	// sdl 
	SDL_Window* _window = NULL;
	SDL_Surface* _screenSurface = NULL;
	SDL_GLContext _context = NULL;
	// engine 
	std::set<int> _deletionList;
	std::vector<EntityAction> _additionList;
	std::set<int> _additionVerification;	// used to ensure an entity isn't added twice
	std::vector<Entity*> _entities;			// used to ANNIHILATE.

	std::vector<std::unique_ptr<System>> _systems;		// systems that are only updated once per frame
	std::vector<std::unique_ptr<System>> _frameSystems;	// systems that can be updated multiple times per frame (fixed update).
	const std::chrono::nanoseconds _frameTime = std::chrono::milliseconds( (long)(16.6666666666666666666) );
	bool _initialized = false;
	bool _running = false;
	CpuProfiler _profiler;
	int _cullCount = 0;

// functions 
public:
	// Initializes the core engine.
	void initialize();

	// Changes the active scene. Does not dispose of the previous scene, but does disable it.
	void setActiveScene(Scene* scene);
	
	// Add a system to receive updates and enabled components. 
	// Note: components update function will run regardless if the system is in the update list or not.
	void addSystem(std::unique_ptr<System> system);

	// DEPRECATED
	void addEntity(std::unique_ptr<Entity> entity);
	
	// Starts the game loop. Blocking.
	void loop();
	
	// Stops the game loop.
	void stop();

	// Properly dispose an entity in the next frame. 
	// Note: Better to call Entity.delete() as there may be some custom funtionality.
	void deleteEntity(int id)
	{
		_deletionList.insert(id);
	}

	// Properly dispose an entity in the next frame.
	// Note: Better to call Entity.delete() as there may be some custom functionality.
	void deleteEntity(Entity* e)
	{
		deleteEntity(e->getID());
	}

	// Properly add an entity in the next frame. 
	// Note: you can directly add an entity with activeScene->rootEntity->addChild(e*) if you know what you're doing.
	void addEntity(Entity* entity);

	// Properly add an entity to a specified parent in the next frame.
	// Note: you can directly add an entity with entity->addChild(e*) if you know what you're doing.
	void addEntity(Entity* entity, int parent);

private: 

	// will update all components in an entity, children in entity, and notify systems
	void updateEntity(Entity* entity, float dt);

	// crawls thru scene and resolves entity addition and deletion before updates
	void resolveEntities(Entity* entity, bool collectComponents);

	// cleans up any remaining entities to be deleted or added
	void resolveCleanup();

	// splitting up is actually faster. this is b/c resolve can do a quick list check and stop early. 
	void resolveAddDel(Entity* entity);

	void yoloadd(Entity * e);

	void resolveUpdates(Entity* entity, bool torf);

	std::atomic<int> threadsWorking = 0;
	std::mutex mtx;
	std::condition_variable cv;
	int threadCount = 4;

	void superiorUpdate()
	{
		std::vector<std::thread> t;
		t.reserve(threadCount);
		threadsWorking = threadCount;

		for (int i = 0; i < threadCount; i++)
		{
			// start thread to do junk
			t.emplace_back(std::bind(&Game::annhilate, this, i));
		}

		// wait for all to finish
		for (int i = 0; i < threadCount; i++)
		{
			t[i].join();
		}

		// manual update for now b/c input is not a system
		auto c = player->getComponents();
		for (auto& comp : c)
		{
			if (player->enableStatus && comp->getEnabled())
				comp->update(0.016);
		}

		// reset for next frame
		for (int i = 0; i < _entities.size(); i++)
		{
			_entities[i]->enableStatus = true;
			_entities[i]->worldTransPrepared = false;
		}


		std::cout << "superior update finished" << std::endl;
	}

	void annhilate(int threadNum);
};

