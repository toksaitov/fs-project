#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#endif

#define TB_IMPL
#if defined(_WIN32) || defined(WINDOWS)
#define fileno _fileno
#include "vendor/termbox2_win.h"
#else
#include "vendor/termbox2.h"
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "krffs_file_system.h"
#include "krffs_node.h"
#include "krffs_platform.h"
#include "krffs_utilities.h"

#ifdef WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif

struct viz_buffer {
    char *data;
    int width;
    int height;
};

static void viz_buffer_init(
        struct viz_buffer *buffer,
        int width,
        int height
    )
{
    buffer->width = width;
    buffer->height = height;
    buffer->data = malloc((size_t) width * (size_t) height);
    if (buffer->data) {
        memset(buffer->data, ' ', (size_t) width * (size_t) height);
    }
}

static void viz_buffer_free(struct viz_buffer *buffer)
{
    free(buffer->data);
    buffer->data = NULL;
    buffer->width = 0;
    buffer->height = 0;
}

static void viz_buffer_set_char(
        struct viz_buffer *buffer,
        int x,
        int y,
        char ch
    )
{
    if (x >= 0 && x < buffer->width &&
        y >= 0 && y < buffer->height) {
        buffer->data[y * buffer->width + x] = ch;
    }
}

static char viz_buffer_get_char(
        struct viz_buffer *buffer,
        int x,
        int y
    )
{
    if (x >= 0 && x < buffer->width &&
        y >= 0 && y < buffer->height) {
        return buffer->data[y * buffer->width + x];
    }
    return ' ';
}

static void viz_buffer_print_string(
        struct viz_buffer *buffer,
        int x,
        int y,
        const char *str
    )
{
    while (*str) {
        viz_buffer_set_char(buffer, x++, y, *str++);
    }
}

static void viz_buffer_render_to_terminal(
        struct viz_buffer *buffer,
        int shift_x,
        int shift_y,
        int screen_y_offset,
        int screen_width,
        int screen_height
    )
{
    for (int screen_y = 0; screen_y < screen_height; ++screen_y) {
        int buffer_y = shift_y + screen_y;
        for (int screen_x = 0; screen_x < screen_width; ++screen_x) {
            int buffer_x = shift_x + screen_x;
            char ch = viz_buffer_get_char(buffer, buffer_x, buffer_y);
            tb_set_cell(screen_x, screen_y + screen_y_offset, (uint32_t) ch, TB_DEFAULT, TB_DEFAULT);
        }
    }
}

static void tb_print_string(
        int x,
        int y,
        const char *str,
        int max_width
    )
{
    int count = 0;
    while (*str && count < max_width) {
        tb_set_cell(x++, y, (uint32_t) (unsigned char) *str++, TB_DEFAULT, TB_DEFAULT);
        ++count;
    }
}

/*
    fsviz.krffs

    Visualize the KRFFS filesystem for debugging or educational purposes.

    Usage:
        fsviz.krffs -h
        fsviz.krffs <file> [-t]

    Options:
        -h    show help and exit
        -t    output text visualization to stdout instead of interactive mode
 */
