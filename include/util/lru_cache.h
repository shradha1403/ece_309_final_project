// include/util/lru_cache.h
//
// YOU IMPLEMENT (this whole file; it is a template). Spec §4.2.
//
// A fixed-capacity cache that evicts the Least Recently Used entry.
// Both get() and put() must be O(1) (expected), so it is built from two
// structures that point at the same nodes:
//
//   std::unordered_map<Key, Node*>   finds a key's node in O(1)
//   a doubly linked list of Nodes    keeps recency order:
//        head_ = most recently used ... tail_ = least recently used
//
// Rules for this file:
//   * Write the doubly linked list yourself (Node with prev/next pointers,
//     new/delete inside this class only). Do NOT use std::list, std::deque
//     or std::forward_list (don't even #include their headers), or a
//     std::vector you shift around (spec §7.3).
//   * std::unordered_map is allowed (and expected) for the key -> Node* index.
//   * No leaks, no double frees: ASan runs on every test.

#pragma once
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class LruCache {
public:
    struct Stats {
        std::size_t hits = 0;        // get() found the key
        std::size_t misses = 0;      // get() did not find the key
        std::size_t evictions = 0;   // put() removed the LRU entry to make room
    };

    // A cache holding at most `capacity` entries. Capacity 0 is legal: such a
    // cache never stores anything (every get() is a miss).
    explicit LruCache(std::size_t capacity) : capacity_(capacity) {}

    // Frees every node.
    ~LruCache() { 
        clear(); 
    }

    // Copying a cache of raw-pointer nodes is not needed; forbid it.
    LruCache(const LruCache&) = delete;
    LruCache& operator=(const LruCache&) = delete;

    // Hit: marks the entry most recently used, counts a hit, and returns a
    //      pointer to its value (valid until the next put/erase/clear).
    // Miss: counts a miss and returns nullptr.
    const Value* get(const Key& key) {
        // The map lets us find a key without searching through the whole list.
        auto found = map_.find(key);

        // If the key isn't in the map, it isn't in the cache.
        if (found == map_.end()) {
            stats_.misses++;
            return nullptr;
        }

        // The map gives us the pointer to the node that stores this key and value.
        Node* entry = found->second;

        // Since this entry was just used, it becomes the most recently used.
        // The front of our linked list always represents the most recently used entry.
        move_to_front(entry);

        stats_.hits++;

        // Return the address of the value stored inside the node.
        return &entry->value;
    }

    // Key present: replaces its value and marks it most recently used (no
    //   eviction, no hit/miss counted).
    // Key absent: if the cache is full, evicts the least recently used entry
    //   (counting one eviction), then inserts the new entry as most recently
    //   used. With capacity 0, does nothing.
    void put(const Key& key, Value value) {
        // Capacity zero means the cache cannot hold anything.
        if (capacity_ == 0) {
            return;
        }

        // First check if this key is already in the cache.
        auto found = map_.find(key);

        if (found != map_.end()) {
            // We don't need a new node because this key already exists.
            Node* entry = found->second;

            // Replace the old value with the new one.
            entry->value = std::move(value);

            // Updating an entry also makes it the most recently used.
            move_to_front(entry);

            return;
        }

        // If the cache is full, we have to remove one entry before adding another.
        if (map_.size() == capacity_) {
            // The tail is always the least recently used entry.
            Node* leastUsed = tail_;

            // Take it out of the linked list first.
            unlink(leastUsed);

            // Remove its key from the map so it can't be found anymore.
            // This has to happen before delete because we still need its key.
            map_.erase(leastUsed->key);

            // We no longer need this node, so free its memory.
            delete leastUsed;

            stats_.evictions++;
        }

        // Create a new node for the key and value.
        // Both pointers start as nullptr because it isn't connected to the list yet.
        Node* newEntry = new Node{key, std::move(value), nullptr, nullptr};

        // New entries go at the front because they were just added.
        push_front(newEntry);

        // Store the node's address in the map so we can find it quickly later.
        map_[key] = newEntry;
    }


    // True if the key is cached. Does NOT change recency or stats.
    bool contains(const Key& key) const {
        // Just checks whether the key exists.
        // Unlike get(), this does not change the order or update the stats.
        return map_.find(key) != map_.end();
    }

    // Removes the key if present (not an eviction). Returns whether it was.

    bool erase(const Key& key) {
        // Find the entry before trying to remove it.
        auto found = map_.find(key);

        // Nothing to remove if the key doesn't exist.
        if (found == map_.end()) {
            return false;
        }

        // Get the node pointer from the map.
        Node* entry = found->second;

        // Remove the node from the linked list so the other nodes connect properly.
        unlink(entry);

        // Remove the key from the map as well.
        map_.erase(found);

        // Now that neither structure needs this node, delete it.
        delete entry;

        // Erasing an entry is not counted as an eviction.
        return true;
    }

    // Removes every entry. Stats are kept.
    void clear() {
        // Start at the head and visit every node in the linked list.
        Node* walker = head_;

        while (walker != nullptr) {
            // Save the next node before deleting the current one.
            // Once walker is deleted, we cannot safely access walker->next.
            Node* nextEntry = walker->next;

            // Free the current node's memory.
            delete walker;

            // Move to the next node we saved.
            walker = nextEntry;
        }

        // The list is now empty, so both pointers need to be reset.
        head_ = nullptr;
        tail_ = nullptr;

        // Remove all the keys from the map too.
        // Stats are intentionally kept because the spec says not to reset them.
        map_.clear();
    }

    std::size_t size() const noexcept {
        return map_.size();
    }

    std::size_t capacity() const noexcept {
        return capacity_;
    }

    Stats stats() const noexcept {
        return stats_;
    }

    // Keys from most to least recently used, by walking the list from head_.
    // Used by the tests to check your recency order.
    std::vector<Key> keys_mru_to_lru() const {
        std::vector<Key> order;

        // Walk from head to tail to get keys from most recently used to least used.
        for (Node* walker = head_; walker != nullptr; walker = walker->next) {
            order.push_back(walker->key);
        }

        return order;
    }

private:
    struct Node {
        Key   key;
        Value value;
        Node* prev;
        Node* next;
    };

    // ===== YOUR HELPERS AND MEMBERS =====
    // Suggested helpers: unlink(Node*), push_front(Node*), move_to_front(Node*).
    // Clue for unlink(n): four cases. If n has a prev, prev->next skips n,
    // otherwise n was head_ and head_ moves to n->next. Likewise, if n has
    // a next, next->prev skips n, otherwise n was tail_ and tail_ moves to
    // n->prev. Unlinking the only node leaves head_ == tail_ == nullptr.
    // Spec Appendix B lists every pointer write.

    // This removes a node from the linked list without deleting its memory.
    void unlink(Node* entry) {
        // If there is a node before this one, connect it to the node after.
        // Otherwise, this entry was the head, so move the head forward.
        if (entry->prev != nullptr) {
            entry->prev->next = entry->next;
        } else {
            head_ = entry->next;
        }

        // If there is a node after this one, connect it to the node before.
        // Otherwise, this entry was the tail, so move the tail backward.
        if (entry->next != nullptr) {
            entry->next->prev = entry->prev;
        } else {
            tail_ = entry->prev;
        }

        // This also handles removing the only node.
        // In that case, both head_ and tail_ become nullptr.
    }

    // Adds a node at the front of the linked list.
    // The front is where we keep the most recently used entry.
    void push_front(Node* entry) {
        // The new front has nothing before it.
        entry->prev = nullptr;

        // Its next pointer should point to the old head.
        entry->next = head_;

        // If the list wasn't empty, the old head now comes after this node.
        if (head_ != nullptr) {
            head_->prev = entry;
        }

        // Update the head because this node is now first.
        head_ = entry;

        // If the list was empty, this node is also the tail.
        if (tail_ == nullptr) {
            tail_ = entry;
        }
    }

    // Moves an existing node to the front without creating a new node.
    void move_to_front(Node* entry) {
        // If it is already at the front, there is nothing to change.
        if (entry == head_) {
            return;
        }

        // First disconnect it from its current position.
        unlink(entry);

        // Then put it at the front.
        push_front(entry);
    }

    std::unordered_map<Key, Node*, Hash> map_;

    // Head is most recently used, tail is least recently used.
    Node* head_ = nullptr;
    Node* tail_ = nullptr;

    std::size_t capacity_;
    Stats stats_;
};
