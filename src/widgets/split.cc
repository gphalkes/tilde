/* Copyright (C) 2011 G.P. Halkes
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
#include "widgets/split.h"

using namespace std;

split_t::split_t(widget_t *widget, bool _horizontal) : horizontal(_horizontal) {
	init_unbacked_window(3, 3);
	set_widget_parent(widget);
	widget->set_anchor(this, 0);
	widget->show();
	widgets.push_back(widget);
	current = widgets.begin();
}

split_t::~split_t(void) {
	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		delete (*iter);
}

bool split_t::process_key(key_t key) {
	split_t *current_window;

	if (widgets.empty())
		return false;

	switch (key) {
		case EKEY_F8:
			current_window = dynamic_cast<split_t *>(*current);
			if (current_window == NULL || !current_window->next()) {
				(*current)->set_focus(false);
				current++;
				if (current == widgets.end())
					current = widgets.begin();

				current_window = dynamic_cast<split_t *>(*current);
				if (current_window != NULL)
					current_window->current = current_window->widgets.begin();
				(*current)->set_focus(focus);
			}
			break;
		case EKEY_F8 | EKEY_SHIFT:
			current_window = dynamic_cast<split_t *>(*current);
			if (current_window == NULL || !current_window->previous()) {
				(*current)->set_focus(false);
				if (current == widgets.begin())
					current = widgets.end();
				current--;

				current_window = dynamic_cast<split_t *>(*current);
				if (current_window != NULL) {
					current_window->current = current_window->widgets.end();
					current_window->current--;
				}
				(*current)->set_focus(focus);
			}
			break;
		default:
			return (*current)->process_key(key);
	}
	return true;
}

bool split_t::set_size(optint height, optint width) {
	bool result;

	if (!height.is_valid())
		height = t3_win_get_height(window);
	if (!width.is_valid())
		width = t3_win_get_width(window);

	result = t3_win_resize(window, height, width);

	if (horizontal) {
		int idx;
		int step = height / widgets.size();
		int left_over = height % widgets.size();
		widgets_t::iterator iter;

		for (iter = widgets.begin(), idx = 0; iter != widgets.end(); iter++, idx++) {
			result &= (*iter)->set_size(step + (idx < left_over), width);
			(*iter)->set_position(idx * step + min(idx, left_over), 0);
		}
	} else {
		int idx;
		int step = width / widgets.size();
		int left_over = width % widgets.size();
		widgets_t::iterator iter;

		for (iter = widgets.begin(), idx = 0; iter != widgets.end(); iter++, idx++) {
			result &= (*iter)->set_size(height, step + (idx < left_over));
			(*iter)->set_position(0, idx * step + min(idx, left_over));
		}
	}
	return result;
}

void split_t::update_contents(void) {
	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		(*iter)->update_contents();
}

void split_t::set_focus(bool _focus) {
	focus = _focus;
	(*current)->set_focus(focus);
}

void split_t::split(widget_t *widget, bool _horizontal) {
	split_t *current_window = dynamic_cast<split_t *>(*current);

	if (current_window != NULL) {
		current_window->split(widget, _horizontal);
	} else if (_horizontal == horizontal) {
		set_widget_parent(widget);
		widget->set_anchor(this, 0);
		widget->show();
		if (focus)
			(*current)->set_focus(false);
		current++;
		current = widgets.insert(current, widget);
		set_size(None, None);
		if (focus)
			(*current)->set_focus(true);
	} else {
		/* Create a new split_t with the current widget as its contents. Then
		   add split that split_t to splice in the requested widget. */
		current_window = new split_t(*current, _horizontal);
		current_window->set_focus(focus);
		current_window->split(widget, _horizontal);
		*current = current_window;
	}
}

bool split_t::unsplit(void) {
	split_t *current_window = dynamic_cast<split_t *>(*current);

	if (current_window == NULL) {
		/* This should not happen for previously split windows. However, for
		   the first split_t instance this may be the case, so we have to
		   handle it. */
		if (widgets.size() == 1)
			return true;
		delete (*current);
		current = widgets.erase(current);
		if (current == widgets.end())
			current--;
		if (focus)
			(*current)->set_focus(true);
		set_size(None, None);
		if (widgets.size() == 1)
			return true;
	} else {
		if (current_window->unsplit()) {
			*current = current_window->widgets.front();
			current_window->widgets.clear();
			delete current_window;
			if (focus)
				(*current)->set_focus(true);
		}
	}
	return false;
}

bool split_t::next(void) {
	split_t *current_window = dynamic_cast<split_t *>(*current);
	if (current_window == NULL || !current_window->next()) {
		(*current)->set_focus(false);
		current++;
		if (current != widgets.end()) {
			current_window = dynamic_cast<split_t *>(*current);
			if (current_window != NULL)
				current_window->current = current_window->widgets.begin();
			(*current)->set_focus(true);
			return true;
		} else {
			current--;
			return false;
		}
	}
	return true;
}

bool split_t::previous(void) {
	split_t *current_window = dynamic_cast<split_t *>(*current);
	if (current_window == NULL || !current_window->previous()) {
		if (current == widgets.begin())
			return false;
		(*current)->set_focus(false);
		current--;

		current_window = dynamic_cast<split_t *>(*current);
		if (current_window != NULL) {
			current_window->current = current_window->widgets.end();
			current_window->current--;
		}
		(*current)->set_focus(true);
	}
	return true;
}

widget_t *split_t::get_current(void) {
	split_t *current_window = dynamic_cast<split_t *>(*current);
	if (current_window == NULL)
		return (edit_window_t *) *current;
	else
		return current_window->get_current();
}

