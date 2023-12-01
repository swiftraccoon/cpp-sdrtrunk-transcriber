// InotifyWatcher.h
#ifndef INOTIFYWATCHER_H
#define INOTIFYWATCHER_H

#include <string>
#include <functional>
#include <sys/inotify.h>
#include <unistd.h>
#include <vector>

class InotifyWatcher {
public:
    using EventCallback = std::function<void(const struct inotify_event*)>;

    InotifyWatcher(const std::string& directoryToMonitor, EventCallback callback);
    ~InotifyWatcher();

    void startWatching();
    void stopWatching();

private:
    int inotifyFd;
    int watchDescriptor;
    std::string directory;
    EventCallback eventCallback;
    bool running;

    void processEvents();
};

#endif // INOTIFYWATCHER_H
