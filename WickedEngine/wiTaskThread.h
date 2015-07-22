#pragma once

#include "CommonInclude.h"

class wiTaskThread {
public:
	wiTaskThread(std::function<void()> task)
      : m_task(std::move(task)),
        m_wakeup(false),
        m_stop(false),
		m_thread(&wiTaskThread::taskFunc, this)
    {}
	~wiTaskThread() { stop(); join(); }

    void wakeup() {
        auto lock = std::unique_lock<std::mutex>(m_wakemutex);
        m_wakeup = true;
        m_wakecond.notify_one();
    }
    void wait() {
        auto lock = std::unique_lock<std::mutex>(m_waitmutex);
        while (m_wakeup)
          m_waitcond.wait(lock);
    }

    void stop() {
        auto lock = std::unique_lock<std::mutex>(m_wakemutex);
        m_stop = true;
        m_wakecond.notify_one();
    }
    void join() {
        m_thread.join();
    }

private:
    std::function<void ()> m_task;

    bool m_wakeup;
    bool m_stop;
    std::mutex m_wakemutex;
    std::condition_variable m_wakecond;

    std::mutex m_waitmutex;
    std::condition_variable m_waitcond;

    std::thread m_thread;

    void taskFunc() {
        while (true) {
            {
                auto lock = std::unique_lock<std::mutex>(m_wakemutex);
                while (!m_wakeup && !m_stop)
                  m_wakecond.wait(lock);
                if (m_stop) {
                  return;
                }
            }

            try { m_task(); } catch (...) {}

            {
                auto lock = std::unique_lock<std::mutex>(m_waitmutex);
                m_wakeup = false;
                m_waitcond.notify_all();
            }
        }
    }

};