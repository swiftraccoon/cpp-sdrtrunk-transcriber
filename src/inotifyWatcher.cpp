// InotifyWatcher.cpp
#include "inotifyWatcher.h"
#include <iostream>
#include <limits.h>
#include <stdexcept>

InotifyWatcher::InotifyWatcher(const std::string& directoryToMonitor, EventCallback callback)
    : inotifyFd(-1), watchDescriptor(-1), directory(directoryToMonitor), eventCallback(callback), running(false) {
    inotifyFd = inotify_init();
    if (inotifyFd == -1) {
        throw std::runtime_error("Failed to initialize inotify");
    }

    watchDescriptor = inotify_add_watch(inotifyFd, directory.c_str(), IN_CREATE);
    if (watchDescriptor == -1) {
        close(inotifyFd);
        throw std::runtime_error("Failed to add watch to directory");
    }
}

InotifyWatcher::~InotifyWatcher() {
    stopWatching();
    if (watchDescriptor != -1) {
        inotify_rm_watch(inotifyFd, watchDescriptor);
    }
    if (inotifyFd != -1) {
        close(inotifyFd);
    }
}

void InotifyWatcher::startWatching() {
    running = true;
    processEvents();
}

void InotifyWatcher::stopWatching() {
    running = false;
}

void InotifyWatcher::processEvents() {
    char buffer[4096]
        __attribute__((aligned(__alignof__(struct inotify_event))));
    ssize_t length;

    while (running) {
        length = read(inotifyFd, buffer, sizeof(buffer));
        if (length < 0 && errno != EAGAIN) {
            throw std::runtime_error("Read error on inotify file descriptor");
        }

        for (char* ptr = buffer; ptr < buffer + length; ) {
            auto* event = reinterpret_cast<const struct inotify_event*>(ptr);
            eventCallback(event);
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
}
