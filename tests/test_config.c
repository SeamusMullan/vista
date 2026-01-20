/**
 * @file test_config.c
 * @brief Tests for configuration parsing
 */

#include "test_framework.h"
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Helper to create a temporary config file */
static char* create_temp_config(const char *content) {
    static char path[256];
    snprintf(path, sizeof(path), "/tmp/vista_test_config_%d.conf", getpid());
    
    FILE *f = fopen(path, "w");
    if (!f) return NULL;
    fprintf(f, "%s", content);
    fclose(f);
    
    return path;
}

static void cleanup_temp_config(const char *path) {
    if (path) unlink(path);
}

/* -------------------------------------------------------------------------- */
/*                               Test Cases                                    */
/* -------------------------------------------------------------------------- */

TEST(config_default_values) {
    Config config = config_default();
    
    ASSERT_EQ(200, config.thumbnail_width);
    ASSERT_EQ(150, config.thumbnail_height);
    ASSERT_EQ(1200, config.window_width);
    ASSERT_EQ(300, config.window_height);
    ASSERT_EQ(5, config.thumbnails_per_row);
    ASSERT_FALSE(config.use_shaders);
    ASSERT_FALSE(config.use_wal);
    ASSERT_FALSE(config.reload_i3);
    ASSERT_EQ(0, config.wallpaper_dirs_count);
    
    TEST_PASS();
}

TEST(config_parse_wallpaper_dir) {
    const char *content = 
        "wallpaper_dir = /home/test/wallpapers\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_STR_EQ("/home/test/wallpapers", config.wallpaper_dir);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_multiple_wallpaper_dirs) {
    const char *content = 
        "wallpaper_dir = /home/test/wallpapers\n"
        "wallpaper_dir_1 = /home/test/pics\n"
        "wallpaper_dir_2 = /usr/share/backgrounds\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_STR_EQ("/home/test/wallpapers", config.wallpaper_dir);
    ASSERT_EQ(2, config.wallpaper_dirs_count);
    ASSERT_STR_EQ("/home/test/pics", config.wallpaper_dirs[0]);
    ASSERT_STR_EQ("/usr/share/backgrounds", config.wallpaper_dirs[1]);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_feh_command) {
    const char *content = 
        "feh_command = feh --bg-fill\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_STR_EQ("feh --bg-fill", config.feh_command);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_thumbnail_dimensions) {
    const char *content = 
        "thumbnail_width = 300\n"
        "thumbnail_height = 200\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_EQ(300, config.thumbnail_width);
    ASSERT_EQ(200, config.thumbnail_height);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_window_dimensions) {
    const char *content = 
        "window_width = 1920\n"
        "window_height = 600\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_EQ(1920, config.window_width);
    ASSERT_EQ(600, config.window_height);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_boolean_true) {
    const char *content = 
        "use_shaders = true\n"
        "use_wal = 1\n"
        "reload_i3 = true\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_TRUE(config.use_shaders);
    ASSERT_TRUE(config.use_wal);
    ASSERT_TRUE(config.reload_i3);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_boolean_false) {
    const char *content = 
        "use_shaders = false\n"
        "use_wal = 0\n"
        "reload_i3 = no\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_FALSE(config.use_shaders);
    ASSERT_FALSE(config.use_wal);
    ASSERT_FALSE(config.reload_i3);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_thumbnails_per_row) {
    const char *content = 
        "thumbnails_per_row = 8\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_EQ(8, config.thumbnails_per_row);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_comments_ignored) {
    const char *content = 
        "# This is a comment\n"
        "thumbnail_width = 250\n"
        "# Another comment\n"
        "thumbnail_height = 180\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_EQ(250, config.thumbnail_width);
    ASSERT_EQ(180, config.thumbnail_height);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_whitespace_handling) {
    const char *content = 
        "  thumbnail_width  =   350  \n"
        "\tthumbnail_height\t=\t250\t\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_EQ(350, config.thumbnail_width);
    ASSERT_EQ(250, config.thumbnail_height);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_quoted_values) {
    const char *content = 
        "feh_command = \"feh --bg-scale\"\n"
        "post_command = 'echo done'\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_STR_EQ("feh --bg-scale", config.feh_command);
    ASSERT_STR_EQ("echo done", config.post_command);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_roulette_settings) {
    const char *content = 
        "roulette_start_duration = 1000\n"
        "roulette_scroll_duration = 3000\n"
        "roulette_slow_duration = 3500\n"
        "roulette_show_duration = 2000\n"
        "roulette_max_velocity = 100.5\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_EQ(1000, config.roulette_start_duration);
    ASSERT_EQ(3000, config.roulette_scroll_duration);
    ASSERT_EQ(3500, config.roulette_slow_duration);
    ASSERT_EQ(2000, config.roulette_show_duration);
    ASSERT((config.roulette_max_velocity > 100.0f && config.roulette_max_velocity < 101.0f));
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_openrgb_settings) {
    const char *content = 
        "use_openrgb = true\n"
        "openrgb_color_source = static\n"
        "openrgb_static_color = FF5733\n"
        "openrgb_mode = breathing\n"
        "openrgb_brightness = 75\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_TRUE(config.use_openrgb);
    ASSERT_STR_EQ("static", config.openrgb_color_source);
    ASSERT_STR_EQ("FF5733", config.openrgb_static_color);
    ASSERT_STR_EQ("breathing", config.openrgb_mode);
    ASSERT_EQ(75, config.openrgb_brightness);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_monitors) {
    const char *content = 
        "monitor_0 = DP-1\n"
        "monitor_1 = HDMI-1\n"
        "use_per_monitor = true\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_EQ(2, config.monitors_count);
    ASSERT_STR_EQ("DP-1", config.monitors[0]);
    ASSERT_STR_EQ("HDMI-1", config.monitors[1]);
    ASSERT_TRUE(config.use_per_monitor);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_nonexistent_file) {
    // Should return default config without crashing
    Config config = config_parse("/nonexistent/path/config.conf");
    
    // Should have default values
    ASSERT_EQ(200, config.thumbnail_width);
    ASSERT_EQ(150, config.thumbnail_height);
    
    TEST_PASS();
}

