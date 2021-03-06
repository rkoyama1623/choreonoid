/**
   @author Shin'ichiro Nakaoka
*/

#ifndef CNOID_UTIL_REFERENCED_H
#define CNOID_UTIL_REFERENCED_H

#include <boost/version.hpp>

#ifdef WIN32
#include <memory>
#endif

#if (BOOST_VERSION >= 105300) && !defined(NOT_USE_BOOST_ATOMIC)
#include <boost/atomic.hpp>
#define CNOID_REFERENCED_USE_ATOMIC_COUNTER
#endif

#include <cassert>
#include <iosfwd>

namespace cnoid {

class Referenced;

/**
   \todo Make this thread safe
*/
class WeakCounter
{
public:
    WeakCounter(){
        isObjectAlive_ = true;
        weakCount = 0;
    }

    void add() { ++weakCount; }

    void release() {
        if(--weakCount == 0){
            if(!isObjectAlive_){
                delete this;
            }
        }
    }

    void setDestructed() {
        isObjectAlive_ = false;
        if(weakCount == 0){
            delete this;
        }
    }

    bool isObjectAlive(){
        return isObjectAlive_;
    }

private:
    bool isObjectAlive_;
    int weakCount;
};

    
/**
   \todo Make this thread safe
*/
class Referenced
{
    friend class WeakCounter;
    template<class Y> friend class weak_ref_ptr;
    template<class Y> friend class ref_ptr;

protected:
    Referenced() : refCount_(0), weakCounter_(0) { }
    Referenced(const Referenced& org) : refCount_(0), weakCounter_(0) { }

public:
    virtual ~Referenced() {
        if(weakCounter_){
            weakCounter_->setDestructed();
        }
    }

#ifdef CNOID_REFERENCED_USE_ATOMIC_COUNTER
    void addRef() const {
        refCount_.fetch_add(1, boost::memory_order_relaxed);
    }
    void releaseRef() const {
        if(refCount_.fetch_sub(1, boost::memory_order_release) == 1) {
            boost::atomic_thread_fence(boost::memory_order_acquire);
            delete this;
        }
    }
private:
    mutable boost::atomic<int> refCount_;
            
protected:
    int refCount() const { return refCount_.load(boost::memory_order_relaxed); }
#else
    void addRef() {
        ++refCount_;
    }
    void releaseRef() {
        if(--refCount_ == 0){
            delete this;
        }
    }

private:
    int refCount_;
            
protected:
    int refCount() const { return refCount_; }
#endif
        
private:
    WeakCounter* weakCounter_;
        
