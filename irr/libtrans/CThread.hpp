/*
 * Copyright (C) 2017 Intel
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef CTHREAD_HPP
#define CTHREAD_HPP

#include "CRunable.h"
#include "CAutoLock.hpp"
#include "CTransLog.h"

/**
 * A simple pthread model. Use it by following below steps:
 * 1. Create a class which inherits CRunable;
 * 2. Create a CThread instance by passing a CRunable instance:
 *    CThread thread(this);
 * 3. thread.start();
 * 4. thread.stop();
 */
class CThread : private CTransLog {
public:
    CThread(CRunable *obj) : CTransLog(__func__) {
        m_pObj  = obj;
        m_bExit = false;
        m_pid   = -1;
        pthread_mutex_init(&m_Mutex, nullptr);
    }

    ~CThread() {
        stop();
        if (pthread_mutex_destroy(&m_Mutex) < 0)
            Error("Fail to call pthread_mutex_destroy()\n");
    }

    int start() {
        if (m_pObj && !status())
            return pthread_create(&m_pid, nullptr, run, this);

        return 0;
    }

    bool status() {
        bool running = !(pthread_mutex_trylock(&m_Mutex) == 0);
        if (!running) {
            if (pthread_mutex_unlock(&m_Mutex) < 0)
                return true;
        }

        return running;
    }

    void wait() {
        CAutoLock lock(&m_Mutex);
    }

    void stop() {
        if (status()) {
            m_bExit = true;
            if (m_pid)
                pthread_join(m_pid, nullptr);
        }
    }

private:
    pthread_t       m_pid;
    CRunable       *m_pObj;
    bool            m_bExit;
    pthread_mutex_t m_Mutex;

private:
    /**
     * Disable empth&copy constructor.
     */
    CThread() = delete;
    CThread(const CThread &) = delete;
    static void *run(void *param) {
        CThread *pThis = static_cast<CThread *>(param);
        CAutoLock lock(&pThis->m_Mutex);
        pThis->Info("Starting from CThread. PID[%#x]\n", pthread_self());

        do {
            pThis->m_pObj->run();
        } while (!pThis->m_bExit && !pThis->m_pObj->interrupt_callback());

        pthread_exit(nullptr);
    }
};

#endif /* CTHREAD_HPP */
