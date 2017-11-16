#pragma once

#include "android/base/threads/Thread.h"

#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/ConditionVariable.h"

#include <memory>
#include <list>


using Lock = android::base::Lock;
using AutoLock = android::base::AutoLock;
using ConditionVariable = android::base::ConditionVariable;

namespace android {
namespace opengl {

class OpenGLESHostDataHandler : public android::base::Thread {
public:
    OpenGLESHostDataHandler() : mIsWorking(false) {};

    virtual intptr_t main() override {
        printf("datahandler thread begins\n");
        while (mIsWorking) {

            AutoLock lock(mQueueLock);
            while (mHandlerQueue.size() == 0) {
                mDatatReady.wait(&mQueueLock);
            }

            if (!mIsWorking) {
                break;
                }

            std::function<void()> func = mHandlerQueue.front();
            mHandlerQueue.pop_front();

            lock.unlock();
            
            (func)();
        }

        printf("datahandler thread exit\n");
        return 0;
    }

    void StartHandler() {
        mIsWorking = true;
        start();
    }

    void StopHandler() {
        mIsWorking = false;
        wait();
    }

    void NotifyDataReady() {
        mDatatReady.signal();
    }

    void PushBack(std::function<void()> func) {
        AutoLock lock(mQueueLock);
        mHandlerQueue.push_back(func);
        mDatatReady.signalAndUnlock(&lock);
    }
    
private:
    bool mIsWorking;
    Lock mQueueLock;
    Lock mLock;
    ConditionVariable mDatatReady;
    std::list<std::function<void()> > mHandlerQueue;
};

}}
