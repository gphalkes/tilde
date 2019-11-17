/* Copyright (C) 2011-2012,2018 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include "tilde/filebuffer.h"
#include "tilde/log.h"
#include "tilde/openfiles.h"
#include "tilde/option.h"
#include "tilde/string_util.h"

static const char kRecentFiles[] = "recent_files";
static constexpr size_t kMaxSavedRecentFiles = 100;
static const char recent_files_schema[] = {
#include "recent_files.bytes"
};

open_files_t open_files;
recent_files_t recent_files;

size_t open_files_t::size() const { return files.size(); }
bool open_files_t::empty() const { return files.empty(); }

void open_files_t::push_back(file_buffer_t *text) {
  files.push_back(text);
  version++;
}

open_files_t::iterator open_files_t::erase(open_files_t::iterator position) {
  open_files_t::iterator result = files.erase(position);
  version++;
  return result;
}

open_files_t::iterator open_files_t::contains(const char *name) {
  /* FIXME: this should really check for ino+dev equality to see if the file
     is already open. */
  for (std::vector<file_buffer_t *>::iterator iter = files.begin(); iter != files.end(); iter++) {
    if (!(*iter)->get_name().empty() && (*iter)->get_name() == name) {
      return iter;
    }
  }
  return files.end();
}

int open_files_t::get_version() { return version; }
open_files_t::iterator open_files_t::begin() { return files.begin(); }
open_files_t::iterator open_files_t::end() { return files.end(); }
open_files_t::reverse_iterator open_files_t::rbegin() { return files.rbegin(); }
open_files_t::reverse_iterator open_files_t::rend() { return files.rend(); }
file_buffer_t *open_files_t::operator[](size_t idx) { return files[idx]; }
file_buffer_t *open_files_t::back() { return files.back(); }

void open_files_t::erase(file_buffer_t *buffer) {
  for (iterator iter = files.begin(); iter != files.end(); iter++) {
    if (*iter == buffer) {
      files.erase(iter);
      version++;
      return;
    }
  }
}

file_buffer_t *open_files_t::next_buffer(file_buffer_t *start) {
  iterator current = files.begin(), iter;

  if (start != nullptr) {
    for (; current != files.end() && *current != start; current++) {
    }
  }

  for (iter = current; iter != files.end(); iter++) {
    if (!(*iter)->get_has_window()) {
      return *iter;
    }
  }

  for (iter = files.begin(); iter != current; iter++) {
    if (!(*iter)->get_has_window()) {
      return *iter;
    }
  }
  return start;
}

file_buffer_t *open_files_t::previous_buffer(file_buffer_t *start) {
  reverse_iterator current = files.rbegin(), iter;

  if (start != nullptr) {
    for (; current != files.rend() && *current != start; current++) {
    }
  }

  for (iter = current; iter != files.rend(); iter++) {
    if (!(*iter)->get_has_window()) {
      return *iter;
    }
  }

  for (iter = files.rbegin(); iter != current; iter++) {
    if (!(*iter)->get_has_window()) {
      return *iter;
    }
  }
  return start;
}

recent_file_info_t::recent_file_info_t(const file_buffer_t *file)
    : recent_file_info_t(file->get_name(), file->get_encoding(), file->get_cursor(),
                         file->get_behavior_parameters()->get_top_left(),
                         static_cast<int64_t>(std::time(nullptr))) {}

recent_file_info_t::recent_file_info_t(string_view _name, string_view _encoding,
                                       text_coordinate_t _position, text_coordinate_t _top_left,
                                       int64_t _close_time)
    : name(_name),
      encoding(_encoding),
      position(_position),
      top_left(_top_left),
      close_time(_close_time) {
  lprintf("Top left: %ld %ld\n", top_left.line, top_left.pos);
}

const std::string &recent_file_info_t::get_name() const { return name; }
const std::string &recent_file_info_t::get_encoding() const { return encoding; }
text_coordinate_t recent_file_info_t::get_position() const { return position; }
text_coordinate_t recent_file_info_t::get_top_left() const { return top_left; }
int64_t recent_file_info_t::get_close_time() const { return close_time; }

