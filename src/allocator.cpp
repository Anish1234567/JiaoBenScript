#include <cassert>
#include <vector>

#include "allocator.h"


void Allocator::destroy(JBObject *obj) {
    assert(obj != nullptr);
    auto it = this->objects.find(obj);
    assert(it != this->objects.end());
    this->objects.erase(it);
    delete obj;
}


void Allocator::each_object(std::function<void(JBObject &obj)> callback) {
    for (JBObject *obj : this->objects) {
        callback(*obj);
    }
}


Allocator::~Allocator() {
    std::vector<JBObject *> to_remove;
    this->each_object([&](JBObject &obj) { to_remove.push_back(&obj); });
    for (JBObject *obj : to_remove) {
        this->destroy(obj);
    }
}