int main(int argc, char **argv)
{
    int exit_status =
        EXIT_SUCCESS;

    struct viz_buffer buffer = {
        .data = NULL,
        .width = 0,
        .height = 0
    };
    bool tb_initialized = false;

    int file_descriptor = -1;
    struct krffs_file_system file_system = {
        .node = NULL
    };

    bool has_help_option = false;
    bool has_text_option = false;
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "-h", 2) == 0) {
            has_help_option = true;
            break;
        } else if (strncmp(argv[i], "-t", 2) == 0) {
            has_text_option = true;
        }
    }

    if (argc <= 1 || has_help_option) {
        fprintf(
            stderr,
            "Usage: %s <file> [-t]\n",
            argc >= 1 ?
                argv[0] : "fsviz.krffs"
        );

        goto cleanup;
    } else if (argv[1][0] == '-') {
        fprintf(
            stderr,
            "The first parameter is invalid.\n"
            "\n"
            "Usage: %s <file> [-t]\n",
            argc >= 1 ?
                argv[0] : "fsviz.krffs"
        );

        exit_status =
            EXIT_FAILURE;

        goto cleanup;
    }

    char *path =
        argv[1];

    /*
        Open the file with the file system.
     */
    if ((file_descriptor = PLATFORM_PREFIX(open(path, O_RDWR))) == -1) {
        fprintf(
            stderr,
            "Failed to open the file system file at '%s'.\n",
            path
        );

        exit_status =
            EXIT_FAILURE;

        goto cleanup;
    }

    /*
        Get file information.
     */
    struct stat file_info;
    if (fstat(file_descriptor, &file_info) == -1) {
        fprintf(
            stderr,
            "Failed to get file information for '%s'.\n",
            path
        );

        exit_status =
            EXIT_FAILURE;

        goto cleanup;
    }

    /*
        Check that we have a regular file (not a directory or a socket).
     */
    if (!S_ISREG(file_info.st_mode)) {
        fprintf(
            stderr,
            "The file system file at '%s' is not a regular file.\n",
            path
        );

        exit_status =
            EXIT_FAILURE;

        goto cleanup;
    }

    /*
        Check that the file is big enough to contain a file system.
     */
    if (file_info.st_size < (PLATFORM_OFF_T)(sizeof(*file_system.node) * 2)) {
        fprintf(
            stderr,
            "The file at '%s' is not big enough to contain a file system.\n",
            path
        );

        exit_status =
            EXIT_FAILURE;

        goto cleanup;
    }

    /*
        Save the size of the file.
     */
    file_system.size =
        file_info.st_size;

    /*
        Map the file system file into memory.
     */
    if ((file_system.node =
             krffs_map_file(
                 file_descriptor,
                 0,
                 file_system.size
             )) == (void *) -1) {
        fprintf(
            stderr,
            "Failed to map the file system file at '%s' into memory.\n",
            path
        );

        exit_status =
            EXIT_FAILURE;

        goto cleanup;
    }

    /*
        It's possible to close the file after a call to `krffs_map_file`. Memory
        pages will still be mapped to the file.
     */
    if (PLATFORM_PREFIX(close(file_descriptor)) == -1) {
        fprintf(
            stderr,
            "Failed to close the file system file at '%s'.\n",
            path
        );

        exit_status =
            EXIT_FAILURE;

        goto cleanup;
    }
    file_descriptor = -1;

    /*
        Try to visualize the file system.
     */
    if (!has_text_option) {
        if (tb_init() != 0) {
            fprintf(
                stderr,
                "Failed to initialize the terminal interface.\n"
            );

            exit_status =
                EXIT_FAILURE;

            goto cleanup;
        }
        tb_initialized = true;
    }

    size_t bytes_per_scr_chr =
        524288;
    size_t prev_bytes_per_scr_chr =
        0;

    int window_height = 0;
    int window_width = 0;
    int pad_shift_x = 0;
    int viz_width = 0;

    static const int H_MARGIN = 5;
    static const int V_MARGIN = 2;
    static const int ARROW_HEIGHT = 2;
    static const int MAX_SCROLL_PERCENT = 90;
    static const size_t MAX_BARS_PER_NODE = 60000;

    size_t largest_node_size = 0;
    {
        struct krffs_node *node = file_system.node;
        do {
            if ((size_t) node->size > largest_node_size) {
                largest_node_size = (size_t) node->size;
            }
        } while ((node =
                    krffs_get_next_node(
                        &file_system,
                        node
                    )) != file_system.node);
    }

    size_t min_bytes_per_scr_chr = largest_node_size / MAX_BARS_PER_NODE;
    if (min_bytes_per_scr_chr < 2) {
        min_bytes_per_scr_chr = 2;
    }
    size_t max_bytes_per_scr_chr = (size_t) file_system.size;

    for (;;) {
        if (bytes_per_scr_chr != prev_bytes_per_scr_chr) {
            prev_bytes_per_scr_chr = bytes_per_scr_chr;

            size_t longest_file_name = 0;

            struct krffs_node *node = file_system.node;
            do {
                if (node->type == KRFFS_Reserved_Node) {
                    size_t file_name_length = strlen((char *) node->name);
                    if (file_name_length > longest_file_name) {
                        longest_file_name =
                            file_name_length;
                    }
                }
            } while ((node =
                        krffs_get_next_node(
                            &file_system,
                            node
                        )) != file_system.node);

            size_t bar_length = 0;

            node = file_system.node;
            do {
                if (node->type == KRFFS_Root_Node) {
                    bar_length += strlen("[* ]");
                    bar_length += longest_file_name;
                } else if (node->type == KRFFS_Free_Node) {
                    bar_length += strlen("[- : ]");
                    bar_length += longest_file_name;
                    bar_length += (size_t) node->size / bytes_per_scr_chr;
                    if ((size_t) node->size % bytes_per_scr_chr > 0) {
                        ++bar_length;
                    }
                } else if (node->type == KRFFS_Reserved_Node) {
                    bar_length += strlen("[# : ]");
                    bar_length += longest_file_name;
                    bar_length += (size_t) node->size / bytes_per_scr_chr;
                    if ((size_t) node->size % bytes_per_scr_chr > 0) {
                        ++bar_length;
                    }
                }
            } while ((node =
                        krffs_get_next_node(
                            &file_system,
                            node
                        )) != file_system.node);

            if (has_text_option) {
                window_height = 20;
                window_width = 80;
            } else {
                window_height = tb_height();
                window_width = tb_width();
            }

            viz_width =
                (int) bar_length + H_MARGIN * 2;
            int buffer_width = viz_width;
            if (buffer_width < window_width) {
                buffer_width = window_width;
            }
            buffer_width += 5;

            int buffer_height =
                V_MARGIN * 2 + ARROW_HEIGHT * 4 + 3;
            if (buffer_height < window_height) {
                buffer_height = window_height;
            }

            viz_buffer_free(&buffer);
            viz_buffer_init(&buffer, buffer_width, buffer_height);
            if (!buffer.data) {
                fprintf(
                    stderr,
                    "Failed to allocate visualization buffer.\n"
                );

                exit_status =
                    EXIT_FAILURE;

                goto cleanup;
            }

            int arrow_v_dir = 1;
            int init_cursor_x = H_MARGIN;
            int cursor_x = init_cursor_x;
            int cursor_y = V_MARGIN + ARROW_HEIGHT * 2;

            #define SCRATCHPAD_BUFFER_SIZE 65536
            static char scratchpad[SCRATCHPAD_BUFFER_SIZE];

            node = file_system.node;
            struct krffs_node *next_node = node;
            do {
                next_node =
                    krffs_get_next_node(
                        &file_system,
                        node
                    );

                size_t chars_written = 0;
                if (node->type == KRFFS_Root_Node) {
                    chars_written =
                        (size_t) snprintf(
                            scratchpad, sizeof(scratchpad),
                            "[* %*s]", (int) longest_file_name, "root node"
                        );
                } else if (node->type == KRFFS_Free_Node) {
                    chars_written =
                        (size_t) snprintf(
                            scratchpad, sizeof(scratchpad),
                            "[- %*s: ", (int) longest_file_name, "free node"
                        );
                    size_t bars = (size_t) node->size / bytes_per_scr_chr;
                    if ((size_t) node->size % bytes_per_scr_chr > 0) {
                        ++bars;
                    }
                    for (size_t i = 0; i < bars; ++i) {
                        strcat(scratchpad, "-");
                    }
                    strcat(scratchpad, "]");
                    chars_written += bars + 1;
                } else if (node->type == KRFFS_Reserved_Node) {
                    chars_written =
                        (size_t) snprintf(
                            scratchpad, sizeof(scratchpad),
                            "[# %*s: ", (int) longest_file_name, (char *) node->name
                        );
                    size_t bars = (size_t) node->size / bytes_per_scr_chr;
                    if ((size_t) node->size % bytes_per_scr_chr > 0) {
                        ++bars;
                    }
                    for (size_t i = 0; i < bars; ++i) {
                        strcat(scratchpad, "#");
                    }
                    strcat(scratchpad, "]");
                    chars_written += bars + 1;
                }
                viz_buffer_print_string(&buffer, cursor_x, cursor_y, scratchpad);
                {
                    int x = cursor_x + 1;
                    int y = cursor_y + arrow_v_dir;
                    viz_buffer_set_char(&buffer, x, y, arrow_v_dir < 0 ? '^' : 'v');
                    y += arrow_v_dir;

                    if (next_node != file_system.node) {
                        for (int i = 1; i < ARROW_HEIGHT; ++i, y += arrow_v_dir) {
                            viz_buffer_set_char(&buffer, x, y, '|');
                        }
                        for (size_t i = 0; i < chars_written; ++i, ++x) {
                            viz_buffer_set_char(&buffer, x, y, '-');
                        }
                        viz_buffer_set_char(&buffer, x, y, '-');
                        y += -arrow_v_dir;
                        for (int i = 0; i < ARROW_HEIGHT - 1; ++i, y += -arrow_v_dir) {
                            viz_buffer_set_char(&buffer, x, y, '|');
                        }
                        viz_buffer_set_char(&buffer, x, y, arrow_v_dir < 0 ? 'v' : '^');
                    } else {
                        for (int i = 1; i < ARROW_HEIGHT * 2; ++i, y += arrow_v_dir) {
                            viz_buffer_set_char(&buffer, x, y, '|');
                        }
                        for (int i = 0; i < cursor_x - init_cursor_x + 4; ++i, --x) {
                            viz_buffer_set_char(&buffer, x, y, '-');
                        }
                        viz_buffer_set_char(&buffer, x, y, '-');
                        y += -arrow_v_dir;
                        for (int i = 0; i < ARROW_HEIGHT * 2; ++i, y += -arrow_v_dir) {
                            viz_buffer_set_char(&buffer, x, y, '|');
                        }
                        viz_buffer_set_char(&buffer, x++, y, '-');
                        viz_buffer_set_char(&buffer, x++, y, '-');
                        viz_buffer_set_char(&buffer, x++, y, '>');
                    }

                    arrow_v_dir = -arrow_v_dir;
                }
                cursor_x += (int) chars_written;
            } while ((node =
                        krffs_get_next_node(
                            &file_system,
                            node
                        )) != file_system.node);
        }

        if (has_text_option) {
            break;
        }

        tb_clear();

        static char title[256];
        snprintf(title, sizeof(title), "fsviz.krffs: %s", path);
        tb_print_string(0, 0, title, window_width);

        int viz_area_height = window_height - 2;
        if (viz_area_height > 0) {
            viz_buffer_render_to_terminal(
                &buffer,
                pad_shift_x, 0,
                1,
                window_width, viz_area_height
            );
        }

        static const char *help =
            "Press 'q' or 'ctrl+c' to exit. "
            "Scroll left or right with arrow keys or 'h' and 'l'. "
            "Scale node size with '+' or '-'.";
        tb_print_string(0, window_height - 1, help, window_width);

        tb_present();

        struct tb_event event;
        int result = tb_peek_event(&event, 100);
        if (result == TB_OK) {
            if (event.type == TB_EVENT_KEY) {
                if (event.ch == 'q' || event.key == TB_KEY_CTRL_C) {
                    goto cleanup;
                } else if (event.ch == 'h' || event.key == TB_KEY_ARROW_LEFT) {
                    --pad_shift_x;
                } else if (event.ch == 'l' || event.key == TB_KEY_ARROW_RIGHT) {
                    ++pad_shift_x;
                } else if (event.ch == '+') {
                    bytes_per_scr_chr /= 2;
                    if (bytes_per_scr_chr < min_bytes_per_scr_chr) {
                        bytes_per_scr_chr = min_bytes_per_scr_chr;
                    }
                } else if (event.ch == '_') {
                    bytes_per_scr_chr *= 2;
                    if (bytes_per_scr_chr > max_bytes_per_scr_chr) {
                        bytes_per_scr_chr = max_bytes_per_scr_chr;
                    }
                }

                if (pad_shift_x < 0) {
                    pad_shift_x = 0;
                }
                int scroll_limit = viz_width * MAX_SCROLL_PERCENT / 100;
                int content_limit = viz_width - window_width;
                int max_shift_x = scroll_limit > content_limit ?
                    scroll_limit : content_limit;
                if (max_shift_x < 0) {
                    max_shift_x = 0;
                }
                if (pad_shift_x > max_shift_x) {
                    pad_shift_x = max_shift_x;
                }
            } else if (event.type == TB_EVENT_RESIZE) {
                window_width = event.w;
                window_height = event.h;
            }
        }
    }

    if (has_text_option && buffer.data) {
        int max_y = 0;
        int max_x = 0;

        for (int y = 0; y < buffer.height; ++y) {
            for (int x = 0; x < buffer.width; ++x) {
                char ch = viz_buffer_get_char(&buffer, x, y);
                if (ch != ' ') {
                    if (y > max_y) {
                        max_y = y;
                    }
                    if (x > max_x) {
                        max_x = x;
                    }
                }
            }
        }

        for (int y = 0; y <= max_y + 1; ++y) {
            for (int x = 0; x <= max_x; ++x) {
                char ch = viz_buffer_get_char(&buffer, x, y);
                putchar(ch);
            }
            putchar('\n');
        }
    }

cleanup:
    if (tb_initialized) {
        tb_shutdown();
    }

    viz_buffer_free(&buffer);

    if (file_descriptor != -1) {
        if (PLATFORM_PREFIX(close(file_descriptor)) == -1) {
            fprintf(
                stderr,
                "Failed to close the file system file.\n"
            );
        }
    }

    if (file_system.node != NULL && file_system.node != (void *) -1) {
        if (krffs_unmap_file(
                file_system.node,
                file_system.size
            ) == -1) {
            fprintf(
                stderr,
                "Failed to unmap the file system file.\n"
            );
        }
    }

    return exit_status;
}
