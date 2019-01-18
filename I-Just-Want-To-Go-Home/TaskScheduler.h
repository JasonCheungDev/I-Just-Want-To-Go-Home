#pragma once
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include "CircularBuffer.h"
#include "Task.h"

class TaskScheduler
{
public:
	static TaskScheduler& instance();
	TaskScheduler();
	~TaskScheduler();
	std::queue<Task*> tasks;
	std::vector<std::thread> threads;
	// CircularBuffer<std::thread> threads;

	std::mutex mtx;
	std::condition_variable cv;

	void ScheduleTask(Task* task);

	Task* Retrieve();

	void ProcessTask();
};

