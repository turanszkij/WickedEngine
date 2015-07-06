#include "CommonInclude.h"

class TaskThread {
public:
    TaskThread(std::function<void ()> task)
      : m_task(std::move(task)),
        m_wakeup(false),
        m_stop(false),
        m_thread(&TaskThread::taskFunc, this)
    {}
    ~TaskThread() { stop(); join(); }

    // wake up the thread and execute the task
    void wakeup() {
        auto lock = std::unique_lock<std::mutex>(m_wakemutex);
        //std::cout << "main: sending wakeup signal..." << std::endl;
        m_wakeup = true;
        m_wakecond.notify_one();
    }
    // wait for the task to complete
    void wait() {
        auto lock = std::unique_lock<std::mutex>(m_waitmutex);
        //std::cout << "main: waiting for task completion..." << std::endl;
        while (m_wakeup)
          m_waitcond.wait(lock);
        //std::cout << "main: task completed!" << std::endl;
    }

    // ask the thread to stop
    void stop() {
        auto lock = std::unique_lock<std::mutex>(m_wakemutex);
        //std::cout << "main: sending stop signal..." << std::endl;
        m_stop = true;
        m_wakecond.notify_one();
    }
    // wait for the thread to actually be stopped
    void join() {
        //std::cout << "main: waiting for join..." << std::endl;
        m_thread.join();
        //std::cout << "main: joined!" << std::endl;
    }

private:
    std::function<void ()> m_task;

    // wake up the thread
    bool m_wakeup;
    bool m_stop;
    std::mutex m_wakemutex;
    std::condition_variable m_wakecond;

    // wait for the thread to finish its task
    std::mutex m_waitmutex;
    std::condition_variable m_waitcond;

    std::thread m_thread;

    void taskFunc() {
        while (true) {
            {
                auto lock = std::unique_lock<std::mutex>(m_wakemutex);
                //std::cout << "thread: waiting for wakeup or stop signal..." << std::endl;
                while (!m_wakeup && !m_stop)
                  m_wakecond.wait(lock);
                if (m_stop) {
                  //std::cout << "thread: got stop signal!" << std::endl;
                  return;
                }
                //std::cout << "thread: got wakeup signal!" << std::endl;
            }

            //std::cout << "thread: running the task..." << std::endl;
            // you should probably do something cleaner than catch (...)
            // just ensure that no exception propagates from m_task() to taskFunc()
            try { m_task(); } catch (...) {}
            //std::cout << "thread: task completed!" << std::endl;

            {
                auto lock = std::unique_lock<std::mutex>(m_waitmutex);
                //std::cout << "thread: sending task completed signal..." << std::endl;
                m_wakeup = false;
                m_waitcond.notify_all();
            }
        }
    }

};