#include "System.h"
#include "../Core/Game.h"


System::System()
{
	//Game::instance().componentCreated.connect(boost::bind(&System::onComponentCreated, this, _1, _2));
	//Game::instance().componentDestroyed.connect(boost::bind(&System::onComponentDestroyed, this, _1, _2));
}

System::System(std::vector<std::type_index> forComponents)
{
	for (auto const& type : forComponents)
	{
		_componentMap.emplace(type, std::vector<Component*>());
	}
}

System::~System()
{
	// b/c binding is used signal is automatically disconnected when this obj is destroyed.
}



#pragma region Concurrency

void System::notifyUpdate(float dt)
{
	std::lock_guard<std::mutex> lock(_mtx);
	_deltaTime = dt;
	_ready = true;
	_cv.notify_one();
}

void System::startLooping()
{
	_thread = new std::thread(&System::concurrentLoop, this);
}

void System::stopLooping()
{
	_alive = false;
}

void System::concurrentLoop()
{
	while (_alive)
	{
		// wait (for next frame).  
		std::unique_lock<std::mutex> lck(_mtx);
		if (!_ready)
			_cv.wait(lck);
		else 
			lck.unlock();
		// update 
		update(_deltaTime);
		clearComponents();
		_ready = false;
		Game::instance().notifySystemFinish();
	}
}

#pragma endregion