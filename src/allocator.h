#ifndef JIAOBENSCRIPT_ALLOCATOR_H
#define JIAOBENSCRIPT_ALLOCATOR_H

#include <functional>
#include <unordered_set>
#include <utility>

#include "jbobject.h"


class Allocator {
public:
    Allocator() {}
    Allocator(const Allocator &) = delete;
    Allocator &operator=(const Allocator &) = delete;

    // http://eli.thegreenplace.net/2014/variadic-templates-in-c/
    template<class T, class ...Args>
    T *construct(Args &&...args) {
        T *obj = new T(std::forward<Args>(args)...);
        this->objects.insert(obj);
        return obj;
    }

    void destroy(JBObject *obj);
    void each_object(std::function<void(JBObject &)> callback);
    ~Allocator();

private:
    std::unordered_set<JBObject *> objects;
};


#endif //JIAOBENSCRIPT_ALLOCATOR_H
