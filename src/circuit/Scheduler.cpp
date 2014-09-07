/*
 * Scheduler.cpp
 *
 *  Created on: Aug 28, 2014
 *      Author: rlcevg
 */

#include "Scheduler.h"

namespace circuit {

CMultiQueue<CScheduler::WorkTask> CScheduler::workTasks;
std::thread CScheduler::workerThread;
std::atomic<bool> CScheduler::workerRunning(false);
unsigned int CScheduler::counterInstance = 0;

CScheduler::CScheduler()
{
	counterInstance++;
}

CScheduler::~CScheduler()
{
	counterInstance--;
	Release();
}

void CScheduler::Init(const std::shared_ptr<CScheduler>& this_ptr)
{
	self = this_ptr;
}

void CScheduler::Release()
{
	std::weak_ptr<CScheduler>& scheduler = self;
	CMultiQueue<WorkTask>::ConditionFunction condition = [&scheduler](WorkTask& item) -> bool {
		return !scheduler.owner_before(item.scheduler) && !item.scheduler.owner_before(scheduler);
	};
	workTasks.RemoveAllIf(condition);

	if (counterInstance == 0 && workerRunning.load()) {
		workerRunning = false;
		// At this point workTasks is empty. Push empty task in case worker stuck at Pop().
		workTasks.Push({self, nullptr, nullptr});
		if (workerThread.joinable()) {
			workerThread.join();
		}
	}
}

void CScheduler::RunTaskAt(std::shared_ptr<CGameTask> task, int frame)
{
	onceTasks.push_back({task, frame});
}

void CScheduler::RunTaskEvery(std::shared_ptr<CGameTask> task, int frameInterval)
{
	repeatTasks.push_back({task, frameInterval, -frameInterval});
}

void CScheduler::ProcessTasks(int frame)
{
	std::list<OnceTask>::iterator ionce = onceTasks.begin();
	while (ionce != onceTasks.end()) {
		if (ionce->frame <= frame) {
			ionce->task->Run();
			ionce = onceTasks.erase(ionce);  // alternatively, onceTasks.erase(iter++);
		} else {
			++ionce;
		}
	}

	std::list<RepeatTask>::iterator irepeat = repeatTasks.begin();
	while (irepeat != repeatTasks.end()) {
		if (frame - irepeat->lastFrame >= irepeat->frameInterval) {
			if (irepeat->task->GetTerminate()) {
				irepeat = repeatTasks.erase(irepeat);
			} else {
				irepeat->task->Run();
				irepeat->lastFrame = frame;
			}
		} else {
			++irepeat;
		}
	}

	CMultiQueue<FinishTask>::ProcessFunction process = [](FinishTask& item) {
		item.task->Run();
	};
	finishTasks.PopAndProcess(process);
}

void CScheduler::RunParallelTask(std::shared_ptr<CGameTask> task, std::shared_ptr<CGameTask> onComplete)
{
	if (!workerRunning.load()) {
		workerRunning = true;
		// TODO: Find out more about std::async, std::bind, std::future.
		workerThread = std::thread(&CScheduler::WorkerThread);
	}
	workTasks.Push({self, task, onComplete});
}

void CScheduler::RemoveTask(std::shared_ptr<CGameTask> task)
{
	// Task must not remove any task during execution, use task->SetTerminate(true);
	onceTasks.remove({task, 0});
	repeatTasks.remove({task, 0, 0});
}

void CScheduler::WorkerThread()
{
	WorkTask container = workTasks.Pop();
	while (workerRunning.load()) {
		container.task->Run();
		container.task = nullptr;
		if (container.onComplete != nullptr) {
			std::shared_ptr<CScheduler> scheduler = container.scheduler.lock();
			if (scheduler) {
				scheduler->finishTasks.Push(container);
			}
			container.onComplete = nullptr;
		}
		container = workTasks.Pop();
	}
}

} // namespace circuit