void recent_files_t::push_front(const file_buffer_t *text) {
  if (text->get_name().empty()) {
    return;
  }

  for (auto iter = recent_file_infos.begin(); iter != recent_file_infos.end();) {
    if ((*iter)->get_name() == text->get_name()) {
      iter = recent_file_infos.erase(iter);
    } else {
      ++iter;
    }
  }

  version++;

  recent_file_infos.push_front(make_unique<recent_file_info_t>(text));
  size_t max_files = std::max(option.max_recent_files, kMaxSavedRecentFiles);
  if (recent_file_infos.size() > max_files) {
    recent_file_infos.resize(max_files);
  }
}

recent_file_info_t *recent_files_t::get_info(size_t idx) { return recent_file_infos[idx].get(); }

void recent_files_t::erase(recent_file_info_t *info) {
  for (auto iter = recent_file_infos.begin(); iter != recent_file_infos.end(); iter++) {
    if (iter->get() == info) {
      recent_file_infos.erase(iter);
      version++;
      break;
    }
  }
}

int recent_files_t::get_version() { return version; }
recent_files_t::iterator recent_files_t::begin() { return recent_file_infos.begin(); }
recent_files_t::iterator recent_files_t::end() { return recent_file_infos.end(); }
recent_files_t::iterator recent_files_t::erase(iterator iter) {
  version++;
  return recent_file_infos.erase(iter);
}
recent_files_t::iterator recent_files_t::find(const std::string &name) {
  for (auto iter = recent_file_infos.begin(); iter != recent_file_infos.end(); ++iter) {
    if ((*iter)->get_name() == name) {
      return iter;
    }
  }
  return recent_file_infos.end();
}

void recent_files_t::load_from_disk() {
  lprintf("Starting recent files load\n");
  std::unique_ptr<char, free_deleter> xdg_path(
      t3_config_xdg_get_path(T3_CONFIG_XDG_CACHE_HOME, "tilde", 0));
  std::string recent_files_path;
  strings::Append(&recent_files_path, xdg_path.get(), "/", kRecentFiles);
  xdg_path.reset();

  std::unique_ptr<FILE, fclose_deleter> recent_files_file(fopen(recent_files_path.c_str(), "r"));
  if (recent_files_file == nullptr) {
    lprintf("Could not open recent files file: %s: %m\n", recent_files_path.c_str());
    return;
  }
  int fd = fileno(recent_files_file.get());

  struct flock flock;
  flock.l_len = 1;
  flock.l_start = 0;
  flock.l_whence = SEEK_SET;
  flock.l_type = F_RDLCK;
  while (fcntl(fd, F_SETLKW, &flock) != 0) {
    if (errno != EINTR) {
      lprintf("Could not lock recent files file: %m\n");
      return;
    }
  }

  t3_config_opts_t opts;
  opts.flags = T3_CONFIG_VERBOSE_ERROR;
  t3_config_error_t error;
  error.error = 0;
  std::unique_ptr<t3_config_t, t3_config_deleter> config(
      t3_config_read_file(recent_files_file.get(), &error, &opts));
  recent_files_file.reset();
  if (!config) {
    lprintf("Error loading recent_files: %d %s: %s\n", error.error, t3_config_strerror(error.error),
            error.extra);
    free(error.extra);
    return;
  }

  std::unique_ptr<t3_config_schema_t, t3_schema_deleter> schema(t3_config_read_schema_buffer(
      recent_files_schema, sizeof(recent_files_schema), &error, &opts));
  if (!schema) {
    lprintf("Error loading recent_files schema: %d: %s: %s\n", error.error,
            t3_config_strerror(error.error), error.extra);
    free(error.extra);
    return;
  }

  if (!t3_config_validate(config.get(), schema.get(), &error, T3_CONFIG_VERBOSE_ERROR)) {
    lprintf("Error loading recent_files data: %s: %s\n", t3_config_strerror(error.error),
            error.extra);
    free(error.extra);
    return;
  }

  for (t3_config_t *recent_file =
           t3_config_get(t3_config_get(config.get(), "recent-files"), nullptr);
       recent_file != nullptr; recent_file = t3_config_get_next(recent_file)) {
    const char *name = t3_config_get_string(t3_config_get(recent_file, "name"));
    const char *encoding = t3_config_get_string(t3_config_get(recent_file, "encoding"));
    t3_config_t *position_config = t3_config_get(t3_config_get(recent_file, "position"), nullptr);
    text_coordinate_t position;
    text_coordinate_t top_left;
    position.line = t3_config_get_int64(position_config);
    position_config = t3_config_get_next(position_config);
    position.pos = t3_config_get_int64(position_config);

    position_config = t3_config_get_next(position_config);
    top_left.line = t3_config_get_int64(position_config);
    position_config = t3_config_get_next(position_config);
    top_left.pos = t3_config_get_int64(position_config);
    int64_t close_time = t3_config_get_int64(t3_config_get(recent_file, "close-time"));
    recent_file_infos.push_back(
        make_unique<recent_file_info_t>(name, encoding, position, top_left, close_time));
  }
  std::sort(recent_file_infos.begin(), recent_file_infos.end(),
            [](const std::unique_ptr<recent_file_info_t> &a,
               const std::unique_ptr<recent_file_info_t> &b) {
              return a->get_close_time() > b->get_close_time();
            });
  version++;
  lprintf("Loaded %zd recent files\n", recent_file_infos.size());
}

