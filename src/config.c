/*
   FSearch - A fast file search utility
   Copyright © 2016 Christian Boxdörfer

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
   */

#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include "config.h"

const char *config_file_name = "fsearch.conf";
const char *config_folder_name = "fsearch";

void
build_config_dir (char *path, size_t len)
{
    g_assert (path != NULL);
    g_assert (len >= 0);

    const gchar *xdg_conf_dir = g_get_user_config_dir ();
    snprintf (path, len, "%s/%s", xdg_conf_dir, config_folder_name);
    return;
}

static void
build_config_path (char *path, size_t len)
{
    g_assert (path != NULL);
    g_assert (len >= 0);

    const gchar *xdg_conf_dir = g_get_user_config_dir ();
    snprintf (path, len, "%s/%s/%s", xdg_conf_dir, config_folder_name, config_file_name);
    return;
}

bool
make_config_dir (void)
{
    gchar config_dir[PATH_MAX] = "";
    build_config_dir (config_dir, sizeof (config_dir));
    return !g_mkdir_with_parents (config_dir, 0700);
}

static void
config_load_handle_error (GError *error)
{
    if (!error) {
        return;
    }
    switch (error->code) {
        case G_KEY_FILE_ERROR_INVALID_VALUE:
            fprintf(stderr, "load_config: invalid value: %s\n", error->message);
            break;
        case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
        case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
            // new config, use default value and don't report anything
            break;
        default:
            fprintf(stderr, "load_config: unkown error: %s\n", error->message);
    }
    g_error_free (error);
}

static uint32_t
config_load_integer (GKeyFile *key_file,
                     const char *group_name,
                     const char *key,
                     uint32_t default_value)
{
    GError *error = NULL;
    uint32_t result = g_key_file_get_integer (key_file, group_name, key, &error);
    if (error != NULL) {
        result = default_value;
        config_load_handle_error (error);
    }
    return result;
}

static bool
config_load_boolean (GKeyFile *key_file,
                     const char *group_name,
                     const char *key,
                     bool default_value)
{
    GError *error = NULL;
    bool result = g_key_file_get_boolean (key_file, group_name, key, &error);
    if (error != NULL) {
        result = default_value;
        config_load_handle_error (error);
    }
    return result;
}

static char *
config_load_string (GKeyFile *key_file,
                    const char *group_name,
                    const char *key,
                    char *default_value)
{
    GError *error = NULL;
    char *result = g_key_file_get_string (key_file, group_name, key, &error);
    if (error != NULL) {
        result = default_value;
        config_load_handle_error (error);
    }
    return result;
}

bool
load_config (FsearchConfig *config)
{
    g_assert (config != NULL);

    bool result = false;
    GKeyFile *key_file = g_key_file_new ();
    g_assert (key_file != NULL);

    gchar config_path[PATH_MAX] = "";
    build_config_path (config_path, sizeof (config_path));

    GError *error = NULL;
    if (g_key_file_load_from_file (key_file, config_path, G_KEY_FILE_NONE, &error)) {
        printf("loaded config file\n");
        // Interface
        config->enable_list_tooltips = config_load_boolean (key_file,
                                                            "Interface",
                                                            "enable_list_tooltips",
                                                            true);
        config->enable_dark_theme = config_load_boolean (key_file,
                                                         "Interface",
                                                         "enable_dark_theme",
                                                         false);
        config->show_menubar = config_load_boolean (key_file,
                                                    "Interface",
                                                    "show_menubar",
                                                    true);
        config->show_statusbar = config_load_boolean (key_file,
                                                      "Interface",
                                                      "show_statusbar",
                                                      true);
        config->show_filter = config_load_boolean (key_file,
                                                   "Interface",
                                                   "show_filter",
                                                   true);
        config->show_search_button = config_load_boolean (key_file,
                                                          "Interface",
                                                          "show_search_button",
                                                          true);
        // Search
        config->match_case = config_load_boolean (key_file,
                                                  "Search",
                                                  "match_case",
                                                  false);
        config->enable_regex = config_load_boolean (key_file,
                                                    "Search",
                                                    "enable_regex",
                                                    false);
        config->search_in_path = config_load_boolean (key_file,
                                                      "Search",
                                                      "search_in_path",
                                                      false);
        config->limit_results = config_load_boolean (key_file,
                                                     "Search",
                                                     "limit_results",
                                                     false);
        config->num_results = config_load_integer (key_file,
                                                   "Search",
                                                   "num_results",
                                                   10000);

        // Locations
        uint32_t pos = 1;
        while (true) {
            char key[100] = "";
            snprintf (key, sizeof (key), "location_%d", pos++);
            char *value = config_load_string (key_file, "Database", key, NULL);
            if (value) {
                config->locations = g_list_append (config->locations, value);
            }
            else {
                break;
            }
        }

        result = true;
    }
    else {
        fprintf(stderr, "load config failed: %s\n", error->message);
        g_error_free (error);
    }

    g_key_file_free (key_file);
    return result;
}

