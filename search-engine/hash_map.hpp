#pragma once
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

unsigned long hash_utf8(const std::string& s) {
  unsigned long h = 5381;
  for (unsigned char c : s) h = ((h << 5) + h) + c;
  return h;
}

template <typename V>
class HashMap {
 private:
  struct Node {
    std::string key;
    V value;
    Node* next;
    Node(const std::string& k, const V& v) : key(k), value(v), next(nullptr) {}
  };

  Node** buckets;
  size_t cap;
  size_t size_ = 0;

 public:
  HashMap(size_t capacity = 65536) : cap(capacity) {
    buckets = (Node**)calloc(cap, sizeof(Node*));
  }

  ~HashMap() {
    clear();
    free(buckets);
  }

  void clear() {
    for (size_t i = 0; i < cap; i++) {
      Node* n = buckets[i];
      while (n) {
        Node* tmp = n->next;
        delete n;
        n = tmp;
      }
      buckets[i] = nullptr;
    }
    size_ = 0;
  }

  V* find(const std::string& key) {
    size_t h = hash_utf8(key) % cap;
    Node* n = buckets[h];
    while (n) {
      if (n->key == key) return &n->value;
      n = n->next;
    }
    return nullptr;
  }

  void insert(const std::string& key, const V& value) {
    size_t h = hash_utf8(key) % cap;
    Node* n = buckets[h];
    while (n) {
      if (n->key == key) {
        n->value = value;
        return;
      }
      n = n->next;
    }
    Node* nn = new Node(key, value);
    nn->next = buckets[h];
    buckets[h] = nn;
    size_++;
  }

  size_t size() const { return size_; }

  void get_all_pairs(std::vector<std::pair<std::string, V>>& pairs) {
    pairs.clear();
    pairs.reserve(size_);
    for (size_t i = 0; i < cap; i++) {
      Node* n = buckets[i];
      while (n) {
        pairs.push_back(std::make_pair(n->key, n->value));
        n = n->next;
      }
    }
  }
};