static bool make_dirs(char *dir) {
  char *slash = strchr(dir + (dir[0] == '/'), '/');

  while (slash != nullptr) {
    *slash = 0;
    if (mkdir(dir, 0777) == -1 && errno != EEXIST) {
      return false;
    }
    *slash = '/';
    slash = strchr(slash + 1, '/');
  }
  if (mkdir(dir, 0777) == -1 && errno != EEXIST) {
    return false;
  }
  return true;
}

void recent_files_t::write_to_disk() {
  std::unique_ptr<char, free_deleter> xdg_path(
      t3_config_xdg_get_path(T3_CONFIG_XDG_CACHE_HOME, "tilde", strlen(kRecentFiles)));
  if (!make_dirs(xdg_path.get())) {
    return;
  }

  std::string recent_files_path;
  strings::Append(&recent_files_path, xdg_path.get(), "/", kRecentFiles);
  xdg_path.reset();

  std::unique_ptr<FILE, fclose_deleter> recent_files_file(fopen(recent_files_path.c_str(), "r+"));
  if (recent_files_file == nullptr) {
    lprintf("Could not open recent files file: %s: %m\n", recent_files_path.c_str());
    return;
  }
  int fd = fileno(recent_files_file.get());

  struct flock flock;
  flock.l_len = 1;
  flock.l_start = 0;
  flock.l_whence = SEEK_SET;
  flock.l_type = F_WRLCK;
  while (fcntl(fd, F_SETLKW, &flock) != 0) {
    if (errno != EINTR) {
      lprintf("Could not lock recent files file: %m\n");
      return;
    }
  }

  t3_config_opts_t opts;
  opts.flags = T3_CONFIG_VERBOSE_ERROR;
  t3_config_error_t error;
  error.error = 0;
  std::unique_ptr<t3_config_t, t3_config_deleter> config(
      t3_config_read_file(recent_files_file.get(), &error, &opts));
  if (!config) {
    lprintf("Error loading recent_files: %d %s: %s\n", error.error, t3_config_strerror(error.error),
            error.extra);
    free(error.extra);
    return;
  }
  rewind(recent_files_file.get());

  std::unique_ptr<t3_config_schema_t, t3_schema_deleter> schema(t3_config_read_schema_buffer(
      recent_files_schema, sizeof(recent_files_schema), &error, &opts));
  if (!schema) {
    lprintf("Error loading recent_files schema: %d: %s: %s\n", error.error,
            t3_config_strerror(error.error), error.extra);
    free(error.extra);
    return;
  }

  if (!t3_config_validate(config.get(), schema.get(), &error, T3_CONFIG_VERBOSE_ERROR)) {
    lprintf("Error loading recent_files data: %s: %s\n", t3_config_strerror(error.error),
            error.extra);
    free(error.extra);
    return;
  }

  std::map<string_view, t3_config_t *> name_to_config;
  t3_config_t *recent_files_list = t3_config_get(config.get(), "recent-files");
  for (t3_config_t *recent_file = t3_config_get(recent_files_list, nullptr); recent_file != nullptr;
       recent_file = t3_config_get_next(recent_file)) {
    const char *name = t3_config_get_string(t3_config_get(recent_file, "name"));
    name_to_config[name] = recent_file;
  }

  if (!recent_files_list && !recent_file_infos.empty()) {
    recent_files_list = t3_config_add_plist(config.get(), "recent-files", nullptr);
    if (!recent_files_list) {
      lprintf("Could not add recent-files list to the config");
      return;
    }
  }

  for (const std::unique_ptr<recent_file_info_t> &recent_file : recent_file_infos) {
    auto existing_iter = name_to_config.find(recent_file->get_name());
    if (existing_iter == name_to_config.end()) {
      t3_config_t *new_recent_file = t3_config_add_section(recent_files_list, nullptr, nullptr);
      int combined_result = 0;
      combined_result |=
          t3_config_add_string(new_recent_file, "name", recent_file->get_name().c_str());
      combined_result |=
          t3_config_add_string(new_recent_file, "encoding", recent_file->get_encoding().c_str());
      t3_config_t *position_list = t3_config_add_list(new_recent_file, "position", nullptr);
      combined_result |=
          t3_config_add_int64(position_list, nullptr, recent_file->get_position().line);
      combined_result |=
          t3_config_add_int64(position_list, nullptr, recent_file->get_position().pos);
      combined_result |=
          t3_config_add_int64(position_list, nullptr, recent_file->get_top_left().line);
      combined_result |=
          t3_config_add_int64(position_list, nullptr, recent_file->get_top_left().pos);
      combined_result |=
          t3_config_add_int64(new_recent_file, "close-time", recent_file->get_close_time());
      if (combined_result != 0) {
        lprintf("Error in adding a new item to the recent-files list");
        return;
      }
    } else {
      int64_t close_time = t3_config_get_int64(t3_config_get(existing_iter->second, "close-time"));
      lprintf("Updating recent file: %s %ld %ld\n", recent_file->get_name().c_str(), close_time,
              recent_file->get_close_time());
      if (close_time < recent_file->get_close_time()) {
        t3_config_add_int64(existing_iter->second, "close-time", recent_file->get_close_time());
        t3_config_t *position_list = t3_config_add_list(existing_iter->second, "position", nullptr);
        t3_config_add_int64(position_list, nullptr, recent_file->get_position().line);
        t3_config_add_int64(position_list, nullptr, recent_file->get_position().pos);
        t3_config_add_int64(position_list, nullptr, recent_file->get_top_left().line);
        t3_config_add_int64(position_list, nullptr, recent_file->get_top_left().pos);
      }
    }
  }

  /* Trim the list of recent files down to a reasonable number, to avoid it from growing
     indefinitely. */
  std::vector<std::pair<int64_t, t3_config_t *>> timestamped_configs;
  timestamped_configs.reserve(kMaxSavedRecentFiles + recent_file_infos.size());
  for (t3_config_t *recent_file = t3_config_get(recent_files_list, nullptr); recent_file != nullptr;
       recent_file = t3_config_get_next(recent_file)) {
    timestamped_configs.push_back(
        {t3_config_get_int64(t3_config_get(recent_file, "close-time")), recent_file});
  }
  if (timestamped_configs.size() > kMaxSavedRecentFiles) {
    std::sort(timestamped_configs.begin(), timestamped_configs.end(),
              [](const std::pair<int64_t, t3_config_t *> &a,
                 const std::pair<int64_t, t3_config_t *> &b) { return a.first > b.first; });
    for (size_t i = kMaxSavedRecentFiles; i < timestamped_configs.size(); ++i) {
      t3_config_erase_from_list(recent_files_list, timestamped_configs[i].second);
    }
  }

  if (!t3_config_validate(config.get(), schema.get(), &error, T3_CONFIG_VERBOSE_ERROR)) {
    lprintf("Invalid recent_files data created: %s: %s\n", t3_config_strerror(error.error),
            error.extra);
    free(error.extra);
    return;
  }

  while (ftruncate(fd, 0) != 0) {
    if (errno != EINTR) {
      return;
    }
  }
  t3_config_write_file(config.get(), recent_files_file.get());
  fsync(fd);
}

void recent_files_t::cleanup() { recent_file_infos.clear(); }
