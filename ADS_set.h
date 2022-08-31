#ifndef ADS_SET_H
#define ADS_SET_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

template <typename Key, size_t N = 7> 
class ADS_set {
	public:
		class Iterator;
		using value_type = Key;
		using key_type = Key;
		using reference = value_type &;
		using const_reference = const value_type &;
		using size_type = size_t;
		using difference_type = std::ptrdiff_t;
		using const_iterator = Iterator;
		using iterator = const_iterator;
		// using key_compare = std::less<key_type>;                         // B+-Tree
		using key_equal = std::equal_to<key_type>; // Hashing
		using hasher = std::hash<key_type>;        // Hashing
	private:
		enum class Mode { free, used, freeagain, end };
		struct Element {
		  key_type key;
		  Mode mode{Mode::free};
		  Element* next = nullptr;
		  Element* previous = nullptr;
		};

		Element *table{nullptr};
		size_type table_size{0};
		size_type current_size{0};
		size_type keller_size{0};
		size_type whole_size{0};
		float max_lf{0.85};

		Element *add(const key_type &key);
		Element *locate(const key_type &key) const;
		size_type h(const key_type &key) const { return hasher{}(key) % table_size; }
		Element *next_free = nullptr;
		void reserve(size_type n);
		void rehash(size_type n);

	public:
		ADS_set() { rehash(N); } // PH1
		ADS_set(std::initializer_list<key_type> ilist) : ADS_set{} { insert(ilist); } // PH1
		template <typename InputIt> ADS_set(InputIt first, InputIt last) : ADS_set{} { insert(first, last); } // PH1
		ADS_set(const ADS_set &other) {
		  rehash(other.whole_size);
		  for (const auto &o : other) add(o);
		}

		~ADS_set() { delete[] table; };

		ADS_set &operator=(const ADS_set &other) {
		  if (this == &other)
		    return *this;
		  ADS_set tmp{other};
		  swap(tmp);
		  return *this;
		}
		ADS_set &operator=(std::initializer_list<key_type> ilist) {
		  ADS_set tmp{ilist};
		  swap(tmp);
		  return *this;
		}

		size_type size() const { return current_size; };  // PH1
		bool empty() const { return current_size == 0; }; // PH1

		void insert(std::initializer_list<key_type> ilist) { insert(ilist.begin(), ilist.end()); } // PH1
		std::pair<iterator, bool> insert(const key_type &key) {
		  if (auto e{locate(key)}) return {iterator{e}, false};
		  reserve(current_size + 1);
		  return {iterator{add(key)}, true};
		}
		template <typename InputIt> void insert(InputIt first, InputIt last); // PH1

		void clear() {
		  ADS_set tmp;
		  swap(tmp);
		}
		size_type erase(const key_type &key);

		size_type count(const key_type &key) const { return locate(key) != nullptr; } // PH1
		iterator find(const key_type &key) const {
		  if (auto e{locate(key)}) return iterator{e};
		  return end();
		}

		void swap(ADS_set &other) {
		  using std::swap;
		  swap(table, other.table);
		  swap(next_free, other.next_free);
		  swap(current_size, other.current_size);
		  swap(table_size, other.table_size);
		  swap(keller_size, other.keller_size);
		  swap(whole_size, other.whole_size);
		}

		const_iterator begin() const { return const_iterator{table}; }
		const_iterator end() const { return const_iterator{table + whole_size}; }

		void dump(std::ostream &o = std::cerr) const;

		friend bool operator==(const ADS_set &lhs, const ADS_set &rhs) {
		  if (lhs.current_size != rhs.current_size)
		    return false;
		  for (const auto &k : lhs)
		    if (!rhs.count(k))
		      return false;
		  return true;
		}
		friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs) { return !(lhs == rhs); }
	};