TEST(config_parse_empty_file) {
    const char *content = "";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    // Should have default values
    ASSERT_EQ(200, config.thumbnail_width);
    ASSERT_EQ(5, config.thumbnails_per_row);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

TEST(config_parse_full_example) {
    const char *content = 
        "# Vista configuration file\n"
        "wallpaper_dir = /home/user/wallpapers\n"
        "wallpaper_dir_1 = /home/user/Pictures\n"
        "feh_command = feh --bg-scale\n"
        "thumbnail_width = 200\n"
        "thumbnail_height = 150\n"
        "window_width = 1200\n"
        "window_height = 300\n"
        "thumbnails_per_row = 5\n"
        "use_shaders = false\n";
    
    char *path = create_temp_config(content);
    ASSERT(path != NULL);
    
    Config config = config_parse(path);
    ASSERT_STR_EQ("/home/user/wallpapers", config.wallpaper_dir);
    ASSERT_EQ(1, config.wallpaper_dirs_count);
    ASSERT_STR_EQ("feh --bg-scale", config.feh_command);
    ASSERT_EQ(200, config.thumbnail_width);
    ASSERT_EQ(150, config.thumbnail_height);
    ASSERT_EQ(1200, config.window_width);
    ASSERT_EQ(300, config.window_height);
    ASSERT_EQ(5, config.thumbnails_per_row);
    ASSERT_FALSE(config.use_shaders);
    
    cleanup_temp_config(path);
    TEST_PASS();
}

/* -------------------------------------------------------------------------- */
/*                                Main Runner                                  */
/* -------------------------------------------------------------------------- */

int main(void) {
    TEST_SUITE_BEGIN("Config Parsing Tests");
    
    RUN_TEST(config_default_values);
    RUN_TEST(config_parse_wallpaper_dir);
    RUN_TEST(config_parse_multiple_wallpaper_dirs);
    RUN_TEST(config_parse_feh_command);
    RUN_TEST(config_parse_thumbnail_dimensions);
    RUN_TEST(config_parse_window_dimensions);
    RUN_TEST(config_parse_boolean_true);
    RUN_TEST(config_parse_boolean_false);
    RUN_TEST(config_parse_thumbnails_per_row);
    RUN_TEST(config_parse_comments_ignored);
    RUN_TEST(config_parse_whitespace_handling);
    RUN_TEST(config_parse_quoted_values);
    RUN_TEST(config_parse_roulette_settings);
    RUN_TEST(config_parse_openrgb_settings);
    RUN_TEST(config_parse_monitors);
    RUN_TEST(config_parse_nonexistent_file);
    RUN_TEST(config_parse_empty_file);
    RUN_TEST(config_parse_full_example);
    
    TEST_SUITE_END();
    RETURN_TEST_RESULT();
}
