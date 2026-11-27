// include/util/min_heap.h
//
// YOU IMPLEMENT (this whole file; it is a template, so the code lives in the
// header). Spec §4.1.
//
// A binary MIN-heap stored in an array (a std::vector), plus top_k(), which
// uses a size-k min-heap to pick the k largest items in O(n log k).
//
// Rules for this file:
//   * Write sift-up and sift-down yourself.
//   * Do NOT use std::priority_queue, std::make_heap / push_heap / pop_heap /
//     sort_heap, std::sort / stable_sort / partial_sort / nth_element, or
//     std::set / multiset / map / multimap (full list: spec §7.3).
//   * std::vector is fine as the storage array; std::swap, std::reverse and
//     std::pair are fine too.
//
// `Less` is a "less-than" function object, exactly like the one std::sort
// takes: less(a, b) is true when a should come before b. The heap keeps the
// SMALLEST element (by less) at the top.

#pragma once
#include <algorithm>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T, typename Less = std::less<T>>
class MinHeap {
public:
    explicit MinHeap(Less less = Less()) : less_(std::move(less)) {}

    // Adds value. O(log n).
    void push(T value) {
        //add the new element to the end.
        data_.push_back(std::move(value));
        //start at the newly inserted element.
        std::size_t boi = data_.size()-1;

        //keep moving up while the child has a parent 
        while(boi>0){
            std:: size_t boss = (boi-1)/2;

            //checking if the child belongs above its parent
            if(!less_(data_[boi], data_[boss])){
                break;
            }
            //swap them
            std::swap(data_[boi], data_[boss]);

            //continue from the parent's old pos
            boi=boss;
        }
        
    }

    // The smallest element. Throws std::out_of_range if empty. O(1).
    const T& top() const {
        if(data_.empty()){
            throw std::out_of_range("HEAP IS EMPTY");
        }
        return data_.front();
    }

    // Removes and returns the smallest element. Throws std::out_of_range if
    // empty. O(log n).
    T pop() {
        if(empty()){
            throw std::out_of_range("MinHeap::pop on empty heap");
        }
        //save the smallest element
        T result = std::move(data_[0]);
        //if theres only one element
        if(data_.size()==1){
            data_.pop_back();
            return result;
        }
        //moving the last element to the root
        data_[0]=std::move(data_.back());
        data_.pop_back();

        //sifting down
        std::size_t boss=0;
        while(true){
            std::size_t left =2*boss+1;
            std::size_t right =2*boss+2;
            std::size_t smallest =boss;
        // left child smaller?
            if(left<data_.size()&& less_(data_[left], data_[smallest])){
                smallest=left;
            }

        //right child smaller?
            if(right< data_.size()&& less_(data_[right], data_[smallest])){
                smallest = right;
            }

        //heap property restored

            if(smallest==boss){
                break;
            }

            std::swap(data_[boss], data_[smallest]);

            boss=smallest;
        }

        return result;

        }

    std::size_t size() const noexcept { return data_.size(); }
    bool        empty() const noexcept { return data_.empty(); }
    void        clear() noexcept { data_.clear(); }

private:
    // ===== YOUR HELPERS AND MEMBERS =====
    // Children of index i are 2i+1 and 2i+2; its parent is (i-1)/2.
    // Suggested: void sift_up(std::size_t i);  void sift_down(std::size_t i);
    // Clue: use std::size_t (not int) for indices, or -Wsign-compare warns
    // every time you compare with data_.size(). Careful: (i-1)/2 with i == 0
    // underflows, so sift_up must stop at i == 0.
    // Clue: in sift_down, the right child may not exist: check
    // 2*i+2 < data_.size() before reading it. Swap with the SMALLER child.

    std::vector<T> data_;
    Less           less_;
};

// Returns the k LARGEST items of `items` (by `less`), largest first.
//   * k == 0 or items empty -> empty result
//   * k >= items.size()     -> every item, largest first
// Must keep a MinHeap of at most k items (the k largest seen so far, with
// the smallest of them on top), so it runs in O(n log k) time and O(k) extra
// space. Items that compare equal may come out in any order.
template <typename T, typename Less = std::less<T>>
std::vector<T> top_k(const std::vector<T>& items, std::size_t k, Less less = Less()) {

    if(k == 0 || items.empty()){
        return {};
    }

    MinHeap<T, Less> heap(less);

    for(const T& item : items){
        if(heap.size() < k){
            heap.push(item);
        }
        else if(less(heap.top(), item)){
            heap.pop();
            heap.push(item);
        }
    }

    std::vector<T> result;

    while(!heap.empty()){
        result.push_back(heap.pop());
    }

    std::reverse(result.begin(), result.end());

    return result;
}