bool
load_default_config (FsearchConfig *config)
{
    g_assert (config != NULL);

    // Search
    config->match_case = false;
    config->enable_regex = false;
    config->search_in_path = false;
    config->limit_results = true;
    config->num_results = 10000;

    // Interface
    config->enable_dark_theme = false;
    config->enable_list_tooltips = true;
    config->show_menubar = true;
    config->show_statusbar = true;
    config->show_filter = true;
    config->show_search_button = true;

    // Locations
    config->locations = NULL;

    return true;
}

bool
save_config (FsearchConfig *config)
{
    g_assert (config != NULL);

    bool result = false;
    GKeyFile *key_file = g_key_file_new ();
    g_assert (key_file != NULL);

    // Interface
    g_key_file_set_boolean (key_file, "Interface", "enable_list_tooltips", config->enable_list_tooltips);
    g_key_file_set_boolean (key_file, "Interface", "enable_dark_theme", config->enable_dark_theme);
    g_key_file_set_boolean (key_file, "Interface", "show_menubar", config->show_menubar);
    g_key_file_set_boolean (key_file, "Interface", "show_statusbar", config->show_statusbar);
    g_key_file_set_boolean (key_file, "Interface", "show_filter", config->show_filter);
    g_key_file_set_boolean (key_file, "Interface", "show_search_button", config->show_search_button);

    // Search
    g_key_file_set_boolean (key_file, "Search", "search_in_path", config->search_in_path);
    g_key_file_set_boolean (key_file, "Search", "enable_regex", config->enable_regex);
    g_key_file_set_boolean (key_file, "Search", "match_case", config->match_case);
    g_key_file_set_boolean (key_file, "Search", "limit_results", config->limit_results);
    g_key_file_set_integer (key_file, "Search", "num_results", config->num_results);

    if (config->locations) {
        uint32_t pos = 1;
        for (GList *l = config->locations; l != NULL; l = l->next) {
            char location[100] = "";
            snprintf (location, sizeof (location), "location_%d", pos);
            g_key_file_set_string (key_file, "Database", location, l->data);
            pos++;
        }
    }

    gchar config_path[PATH_MAX] = "";
    build_config_path (config_path, sizeof (config_path));

    GError *error = NULL;
    if (g_key_file_save_to_file (key_file, config_path, &error)) {
        printf("saved config file\n");
        result = true;
    }
    else {
        fprintf(stderr, "save config failed: %s\n", error->message);
    }

    g_key_file_free (key_file);
    return result;
}

void
config_free (FsearchConfig *config)
{
    g_assert (config != NULL);

    if (config->locations) {
        g_list_free_full (config->locations, (GDestroyNotify)free);
        config->locations = NULL;
    }
    free (config);
    config = NULL;
}