template <typename Key, size_t N>
typename ADS_set<Key, N>::Element *ADS_set<Key, N>::add(const key_type &key) {
  size_type idx{h(key)};
  Element* key_ptr = table + idx;
  Element* temp = nullptr;
  while (key_ptr != nullptr) {
    temp = key_ptr;
    key_ptr = key_ptr->Element::next;
  }
  if (temp->Element::mode == Mode::used) {
    *next_free = {key, Mode::used, nullptr, temp};
    temp->Element::next = next_free;
    temp = next_free;
  } else {
    *temp = {key, Mode::used, nullptr, nullptr};
  }
  while (next_free->Element::mode == Mode::used) {
    --next_free;
  }
  ++current_size;
  return temp;
}
template <typename Key, size_t N>
typename ADS_set<Key, N>::size_type ADS_set<Key, N>::erase(const key_type &key) {
  size_type idx {h(key)};
  switch (table[idx].mode) {
		case Mode::free:
			return 0;
		case Mode::freeagain:
			return 0;
		case Mode::end:
			return 0;
		case Mode::used:
			if(Element* e{locate(key)}) {
				Element* next = nullptr;
				e->Element::mode = Mode::freeagain;
				if(e->Element::previous != nullptr) {
					e->Element::previous->Element::next = nullptr;
					e->Element::previous = nullptr;
				}
				if(e > next_free) {
					next_free = e;
				}
				next = e->Element::next;
				Element* temp = next;
				e->Element::next = nullptr;
				--current_size;
				if(temp != nullptr) {
					std::vector<key_type> list = {};
					do {
						next = temp;
						temp = temp->Element::next;
						if(next > next_free) {
							next_free = next;
						}
						list.push_back(next->Element::key);
						next->Element::mode = Mode::free;
						next->Element::next = nullptr;
						--current_size;
					} while(temp != nullptr);
					insert(list.begin(), list.end());
				}
				return 1;
			}
	}
 return 0; 
}
template <typename Key, size_t N>
typename ADS_set<Key, N>::Element *ADS_set<Key, N>::locate(const key_type &key) const {
  size_type idx{h(key)}; //, start_idx{idx};
  Element *key_ptr{table + idx};
  do {
    switch (key_ptr->Element::mode) {
    case Mode::free:
      return nullptr;
    case Mode::used:
      if (key_equal{}(key_ptr->key, key))
        return key_ptr; // return &table[idx];
      break;
    case Mode::freeagain:
      break;
    case Mode::end:
      throw std::runtime_error("Element not in the table!");
    }
    key_ptr = key_ptr->Element::next;
  } while (key_ptr != nullptr);
  return nullptr;
}
template <typename Key, size_t N>
template <typename InputIt> void ADS_set<Key, N>::insert(InputIt first, InputIt last) {
  for (auto it{first}; it != last; ++it) {
    if (!count(*it)) {
      reserve(current_size + 1);
      add(*it);
    }
  }
}
template <typename Key, size_t N>
void ADS_set<Key, N>::dump(std::ostream &o) const {
  o << "table_size = " << table_size << ", current_size = " << current_size << ", keller_size = " << keller_size << ", whole_size = " << whole_size << "\n";
  for (size_type idx{0}; idx <= whole_size; ++idx) {
    o << idx << ": ";
    switch (table[idx].mode) {
    case Mode::free:
      o << "--FREE";
      break;
    case Mode::used:
      o << table[idx].key << " --USED";
      if (table[idx].next != nullptr) {
        o << " --NEXT: " << table[idx].next - table;
      }
      break;
    case Mode::freeagain:
      o << "--FREEAGAIN";
      break;
    case Mode::end:
      o << "--END";
    }
    o << "\n";
  }
}
template <typename Key, size_t N> void ADS_set<Key, N>::reserve(size_type n) {
  if (table_size * max_lf > n) return;
  size_type new_table_size{table_size};
  while (new_table_size * max_lf < n) ++(new_table_size *= 2);
  rehash(new_table_size);
}
template <typename Key, size_t N> void ADS_set<Key, N>::rehash(size_type n) {
  size_type new_table_size{std::max(N, std::max(n, size_type(current_size / max_lf)))};
  size_type new_keller_size{static_cast<size_type>(new_table_size * 0.1628)};
  size_type new_whole_size{new_table_size + new_keller_size};
  Element *new_table{new Element[new_whole_size + 1]};
  Element *old_table{table};
  size_type old_whole_size{whole_size};

  current_size = 0;
  table = new_table;
  table_size = new_table_size;
  keller_size = new_keller_size;
  whole_size = new_whole_size;
  next_free = table + whole_size - 1;
  table[whole_size].mode = Mode::end;

  for (size_type idx{0}; idx < old_whole_size; ++idx) {
    if (old_table[idx].mode == Mode::used)
      add(old_table[idx].key);
  }
  delete[] old_table;
}

template <typename Key, size_t N>
class ADS_set<Key, N>::Iterator {
  Element *e;
  void skip() { while (e->mode == Mode::free || e->mode == Mode::freeagain) ++e; }
	public:
		using value_type = Key;
		using difference_type = std::ptrdiff_t;
		using reference = const value_type &;
		using pointer = const value_type *;
		using iterator_category = std::forward_iterator_tag;

		explicit Iterator(Element *e = nullptr) : e{e} { if (e) skip(); }
		reference operator*() const { return e->key; }
		pointer operator->() const { return &e->key; }
		Iterator &operator++() {
		  ++e;
		  skip();
		  return *this;
		}
		Iterator operator++(int) {
		  auto rc{*this};
		  ++*this;
		  return rc;
		}
		friend bool operator==(const Iterator &lhs, const Iterator &rhs) { return lhs.e == rhs.e; }
		friend bool operator!=(const Iterator &lhs, const Iterator &rhs) { return !(lhs == rhs); }
};
template <typename Key, size_t N>
void swap(ADS_set<Key, N> &lhs, ADS_set<Key, N> &rhs) {
  lhs.swap(rhs);
}
#endif // ADS_SET_H
