#include "attributemap.h"

attribute_map_t::attribute_map_t() {
  /* The normal mapping must be the first to be inserted. */
  insert_mapping("normal", 0);
}

optional<int> attribute_map_t::lookup_mapping(string_view name) {
  auto iter = mapping.find(name);
  if (iter != mapping.end()) {
    return iter->second;
  }
  known_highlights.insert(std::string(name));
  return nullopt;
}

optional<t3_attr_t> attribute_map_t::lookup_attributes(string_view name) const {
  auto iter = mapping.find(name);
  if (iter != mapping.end()) {
    return attributes[iter->second];
  }
  return nullopt;
}

optional<t3_attr_t> attribute_map_t::lookup_attributes(int idx) const {
  if (idx >= 0 && static_cast<size_t>(idx) < attributes.size()) {
    return attributes[idx];
  }
  return nullopt;
}

void attribute_map_t::insert_mapping(string_view name, t3_attr_t attr) {
  auto known_highlights_iter = known_highlights.insert(std::string(name)).first;
  auto mapping_iter = mapping.find(*known_highlights_iter);
  if (mapping_iter != mapping.end()) {
    attributes[mapping_iter->second] = attr;
    return;
  }
  /* Inserts based on the string in known_highlights, to ensure that the string_view member is
     initialized with a string that will outlive the mapping. */
  mapping.insert({*known_highlights_iter, attributes.size()});
  attributes.push_back(attr);
}

void attribute_map_t::erase_mapping(string_view name) { mapping.erase(name); }

attribute_map_t::iterator attribute_map_t::begin() const {
  return iterator(*this, mapping.begin());
}
attribute_map_t::iterator attribute_map_t::end() const { return iterator(*this, mapping.end()); }

void attribute_map_t::clear_mappings() {
  mapping.clear();
  attributes.clear();
  insert_mapping("normal", 0);
}
