#include "TaskScheduler.h"

//
//TaskScheduler::TaskScheduler()
//{
//}
//
//
//TaskScheduler::~TaskScheduler()
//{
//}

TaskScheduler& TaskScheduler::instance()
{
	static TaskScheduler singleton;
	return singleton;
}

TaskScheduler::TaskScheduler()
{
	unsigned concurentThreadsSupported = std::thread::hardware_concurrency();
	threads.reserve(concurentThreadsSupported);
	for (int i = 0; i < 1; /*concurentThreadsSupported;*/ i++)
	{
		threads.push_back(std::thread(&TaskScheduler::ProcessTask, this));
		// threads.emplace_back(std::bind(ProcessTask));
	}
	//std::vector<std::thread> threads;
}

TaskScheduler::~TaskScheduler()
{
}

void TaskScheduler::ScheduleTask(Task * task)
{
	// LOCK
	std::unique_lock<std::mutex> lock(mtx);
	std::cout << "+++Added task" << std::endl;
	tasks.push(task);
	cv.notify_one();
	// UNLOCK
}

Task * TaskScheduler::Retrieve()
{
	{
		// LOCK
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock);	// UNLOCK

		//if (tasks.empty())
		//{
		//	std::cout << "Waiting for tasks" << std::endl;
		//	// LOCK 
		//	std::cout << "---Found a task!" << std::endl;
		//}
		//else
		//{
		//	std::cout << "---Task was present" << std::endl;
		//}

		// SHOULD BE LOCKED 
		auto s = tasks.size();
		auto t = tasks.front();
		tasks.pop();
		return t;
	}
}

void TaskScheduler::ProcessTask()
{
	while (true)
	{
		auto t = Retrieve();
		t->Execute();
	}
}
