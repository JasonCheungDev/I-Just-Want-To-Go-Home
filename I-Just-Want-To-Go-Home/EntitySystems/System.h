#pragma once

#include <atomic>
#include <vector>
#include <iterator>
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <condition_variable>
#include "Component.h"

class Game;	// circular dependencies


class System
{
	typedef std::unordered_map<std::type_index, std::vector<Component*>> ComponentMap;

public:
	System();
	System(std::vector<std::type_index> forComponents);
	~System();
	
	virtual void update(float dt) = 0;

	virtual void addComponent(std::type_index t, Component* component)
	{
		auto type = std::type_index(typeid(component));
		if (_componentMap.count(t))
		{
			_componentMap[t].push_back(component);
		}
	}
	
	virtual void clearComponents()
	{
		for (ComponentMap::iterator it = _componentMap.begin(); it != _componentMap.end(); ++it)
		{
			it->second.clear();
		}
	}

	// retrieve an immutable list of the components. components can be mutated.
	// this function creates a copy so store the return results.
	template<class T>
	const std::vector<T*> GetComponents()
	{
		// gross but no other way is working.
		auto type = std::type_index(typeid(T));
		std::vector<T*> casted(_componentMap[type].size());
		for (int i = 0; i < _componentMap[type].size(); i++)
			casted[i] = static_cast<T*>(_componentMap[type][i]);
		return casted;
	}

protected:
	//virtual void onComponentCreated(std::type_index t, Component* c) = 0;
	//virtual void onComponentDestroyed(std::type_index t, Component* c) = 0;
	ComponentMap _componentMap;


#pragma region Concurrency 
// use these variables if your system can be ran in the background 

public:
	// WARNING: use for concurrent systems only.
	// update function for concurrent systems 
	void notifyUpdate(float dt);

	// WARNING: use for concurrent systems only. 
	// starts the update loop that waits for a notification from core game engine.
	void startLooping();

	// WARNING: use for concurrent systems only.
	// notify the system to stop looping. call this before disposing system.
	void stopLooping();

private:
	std::mutex _mtx;					// concurrency mutex
	std::condition_variable _cv;		// concurrency condition variable (used to wait for game engine)
	std::atomic<bool> _ready = true;	// if frame is ready for update 
	std::thread* _thread;				// concurrency thread 
	bool _alive = true;					// whether to run thread or not
	float _deltaTime;					// stored delta time for concurrency updates 

	// use this function to create the thread. 
	// continually calls update() while waiting for game engine updates. 
	void concurrentLoop();

#pragma endregion

};

/* Tentative: 

class System 
{
	map < T , ComponentList<T> > 

	void addListener<T>
	{
		map[ t ] = new ComponentList < T> 
	}

	vector<T> getComponents<T>
	{
		return map [ t] . getList()
	}
}

class ComponentList < T >
{
	vector<T> list;

	ctor
	{
		Game::instance()->addlistener()
	}

	dtor
	{
		Game::instance()->removelistener()
	}

	void onComponentCreated(Component)
	{
		if (typeid(component) == typeid(T))
			list.add( cast <T> component);
	}

	void onComponentDestroyed()
	{
		if (typeid(component) == typeid(T))
			list.remove( component );
	}

	vector<T> * getList() 
	{
		return list; 
	}

	iterator<T> getValidComponents()
	{
		iterate thru list 
			ensure component is active
			ensure parent exists
			ensure entity is active (not dangling too)
			return value
	}
}

class derived system 
{
	ctor 
	{
		addlistener < T1 > ()
		addlistener < T2 > ()
		addlistener < T3 > ()
	}

	update 
	{
		foreach T1 in getComponents <T1>
			// ...
		foreach T2 in getComponents <T2>
			// ...
		foreach T3 in getComponents <T3>
			// ...
	}
}
*/