/* Copyright (C) 2011-2012 G.P. Halkes
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
#include <cstring>
#include <new>

#include <t3widget/widget.h>
#include <uninorm.h>

using namespace t3_widget;

#include "filestate.h"
#include "filewrapper.h"

bool read_buffer_t::fill_buffer(int used) {
  ssize_t retval;

  if (used < fill) {
    memmove(buffer, buffer + used, fill - used);
    fill -= used;
  } else {
    fill = 0;
  }

  if ((retval = nosig_read(fd, buffer + fill, FILE_BUFFER_SIZE - fill)) < 0) {
    throw rw_result_t(rw_result_t::ERRNO_ERROR, errno);
  }

  fill += retval;
  return fill > 0;
}

bool transcript_buffer_t::fill_buffer(int used) {
  const char *inbuf;
  char *outbuf;
  transcript_error_t retval;

  if (used < fill) {
    memmove(buffer, buffer + used, fill - used);
    fill -= used;
  } else {
    fill = 0;
  }

  if (!at_eof) {  // Don't try to read more bytes when we have already hit EOF
    if (!wrapped_buffer->fill_buffer(buffer_index)) {
      at_eof = true;
      conversion_flags |= TRANSCRIPT_END_OF_TEXT;
    }
    buffer_index = 0;
  }

  if (buffer_index >= wrapped_buffer->get_fill()) {
    return false;
  }

  inbuf = wrapped_buffer->get_buffer() + buffer_index;
  outbuf = buffer + fill;

  retval = transcript_to_unicode(handle, &inbuf,
                                 wrapped_buffer->get_buffer() + wrapped_buffer->get_fill(), &outbuf,
                                 buffer + FILE_BUFFER_SIZE, conversion_flags);
  buffer_index = inbuf - wrapped_buffer->get_buffer();
  fill = outbuf - buffer;

  switch (retval) {
    case TRANSCRIPT_SUCCESS:
    case TRANSCRIPT_NO_SPACE:
    case TRANSCRIPT_INCOMPLETE:
      break;

    case TRANSCRIPT_FALLBACK:
    case TRANSCRIPT_UNASSIGNED:
    case TRANSCRIPT_PRIVATE_USE:
      conversion_flags |= TRANSCRIPT_ALLOW_FALLBACK | TRANSCRIPT_SUBST_UNASSIGNED;
      throw rw_result_t(rw_result_t::CONVERSION_IMPRECISE);

    case TRANSCRIPT_ILLEGAL:
      conversion_flags |= TRANSCRIPT_SUBST_ILLEGAL;
      throw rw_result_t(rw_result_t::CONVERSION_ILLEGAL);

    case TRANSCRIPT_ILLEGAL_END:
      throw rw_result_t(rw_result_t::CONVERSION_TRUNCATED);

    case TRANSCRIPT_INTERNAL_ERROR:
    default:
      throw rw_result_t(rw_result_t::CONVERSION_ERROR);
  }
  if (buffer_index > 0) {
    conversion_flags &= ~TRANSCRIPT_FILE_START;
  }
  return fill > 0;
}

transcript_buffer_t::~transcript_buffer_t() {
  transcript_close_converter(handle);
  delete wrapped_buffer;
}

file_read_wrapper_t::file_read_wrapper_t(int fd, transcript_t *handle) {
  buffer = new read_buffer_t(fd);
  if (handle != nullptr) {
    buffer_t *transcript_buffer = new transcript_buffer_t(buffer, handle);
    buffer = transcript_buffer;
  }
}

file_read_wrapper_t::~file_read_wrapper_t() { delete buffer; }

const char *file_read_wrapper_t::get_buffer() { return buffer->get_buffer(); }

int file_read_wrapper_t::get_fill() { return buffer->get_fill(); }

bool file_read_wrapper_t::fill_buffer(int used) { return buffer->fill_buffer(used); }

void file_write_wrapper_t::write(const char *buffer, size_t bytes) {
  std::unique_ptr<char, free_deleter> nfc_output;
  size_t nfc_output_len;

  char transcript_buffer[FILE_BUFFER_SIZE], *transcript_buffer_ptr;
  const char *buffer_end, *transcript_buffer_end;
  bool imprecise = false;

  // Convert to NFC before writing
  // FIXME: check return value
  nfc_output.reset(reinterpret_cast<char *>(u8_normalize(
      UNINORM_NFC, reinterpret_cast<const uint8_t *>(buffer), bytes, nullptr, &nfc_output_len)));
  if (handle == nullptr) {
    if (nosig_write(fd, nfc_output.get(), nfc_output_len) < 0) {
      throw rw_result_t(rw_result_t::ERRNO_ERROR, errno);
    }
    return;
  }

  transcript_buffer_end = transcript_buffer + FILE_BUFFER_SIZE;
  buffer = nfc_output.get();
  buffer_end = nfc_output.get() + nfc_output_len;

  while (buffer < buffer_end) {
    transcript_buffer_ptr = transcript_buffer;
    switch (transcript_from_unicode(handle, &buffer, buffer_end, &transcript_buffer_ptr,
                                    transcript_buffer_end, conversion_flags)) {
      case TRANSCRIPT_SUCCESS:
        ASSERT(buffer == buffer_end);
        break;
      case TRANSCRIPT_NO_SPACE:
        break;
      case TRANSCRIPT_FALLBACK:
      case TRANSCRIPT_UNASSIGNED:
      case TRANSCRIPT_PRIVATE_USE:
        imprecise = true;
        conversion_flags |= TRANSCRIPT_ALLOW_FALLBACK | TRANSCRIPT_SUBST_UNASSIGNED;
        break;
      case TRANSCRIPT_INCOMPLETE:
      default:
        throw rw_result_t(rw_result_t::CONVERSION_ERROR);
    }
    if (transcript_buffer_ptr > transcript_buffer) {
      conversion_flags &= ~TRANSCRIPT_FILE_START;
      if (nosig_write(fd, transcript_buffer, transcript_buffer_ptr - transcript_buffer) < 0) {
        throw rw_result_t(rw_result_t::ERRNO_ERROR, errno);
      }
    }
  }

  if (imprecise) {
    throw rw_result_t(rw_result_t::CONVERSION_IMPRECISE);
  }
}

file_write_wrapper_t::~file_write_wrapper_t() {
  if (handle != nullptr) {
    transcript_close_converter(handle);
  }
}