    WeakCounter* weakCounter(){
        if(!weakCounter_){
            weakCounter_ = new WeakCounter();
        }
        return weakCounter_;
    }
};

    
template<class T> class ref_ptr
{
public:
    typedef T element_type;
    
    ref_ptr() : px(0) { }

    ref_ptr(T* p) : px(p){
        if(px != 0){
            px->addRef();
        }
    }

    template<class U>
    ref_ptr(ref_ptr<U> const & rhs) : px(rhs.get()){
        if(px != 0){
            px->addRef();
        }
    }

    ref_ptr(ref_ptr const & rhs) : px(rhs.px){
        if(px != 0){
            px->addRef();
        }
    }

    ~ref_ptr(){
        if(px != 0){
            px->releaseRef();
        }
    }

    template<class U> ref_ptr& operator=(ref_ptr<U> const & rhs){
        ref_ptr(rhs).swap(*this);
        return *this;
    }

#if defined( BOOST_HAS_RVALUE_REFS )

    ref_ptr(ref_ptr&& rhs) : px(rhs.px){
        rhs.px = 0;
    }

    ref_ptr & operator=(ref_ptr&& rhs){
        ref_ptr(static_cast<ref_ptr &&>(rhs)).swap(*this);
        return *this;
    }

#endif

    ref_ptr& operator=(ref_ptr const & rhs){
        ref_ptr(rhs).swap(*this);
        return *this;
    }

    ref_ptr& operator=(T* rhs){
        ref_ptr(rhs).swap(*this);
        return *this;
    }

    void reset(){
        ref_ptr().swap(*this);
    }

    void reset(T* rhs){
        ref_ptr(rhs).swap(*this);
    }

    // explicit conversion to the raw pointer
    T* get() const{
        return px;
    }

    // implict conversion to the raw pointer
    operator T*() const {
        return px;
    }        

    T& operator*() const {
        assert(px != 0);
        return *px;
    }

    T* operator->() const {
        assert(px != 0);
        return px;
    }

    void swap(ref_ptr& rhs){
        T* tmp = px;
        px = rhs.px;
        rhs.px = tmp;
    }

private:
    template<class Y> friend class weak_ref_ptr;
    template<class Y> friend class ref_ptr;

    T* px;
};


template<class T, class U> inline bool operator==(ref_ptr<T> const & a, ref_ptr<U> const & b)
{
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=(ref_ptr<T> const & a, ref_ptr<U> const & b)
{
    return a.get() != b.get();
}

template<class T, class U> inline bool operator==(ref_ptr<T> const & a, U * b)
{
    return a.get() == b;
}

template<class T, class U> inline bool operator!=(ref_ptr<T> const & a, U * b)
{
    return a.get() != b;
}

template<class T, class U> inline bool operator==(T * a, ref_ptr<U> const & b)
{
    return a == b.get();
}

template<class T, class U> inline bool operator!=(T * a, ref_ptr<U> const & b)
{
    return a != b.get();
}

template<class T> inline bool operator<(ref_ptr<T> const & a, ref_ptr<T> const & b)
{
    return a.get() < b.get();
}

template<class T> void swap(ref_ptr<T> & lhs, ref_ptr<T> & rhs)
{
    lhs.swap(rhs);
}

template<class T, class U> ref_ptr<T> static_pointer_cast(ref_ptr<U> const & p)
{
    return static_cast<T*>(p.get());
}
    
template<class T, class U> ref_ptr<T> const_pointer_cast(ref_ptr<U> const & p)
{
    return const_cast<T*>(p.get());
}
    
template<class T, class U> ref_ptr<T> dynamic_pointer_cast(ref_ptr<U> const & p)
{
    return dynamic_cast<T*>(p.get());
}

template<class Y> std::ostream & operator<< (std::ostream & os, ref_ptr<Y> const & p)
{
    os << p.get();
    return os;
}
    
    
typedef ref_ptr<Referenced> ReferencedPtr;


template<class T> class weak_ref_ptr
{
    typedef void (weak_ref_ptr<T>::*bool_type)() const;

    void bool_type_func() const { }
    
    void setCounter(){
        if(px){
            counter = px->weakCounter();
            counter->add();
        } else {
            counter = 0;
        }
    }
            
public:
    typedef T element_type;
    
    weak_ref_ptr() : px(0), counter(0) { }

    template<class Y>
    weak_ref_ptr(weak_ref_ptr<Y> const & rhs) : px(rhs.lock().get()){
        setCounter();
    }

    template<class Y>
    weak_ref_ptr& operator=(weak_ref_ptr<Y> const & rhs){
        px = rhs.lock().get();
        setCounter();
        return *this;
    }
        
#if defined( BOOST_HAS_RVALUE_REFS )

    template<class Y>
    weak_ref_ptr(weak_ref_ptr<Y>&& rhs) : px(rhs.lock().get()), counter(rhs.counter){
        rhs.px = 0;
        rhs.counter = 0;
    }

    weak_ref_ptr(weak_ref_ptr&& rhs) : px(rhs.px), counter(rhs.counter){
        rhs.px = 0;
        rhs.counter = 0;
    }

    weak_ref_ptr& operator=(weak_ref_ptr&& rhs){
        weak_ref_ptr(static_cast<weak_ref_ptr&&>(rhs)).swap(*this);
        rhs.px = 0;
        rhs.counter = 0;
        return rhs;
    }

#endif
    template<class Y>
    weak_ref_ptr(ref_ptr<Y> const & rhs) : px(rhs.px){
        setCounter();
    }

    template<class Y>
    weak_ref_ptr(Y* const & rhs) : px(rhs){
        setCounter();
    }
        
    template<class Y>
    weak_ref_ptr& operator=(ref_ptr<Y> const & rhs){
        px = rhs.px;
        setCounter();
        return *this;
    }

    operator bool_type() const { return px ? &weak_ref_ptr<T>::bool_type_func : 0; }

    ref_ptr<T> lock() const {
        if(counter && counter->isObjectAlive()){
            return ref_ptr<T>(px);
        } else {
            return ref_ptr<T>();
        }
    }

    bool expired() const {
        return !counter || !counter->isObjectAlive();
    }

    void reset(){
        weak_ref_ptr().swap(*this);
    }

    void swap(weak_ref_ptr& other){
        T* px_ = px;
        px = other.px;
        other.px = px_;
        WeakCounter* counter_ = counter;
        counter = other.counter;
        other.counter = counter_;
    }

    template<class Y> bool _internal_less(weak_ref_ptr<Y> const & rhs) const {
        return counter < rhs.counter;
    }
        
private:
    template<class Y> friend class weak_ref_ptr;
    template<class Y> friend class ref_ptr;
                                                        
    T* px;
    WeakCounter* counter;
};

template<class T, class U> inline bool operator<(weak_ref_ptr<T> const & a, weak_ref_ptr<U> const & b)
{
    return a._internal_less(b);
}

template<class T> void swap(weak_ref_ptr<T> & a, weak_ref_ptr<T> & b)
{
    a.swap(b);
}

}

#endif
