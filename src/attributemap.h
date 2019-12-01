#ifndef ATTRIBUTEMAP_H
#define ATTRIBUTEMAP_H

#include <map>
#include <set>
#include <t3widget/string_view.h>
#include <t3widget/util.h>
#include <t3window/terminal.h>
#include <vector>

using namespace t3widget;

class attribute_map_t {
 public:
  class iterator;

  /** Creates a new attribute_map_t, with only a "normal" entry. */
  attribute_map_t();

  /** Looks up the mapping from name to index.

      For names that are not mapped yet, it registers the name in the list of known highlights.
  */
  optional<int> lookup_mapping(string_view name);

  /** Looks up the attributes associated with name or @c nullopt if name has not been inserted. */
  optional<t3_attr_t> lookup_attributes(string_view name) const;

  /** Looks up the attributes associated with idx or @c nullpt if idx is out of range. */
  optional<t3_attr_t> lookup_attributes(int idx) const;

  /** Inserts or overwrites the mapping with the given attributes. */
  void insert_mapping(string_view name, t3_attr_t attr);

  void erase_mapping(string_view name);

  iterator begin() const;
  iterator end() const;

  void clear_mappings();

 private:
  std::set<std::string> known_highlights;
  std::map<string_view, int> mapping;
  std::vector<t3_attr_t> attributes;
};

class attribute_map_t::iterator {
 public:
  using difference_type = std::map<string_view, int>::difference_type;
  using value_type = const std::pair<string_view, t3_attr_t>;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::input_iterator_tag;

  bool operator==(const iterator &other) const { return iter_ == other.iter_; }
  bool operator!=(const iterator &other) const { return !(*this == other); }

  reference operator*() {
    value_ = {iter_->first, attribute_map_.attributes[iter_->second]};
    return value_;
  }
  pointer operator->() {
    value_ = {iter_->first, attribute_map_.attributes[iter_->second]};
    return &value_;
  }
  iterator &operator++() {
    ++iter_;
    return *this;
  }
  iterator operator++(int) {
    iterator result = *this;
    ++iter_;
    return result;
  }

 private:
  iterator(const attribute_map_t &attribute_map, std::map<string_view, int>::const_iterator iter)
      : attribute_map_(attribute_map), iter_(iter) {}

  const attribute_map_t &attribute_map_;
  std::map<string_view, int>::const_iterator iter_;
  std::pair<string_view, t3_attr_t> value_;

  friend class attribute_map_t;
};

#endif  // ATTRIBUTEMAP_H
