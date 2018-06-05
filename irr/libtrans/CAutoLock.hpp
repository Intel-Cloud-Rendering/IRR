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

#ifndef CAUTOLOCK_HPP
#define CAUTOLOCK_HPP

#include <iostream>
#include <pthread.h>

class CAutoLock {
public:
    CAutoLock(pthread_mutex_t *pMutex) : m_pMutex(pMutex) {
        if (pthread_mutex_lock(m_pMutex) < 0)
            std::cerr << "Fail to call pthread_mutex_lock()" << std::endl;
    }

    ~CAutoLock() {
        if (m_pMutex) {
            if (pthread_mutex_unlock(m_pMutex) < 0)
                std::cerr << "Pthread_mutex_unlock() failed." << std::endl;
        }
    }

private:
    pthread_mutex_t *m_pMutex;
};

#endif /* CAUTOLOCK_HPP */

