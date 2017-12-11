#pragma once

#include "android/opengl/OpenGLESHostConnection.h"

#include "android/base/threads/Thread.h"

#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/ConditionVariable.h"

#include <unistd.h>

#include <memory>
#include <unordered_map>

#include <list>
#include <sys/epoll.h>

using Lock = android::base::Lock;
using AutoLock = android::base::AutoLock;
using ConditionVariable = android::base::ConditionVariable;

#define MAX_HANDLE_FD_SIZE 100
namespace android {
namespace opengl {

class OpenGLESHostDataHandler : public android::base::Thread {
public:
    OpenGLESHostDataHandler() :
        mEpollFD(-1),
        mIsWorking(false)
         {};

    virtual intptr_t main() override {
        struct epoll_event events[MAX_HANDLE_FD_SIZE];
        
        printf("datahandler thread begins\n");
        while (mIsWorking) {
            //printf("connection count = %d-------------------\n", (int)mConnectionList.size());
            int ret = ::epoll_wait(mEpollFD, events, MAX_HANDLE_FD_SIZE, 100);

            for (int i = 0; i < ret; i ++) {
                if ((events[i].events & EPOLLERR) ||  
                      (events[i].events & EPOLLHUP) ||
                      (events[i].events & EPOLLRDHUP)) {  
                      //printf("found a connection exited\n");  
                      removeConnection(events[i].data.fd);
                      continue;  
                } else if (events[i].events & EPOLLIN) {
                    AutoLock lock(mListLock);
                    auto it =
                        mConnectionList.find(events[i].data.fd);
                    if (it == mConnectionList.end())
                        continue;
                    
                    std::shared_ptr<OpenGLESHostServerConnection> connection = it->second;
                    
                    lock.unlock();
                    
                    if (!connection->onNetworkDataReady()) {
                        removeConnection(events[i].data.fd);
                        //printf("connection count = %d-------------------\n", (int)mConnectionList.size());
                    }
                } else if (events[i].events & EPOLLOUT) {
                    AutoLock lock(mListLock);
                    auto it =
                        mConnectionList.find(events[i].data.fd);
                    if (it == mConnectionList.end())
                        continue;
                    
                    std::shared_ptr<OpenGLESHostServerConnection> connection = it->second;
                    
                    lock.unlock();
                    
                    if (!connection->onRenderChannelDataReady()) {
                        removeConnection(events[i].data.fd);
                    }
                }
            }

            
        }

        printf("datahandler thread exit\n");
        return 0;
    }

    void StartHandler() {
        mIsWorking = true;

        mEpollFD = ::epoll_create(MAX_HANDLE_FD_SIZE);
        
        start();
    }

    void StopHandler() {
        mIsWorking = false;
        wait();

        if (mEpollFD > 0)
            ::close(mEpollFD);
    }

    void NotifyDataReady() {
        mDatatReady.signal();
    }

    void PushBack(std::function<void()> func) {
        AutoLock lock(mQueueLock);
        mHandlerQueue.push_back(func);
        mDatatReady.signalAndUnlock(&lock);
    }

    void removeConnection(int fd) {
        ::epoll_ctl(mEpollFD, EPOLL_CTL_DEL, fd, NULL);
        
        AutoLock lock(mListLock);
        mConnectionList.erase(fd);
    }

    void addConnection(int socket) {
        std::shared_ptr<OpenGLESHostServerConnection> connection = 
                std::make_shared<OpenGLESHostServerConnection>(socket, this);
        
        AutoLock lock(mListLock);
        mConnectionList.insert(std::make_pair(socket, connection));
        lock.unlock();
        
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        ev.data.fd = socket;
        ::epoll_ctl(mEpollFD, EPOLL_CTL_ADD, socket, &ev);
    }

    void modConnection(int fd, bool askRead) {
        if (fd <= 0)
            return;
        
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        if (askRead)
            ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        else
            ev.events = EPOLLOUT | EPOLLRDHUP;

        ::epoll_ctl(mEpollFD, EPOLL_CTL_MOD, fd, &ev);
    }
    
private:
    int mEpollFD;
    
    bool mIsWorking;

    Lock mQueueLock;
    ConditionVariable mDatatReady;
    std::list<std::function<void()> > mHandlerQueue;

    std::unordered_map<int, std::shared_ptr<OpenGLESHostServerConnection> > mConnectionList;
    Lock mListLock;
};

}}
