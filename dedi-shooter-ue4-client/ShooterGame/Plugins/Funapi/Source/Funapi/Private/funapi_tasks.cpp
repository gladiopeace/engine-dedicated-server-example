// Copyright (C) 2013-2018 iFunFactory Inc. All Rights Reserved.
//
// This work is confidential and proprietary to iFunFactory Inc. and
// must not be used, disclosed, copied, or distributed without the prior
// consent of iFunFactory Inc.

#include "funapi_tasks.h"

#ifdef FUNAPI_UE4
#include "FunapiPrivatePCH.h"
#endif

#include "funapi_utils.h"
#include "funapi_session.h"
#include "funapi_socket.h"
#include "funapi_announcement.h"
#include "funapi_downloader.h"

namespace fun {

////////////////////////////////////////////////////////////////////////////////
// FunapiTasksImpl implementation.

class FunapiTasksImpl : public std::enable_shared_from_this<FunapiTasksImpl> {
 public:
  typedef FunapiTasks::TaskHandler TaskHandler;

  FunapiTasksImpl();
  virtual ~FunapiTasksImpl();

  void Update();
  virtual void Push(const TaskHandler &task);
  virtual int Size();

 protected:
  fun::queue<std::shared_ptr<std::function<bool()>>> queue_;
  std::mutex mutex_;
};


FunapiTasksImpl::FunapiTasksImpl() {
}


FunapiTasksImpl::~FunapiTasksImpl() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


void FunapiTasksImpl::Update()
{
  std::shared_ptr<std::function<bool()>> task = nullptr;
  fun::queue<std::shared_ptr<std::function<bool()>>> update_queue;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.swap(update_queue);
  }

  while (true)
  {
    {
      if (update_queue.empty())
      {
        break;
      }
      else
      {
        task = update_queue.front();
        update_queue.pop();
      }
    }

    if (task)
    {
      if ((*task)() == false) break;
    }
    else
    {
      break;
    }
  }
}


void FunapiTasksImpl::Push(const TaskHandler &task)
{
  std::unique_lock<std::mutex> lock(mutex_);

  if (task) {
    auto h = std::make_shared<TaskHandler>(task);
    queue_.push(h);
  }
}


int FunapiTasksImpl::Size()
{
  std::unique_lock<std::mutex> lock(mutex_);
  return static_cast<int>(queue_.size());
}


////////////////////////////////////////////////////////////////////////////////
// FunapiTask implementation.

FunapiTasks::FunapiTasks() : impl_(std::make_shared<FunapiTasksImpl>()) {
}


FunapiTasks::~FunapiTasks() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


std::shared_ptr<FunapiTasks> FunapiTasks::Create() {
  return std::make_shared<FunapiTasks>();
}


void FunapiTasks::Push(const TaskHandler &task) {
  impl_->Push(task);
}


int FunapiTasks::Size() {
  return impl_->Size();
}


void FunapiTasks::Update() {
  impl_->Update();
}


void FunapiTasks::UpdateAll() {
  FunapiSession::UpdateAll();
  FunapiAnnouncement::UpdateAll();
  FunapiHttpDownloader::UpdateAll();
}


////////////////////////////////////////////////////////////////////////////////
// FunapiThreadImpl implementation.

class FunapiThreadImpl : public FunapiTasksImpl {
 public:
  FunapiThreadImpl() = delete;
  FunapiThreadImpl(const fun::string &thread_id);
  virtual ~FunapiThreadImpl();

  void Push(const TaskHandler &task);
  void Join();

  fun::string GetThreadId();

 private:
  void Initialize();
  void Finalize();
  void JoinThread();
  void Thread();

  std::thread thread_;
  bool run_ = false;
  std::condition_variable_any condition_;
  fun::string thread_id_;
};


FunapiThreadImpl::FunapiThreadImpl(const fun::string &thread_id) : thread_id_(thread_id) {
  Initialize();
}


FunapiThreadImpl::~FunapiThreadImpl() {
  Finalize();
  // DebugUtils::Log("%s", __FUNCTION__);
}


void FunapiThreadImpl::Initialize() {
  run_ = true;
  thread_ = std::thread([this]{ Thread(); });
}


void FunapiThreadImpl::Finalize() {
  JoinThread();
}


void FunapiThreadImpl::Push(const TaskHandler &task)
{
  FunapiTasksImpl::Push(task);
  condition_.notify_one();
}


void FunapiThreadImpl::JoinThread() {
  run_ = false;
#ifndef FUNAPI_UE4_PLATFORM_WINDOWS
  condition_.notify_all();
#endif
  if (thread_.joinable())
    thread_.join();
}


void FunapiThreadImpl::Thread() {
  bool is_network = false;
  if (thread_id_.compare("_network") == 0)
  {
    is_network = true;
  }

  while (run_)
  {
    if (is_network)
    {
      FunapiSocket::Poll();
    }
    else
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (queue_.empty()) {
        condition_.wait(mutex_);
      }
    }

    Update();
  }
}


void FunapiThreadImpl::Join() {
  JoinThread();
}


fun::string FunapiThreadImpl::GetThreadId() {
  return thread_id_;
}


////////////////////////////////////////////////////////////////////////////////
// FunapiThread implementation.

FunapiThread::FunapiThread(const fun::string &thread_id)
: impl_(std::make_shared<FunapiThreadImpl>(thread_id)) {
}


FunapiThread::~FunapiThread() {
  // DebugUtils::Log("%s", __FUNCTION__);
}


std::shared_ptr<FunapiThread> FunapiThread::Create(const fun::string &thread_id) {
  if (auto t = FunapiThread::Get(thread_id)) {
    return t;
  }

  return nullptr;
}


void FunapiThread::Push(const TaskHandler &task) {
  impl_->Push(task);
}


int FunapiThread::Size() {
  return impl_->Size();
}


void FunapiThread::Join() {
  impl_->Join();
}


std::shared_ptr<FunapiThread> FunapiThread::Get(const fun::string &thread_id) {
  static fun::unordered_map<fun::string, std::shared_ptr<FunapiThread>> map_threads;
  static std::mutex threads_mutex;

  std::unique_lock<std::mutex> lock(threads_mutex);

  auto it = map_threads.find(thread_id);
  if (it != map_threads.end()) {
    return it->second;
  }
  else {
    if (auto t = std::make_shared<FunapiThread>(thread_id)) {
      map_threads[thread_id] = t;
      return t;
    }
  }

  return nullptr;
}

}  // namespace fun
