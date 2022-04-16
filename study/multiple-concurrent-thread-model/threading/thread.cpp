#include "thread.h"
#include "thread_util.h"

#include "logging/logging.h"
#include "message_loop/run_loop.h"

namespace
{
    void ThreadFunc(void* params)
    {
        mctm::StdThreadDelegate* thread = static_cast<mctm::StdThreadDelegate*>(params);
        thread->ThreadMain();
    }

    void ThreadQuitHelper()
    {
        mctm::MessageLoop::current()->Quit();
    }
}

namespace mctm
{
    std::unique_ptr<mctm::Thread> Thread::AttachCurrentThread(const char* thread_name, MessageLoop::Type type)
    {
        if (!MessageLoop::current())
        {
            std::unique_ptr<mctm::Thread> thread = std::make_unique<mctm::Thread>(thread_name);
            SetThreadName(::GetCurrentThreadId(), thread_name);
            auto message_loop = std::make_unique<MessageLoop>(type);
            thread->set_message_loop(message_loop.release());
            return std::move(thread);
        }
        return nullptr;
    }

    Thread::Thread(const std::string& thread_name)
        : thread_name_(thread_name)
    {
    }

    Thread::~Thread()
    {
        Stop();
    }

    bool Thread::Start()
    {
        Options options;
        return StartWithOptions(options);
    }

    bool Thread::StartWithOptions(const Options& options)
    {
        startup_data_.options = options;
        thread_ = std::thread(ThreadFunc, this);
        if (thread_.joinable())
        {
            startup_data_.wait_for_run_event.Wait();
            started_ = true;
        }

        return thread_.joinable();
    }
    
    void Thread::Stop()
    {
        if (!started_)
        {
            return;
        }

        // ���һ�µ�ǰ����Ϣѭ���ǲ���Ƕ���˶�㣬�����Ƕ�˶�����ôThread::Stop���˲���ȫ��Ƕ�׵ģ�ֻ���˵�ǰ�ģ�
        // ���֧��ǿ���˳�Ƕ��ѭ�������ѭ�����������̴���һ���������⣬�ɴ�Ͳ�֧���ˣ�
        // ��Ҫ���ŵ��˳��߳���ȷ������Ӧ����Stopǰ��֪ͨ������Ϣѭ�������Լ����߼�����ҵ��Ȼ�������˳����Դ����ϲ�Ƕ�׵ݹ飻
        // ����ֱ�Ӽ�⵽�����ڶ��ѭ��Ƕ�׾Ͳ�֧��ʵ�ʵ�Stop
        if (message_loop_)
        {
            DCHECK(!message_loop_->IsNested());

            message_loop_->PostTask(FROM_HERE, Bind(ThreadQuitHelper));
        }

        if (thread_.joinable())
        {
            thread_.join();
        }

        message_loop_ = nullptr;
        started_ = false;
    }

    void Thread::set_message_loop(MessageLoop* message_loop)
    {
        message_loop_.reset(message_loop);
    }

    void Thread::ThreadMain()
    {
        SetThreadName(::GetCurrentThreadId(), thread_name_.c_str());

        std::unique_ptr<ScopedCOMInitializer> com;
        if (startup_data_.options.com != NONE)
        {
            com = std::make_unique<ScopedCOMInitializer>(
                startup_data_.options.com == STA ? COINIT_APARTMENTTHREADED : COINIT_MULTITHREADED);
        }

        message_loop_ = std::make_unique<MessageLoop>(startup_data_.options.type);

        Init();
        
        startup_data_.wait_for_run_event.Signal();

        Run();

        CleanUp();

        message_loop_ = nullptr;
    }

    void Thread::Init()
    {
    }

    void Thread::Run()
    {
        RunLoop run_loop;
        run_loop.Run();
    }

    void Thread::CleanUp()
    {
    }

}