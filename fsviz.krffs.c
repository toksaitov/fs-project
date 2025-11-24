#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ncurses.h>

#include "krffs_file_system.h"
#include "krffs_node.h"
#include "krffs_platform.h"
#include "krffs_utilities.h"

#ifdef WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif

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

    WINDOW *pad =
        NULL;
    char *output_buffer =
        NULL;

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
    if (file_info.st_size < sizeof(*file_system.node) * 2) {
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
        Map the file system file into memory. Changes to memory at
        `file_system.node` after a successful call to `krffs_map_file` will be
        written directly to a file (right away or after calls to
        `krffs_unmap_file` or `krffs_sync_mapping`).

        The kernel uses its virtual memory system to implement the memory
        mapping.
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

    size_t bytes_per_scr_chr =
        524288;
    size_t prev_bytes_per_scr_chr =
        0;

    int window_height, window_width;
    int pad_width;
    int pad_shift_y = 0;
    int pad_shift_x = 0;

    for (;;) {
        if (bytes_per_scr_chr != prev_bytes_per_scr_chr) {
            prev_bytes_per_scr_chr = bytes_per_scr_chr;
            size_t longest_file_name = 0;
            size_t max_node_size     = 0;

            struct krffs_node *node = file_system.node;
            do {
                if (node->type == KRFFS_Reserved_Node) {
                    size_t file_name_length = strlen((char *) node->name);
                    if (file_name_length > longest_file_name) {
                        longest_file_name =
                            file_name_length;
                    }
                }

                if ((size_t) node->size > max_node_size) {
                    max_node_size =
                        (size_t) node->size;
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

            if (!initscr()) {
                fprintf(
                    stderr,
                    "Failed to create the program's UI.\n"
                );

                exit_status =
                    EXIT_FAILURE;

                goto cleanup;
            }

            if (has_colors()) {
                start_color();
                use_default_colors();
                init_pair(1, COLOR_BLACK, COLOR_WHITE);
            }
            noecho();
            static const unsigned int Input_Delay = 10;
            halfdelay(Input_Delay);

            int h_margin = 5, v_margin = 2;
            int arrow_height = 2;
            int arrow_v_dir = 1;
            getmaxyx(stdscr, window_height, window_width);

            pad_width =
                bar_length + h_margin * 2 > window_width ?
                    bar_length + h_margin * 2 : window_width;

            pad = newpad(window_height, pad_width + 5);
            if (!pad) {
                fprintf(
                    stderr,
                    "Failed to create a UI scrollable area.\n"
                );

                exit_status =
                    EXIT_FAILURE;

                goto cleanup;
            }
            keypad(pad, true);
            werase(pad);

            int init_cursor_x = h_margin;
            int cursor_x = init_cursor_x;
            int cursor_y = v_margin + arrow_height * 2;

            #define SCRATCHPAD_BUFFER_SIZE 2048
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
                        snprintf(
                            scratchpad, sizeof(scratchpad),
                            "[* %*s]", (int) longest_file_name, "root node"
                        );
                } else if (node->type == KRFFS_Free_Node) {
                    chars_written +=
                        snprintf(
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
                    chars_written +=
                        snprintf(
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
                mvwprintw(pad, cursor_y, cursor_x, "%s", scratchpad);
                {
                    int x = cursor_x + 1;
                    int y = cursor_y + arrow_v_dir;
                    mvwaddch(pad, y, x, arrow_v_dir < 0 ? ACS_UARROW : ACS_DARROW);
                    y += arrow_v_dir;

                    if (next_node != file_system.node) {
                        for (int i = 1; i < arrow_height; ++i, y += arrow_v_dir) {
                            mvwaddch(pad, y, x, '|');
                        }
                        for (int i = 0; i < chars_written; ++i, ++x) {
                            mvwaddch(pad, y, x, '-');
                        }
                        mvwaddch(pad, y, x, '-');
                        y += -arrow_v_dir;
                        for (int i = 0; i < arrow_height - 1; ++i, y += -arrow_v_dir) {
                            mvwaddch(pad, y, x, '|');
                        }
                        mvwaddch(pad, y, x, arrow_v_dir < 0 ? ACS_DARROW : ACS_UARROW);
                    } else {
                        for (int i = 1; i < arrow_height * 2; ++i, y += arrow_v_dir) {
                            mvwaddch(pad, y, x, '|');
                        }
                        for (int i = 0; i < cursor_x - init_cursor_x + 4; ++i, --x) {
                            mvwaddch(pad, y, x, '-');
                        }
                        mvwaddch(pad, y, x, '-');
                        y += -arrow_v_dir;
                        for (int i = 0; i < arrow_height * 2; ++i, y += -arrow_v_dir) {
                            mvwaddch(pad, y, x, '|');
                        }
                        mvwaddch(pad, y, x++, '-');
                        mvwaddch(pad, y, x++, '-');
                        mvwaddch(pad, y, x++, ACS_RARROW);
                    }

                    arrow_v_dir = -arrow_v_dir;
                }
                cursor_x += chars_written;
            } while ((node =
                        krffs_get_next_node(
                            &file_system,
                            node
                        )) != file_system.node);
        }

        if (has_text_option) {
            break;
        }

        wnoutrefresh(stdscr);
        prefresh(
            pad,
            pad_shift_y, pad_shift_x,
            0, 0,
            window_height - 1, window_width - 1
        );
        doupdate();

        int key = wgetch(pad);
        if (key != ERR) {
            switch (key) {
                case 'h':
                case KEY_LEFT:
                    --pad_shift_x;
                    break;
                case 'j':
                case KEY_DOWN:
                    ++pad_shift_y;
                    break;
                case 'k':
                case KEY_UP:
                    --pad_shift_y;
                    break;
                case 'l':
                case KEY_RIGHT:
                    ++pad_shift_x;
                    break;
                case '+':
                    bytes_per_scr_chr *= 2;
                    break;
                case '_':
                    bytes_per_scr_chr /= 2;
                    if (bytes_per_scr_chr == 0) {
                        bytes_per_scr_chr = 2;
                    }
                    break;
                case 'q':
                    goto cleanup;
            }

            int pad_width_limit = pad_width - 5;
            if (pad_shift_x < 0) {
                pad_shift_x = 0;
            } else if (pad_shift_x > pad_width_limit) {
                pad_shift_x = pad_width_limit;
            }
        }
    }

    if (has_text_option && pad != NULL) {
        int max_y = 0;
        int max_x = 0;

        for (int y = 0; y < window_height && y < 100; ++y) {
            for (int x = 0; x < pad_width; ++x) {
                chtype ch = mvwinch(pad, y, x);
                if ((ch & 0xFF) != ' ') {
                    if (y > max_y) {
                        max_y = y;
                    }
                    if (x > max_x) {
                        max_x = x;
                    }
                }
            }
        }

        size_t buffer_size = (max_y + 2) * (max_x + 2);
        output_buffer = malloc(buffer_size);
        if (output_buffer) {
            size_t pos = 0;
            for (int y = 0; y <= max_y + 1; ++y) {
                for (int x = 0; x <= max_x; ++x) {
                    chtype ch = mvwinch(pad, y, x);
                    char c;

                    if (ch == ACS_UARROW) {
                        c = '^';
                    } else if (ch == ACS_DARROW) {
                        c = 'v';
                    } else if (ch == ACS_RARROW) {
                        c = '>';
                    } else {
                        c = ch & 0xFF;
                    }

                    if (pos < buffer_size - 2) {
                        output_buffer[pos++] = c;
                    }
                }
                if (pos < buffer_size - 1) {
                    output_buffer[pos++] = '\n';
                }
            }
            output_buffer[pos] = '\0';
        }
    }

cleanup:
    if (pad != NULL) {
        delwin(pad);
        pad = NULL;
    }
    endwin();

    if (output_buffer != NULL) {
        printf("%s", output_buffer);
        fflush(stdout);
        free(output_buffer);
        output_buffer = NULL;
    }

    if (file_descriptor != -1) {
        if (PLATFORM_PREFIX(close(file_descriptor)) == -1) {
            fprintf(
                stderr,
                "Failed to close the file system file.\n"
            );
        }
    }

    if (file_system.node != NULL) {
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
