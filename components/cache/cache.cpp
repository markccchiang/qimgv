#include "cache.h"

 Cache::Cache() {
    sem = new QSemaphore(1);
}

void Cache::lock() {
    sem->acquire();
}

void Cache::unlock() {
    sem->release();
}

bool Cache::contains(QString name) {
    return images.contains(name);
}

bool Cache::insert(QString name, std::shared_ptr<Image> img) {
    if(images.contains(name)) {
        return false;
    } else {
        images.insert(name, new CacheItem(img));
        return true;
    }
}

void Cache::remove(QString name) {
    if(images.contains(name)) {
        images[name]->lock();
        auto *item = images.take(name);
        delete item;
    }
}

void Cache::clear() {
    for(auto name : images.keys()) {
        images[name]->lock();
        auto item = images.take(name);
        delete item;
    }
}

std::shared_ptr<Image> Cache::get(QString name) {
    if(images.contains(name)) {
        CacheItem *item = images.value(name);
        return item->getContents();
    } else {
        qDebug() << "Cache::get() - !!! returning nullptr for " << name << ". There is a bug in logic somewhere!";
        return nullptr;
    }
}

bool Cache::reserve(QString name) {
    if(images.contains(name)) {
        images[name]->lock();
        return true;
    } else {
        return false;
    }
}

bool Cache::release(QString name) {
    if(images.contains(name)) {
        images[name]->unlock();
        return true;
    } else {
        return false;
    }
}

// removes all items except the ones in list
void Cache::trimTo(QStringList nameList) {
    for(auto name : images.keys()) {
        if(!nameList.contains(name)) {
            //qDebug() << "CACHE-RM: locking.. " << name;
            images[name]->lock();
            //qDebug() << "CACHE-RM: LOCKED: " << name;
            auto *item = images.take(name);
            delete item;
        }
    }
}

const QList<QString> Cache::keys() {
    return images.keys();
}
