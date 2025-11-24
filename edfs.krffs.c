#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

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
    edfs.krffs

    Edits a node of a KRFFS FUSE file system in a file.

    Usage:
        edfs.krffs -h
        edfs.krffs <file> <node position> \
            [--set-magic <magic>]
            [--set-type root|free|reserved]
            [--set-id <id>]
            [--set-name <name>]
            [--set-size <size>]
            [--set-prev-size <size>]
            [--set-data-size <size>]

    Options:
        -h              show help and exit
        --set-magic     set the node's signature to a 16-bit unsigned number
        --set-type      set the node's type to 'root', 'free', or 'reserved' type
        --set-id        set the node's id to a 64-bit unsigned number
        --set-name      set the node's name to a string of up to 255 bytes in size
        --set-size      set the node's size field to a 64-bit unsigned number
        --set-prev-size set the node's field 'previous node size' to a 64-bit unsigned number
        --set-data-size set the node's data size field to a 64-bit unsigned number
 */
int main(int argc, char **argv)
{
    int exit_status = EXIT_SUCCESS;

    int file_descriptor = -1;
    struct krffs_file_system file_system = {
        .node = NULL
    };

    bool has_help_option = false;

    bool has_set_magic_option = false;
    uint16_t magic = (uint16_t) -1;

    bool has_set_type_option = false;
    uint8_t type = (uint8_t) -1;

    bool has_set_id_option = false;
    uint64_t new_id = (uint64_t) -1;

    bool has_set_name_option = false;
    uint8_t name[KRFFS_FILE_NAME_BUFFER_SIZE];
    name[0] = '\0';

    bool has_set_size_option = false;
    uint64_t size = (uint64_t) -1;

    bool has_set_prev_size_option = false;
    uint64_t previous_node_size = (uint64_t) -1;

    bool has_set_data_size_option = false;
    uint64_t data_size = (uint64_t) -1;

    bool failed_to_parse_options = false;

    char *end_ptr = NULL;
    int base = 10;

    int pos;
    if (argc > 2) {
        errno = 0;
        pos = (uint64_t) strtol(argv[2], &end_ptr, base);
        if (argv[2] == end_ptr || errno != 0) {
            failed_to_parse_options = true;
        }
    }

    for (int i = 3; !failed_to_parse_options && i < argc; ++i) {
        if (!has_help_option && strncmp(argv[i], "-h", 2) == 0) {
            has_help_option = true;
        } else if (!has_set_magic_option && strncmp(argv[i], "--set-magic", 11) == 0) {
            has_set_magic_option = true;
            if (i + 1 < argc) {
                errno = 0;
                magic = (uint16_t) strtol(argv[i + 1], &end_ptr, base);
                if (argv[i + i] == end_ptr || errno != 0) {
                    failed_to_parse_options = true;
                }
            } else {
                failed_to_parse_options = true;
            }
            ++i;
        } else if (!has_set_type_option && strncmp(argv[i], "--set-type", 10) == 0) {
            has_set_type_option = true;
            if (i + 1 < argc) {
                for (char *curs = argv[i + 1]; *curs; ++curs) {
                    *curs = tolower(*curs);
                }
                if (strncmp(argv[i + 1], "root", 4) == 0) {
                    type = KRFFS_Root_Node;
                } else if (strncmp(argv[i + 1], "reserved", 8) == 0) {
                    type = KRFFS_Reserved_Node;
                } else if (strncmp(argv[i + 1], "free", 4) == 0) {
                    type = KRFFS_Free_Node;
                } else {
                    failed_to_parse_options = true;
                }
            } else {
                failed_to_parse_options = true;
            }
            ++i;
        } else if (!has_set_id_option && strncmp(argv[i], "--set-id", 8) == 0) {
            has_set_id_option = true;
            if (i + 1 < argc) {
                errno = 0;
                new_id = (uint64_t) strtol(argv[i + 1], &end_ptr, base);
                if (argv[i + i] == end_ptr || errno != 0) {
                    failed_to_parse_options = true;
                }
            } else {
                failed_to_parse_options = true;
            }
            ++i;
        } else if (!has_set_name_option && strncmp(argv[i], "--set-name", 10) == 0) {
            has_set_name_option = true;
            if (i + 1 < argc) {
                strncpy(
                    name,
                    argv[i + 1],
                    KRFFS_FILE_NAME_BUFFER_SIZE
                );
                name[KRFFS_FILE_NAME_BUFFER_SIZE - 1] = '\0';
            } else {
                failed_to_parse_options = true;
            }
            ++i;
        } else if (!has_set_size_option && strncmp(argv[i], "--set-size", 10) == 0) {
            has_set_size_option = true;
            if (i + 1 < argc) {
                errno = 0;
                size = (uint64_t) strtol(argv[i + 1], &end_ptr, base);
                if (argv[i + i] == end_ptr || errno != 0) {
                    failed_to_parse_options = true;
                }
            } else {
                failed_to_parse_options = true;
            }
            ++i;
        } else if (!has_set_prev_size_option && strncmp(argv[i], "--set-prev-size", 15) == 0) {
            has_set_prev_size_option = true;
            if (i + 1 < argc) {
                errno = 0;
                previous_node_size = (uint64_t) strtol(argv[i + 1], &end_ptr, base);
                if (argv[i + i] == end_ptr || errno != 0) {
                    failed_to_parse_options = true;
                }
            } else {
                failed_to_parse_options = true;
            }
            ++i;
        } else if (!has_set_data_size_option && strncmp(argv[i], "--set-data-size", 15) == 0) {
            has_set_data_size_option = true;
            if (i + 1 < argc) {
                errno = 0;
                data_size = (uint64_t) strtol(argv[i + 1], &end_ptr, base);
                if (argv[i + i] == end_ptr || errno != 0) {
                    failed_to_parse_options = true;
                }
            } else {
                failed_to_parse_options = true;
            }
            ++i;
        }
    }

    if (argc <= 2 || has_help_option || failed_to_parse_options) {
        fprintf(
            stderr,
            "Usage: %s <file> <node position> <options>\n",
            argc >= 1 ?
                argv[0] : "edfs.krffs"
        );

        goto cleanup;
    } else if (argv[1][0] == '-' || argv[2][0] == '-') {
        fprintf(
            stderr,
            "Not all first parameters are valid.\n"
            "\n"
            "Usage: %s <file> <node position> <options>\n",
            argc >= 1 ?
                argv[0] : "edfs.krffs"
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
        Check that the file is big enough to initialize a file system.
     */
    if (file_info.st_size < sizeof(*file_system.node) * 2) {
        fprintf(
            stderr,
            "The file at '%s' is not big enough to initialize a file system.\n",
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
        Try to find and edit the node.
     */
    struct krffs_node *node = file_system.node;
    do {
        if (krffs_get_node_relative_position(&file_system, node) == pos) {
            if (has_set_magic_option) {
                node->magic = magic;
            }

            if (has_set_type_option) {
                node->type = type;
            }

            if (has_set_id_option) {
                node->id = new_id;
            }

            if (has_set_name_option) {
                strncpy(
                    (char *) node->name,
                    name,
                    KRFFS_FILE_NAME_BUFFER_SIZE
                );
                node->name[KRFFS_FILE_NAME_BUFFER_SIZE - 1] = '\0';

                node->id =
                    krffs_calculate_djb_hash(
                        (uint8_t *) node->name
                    );
            }

            if (has_set_size_option) {
                node->size = size;
            }

            if (has_set_prev_size_option) {
                node->previous_node_size = previous_node_size;
            }

            if (has_set_data_size_option) {
                node->data_size = data_size;
            }

            krffs_sync_mapping(
                node,
                sizeof(*node)
            );

            break;
        }
    } while ((node =
                krffs_get_next_node(
                    &file_system,
                    node
                )) != file_system.node);

cleanup:
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
