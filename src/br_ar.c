/*
 * br-ar - create and maintain .brarchive files
 *
 * Copyright (C) 2025  Torrekie <me@torrekie.dev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This program is based on the original Rust implementation brarchive-cli
 * by Lucy <theaddonn@gmail.com> (https://github.com/theaddonn/brarchive-rs).
 * This C implementation is a complete rewrite with the following modifications:
 * - Rewritten in C11 for better portability and performance
 * - Uses GNU Autotools for cross-platform build system
 * - Implements ar(1)-like command-line interface
 * - Added delete operation support
 * - POSIX-compliant shell scripts for wrapper and tests
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Feature test macros for POSIX and GNU extensions */
/* Define before any includes to ensure getopt, strdup, etc. are available */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

/* Windows-specific includes before unistd.h to avoid conflicts */
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#endif

#include <unistd.h>

/* getopt should be available via unistd.h with _POSIX_C_SOURCE */
/* If not available, declare it */
#ifdef HAVE_GETOPT
/* getopt is available */
#else
/* getopt might still be available via unistd.h with feature test macros */
/* If compilation fails, we'll need to provide an implementation */
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#else
#ifndef bool
#define bool int
#define true 1
#define false 0
#endif
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

/* Define PATH_MAX if not available */
#ifndef PATH_MAX
#ifdef _WIN32
/* Windows MAX_PATH is 260, but we need extra space for separators */
#define PATH_MAX 512
#else
#define PATH_MAX 4096
#endif
#endif

/* Windows mkdir compatibility - Windows mkdir only takes one argument */
#ifdef _WIN32
/* Create a wrapper for POSIX-compatible mkdir */
static int mkdir_with_mode(const char *path, mode_t mode) {
    (void)mode; /* Windows ignores mode */
    return _mkdir(path);
}
#define mkdir(path, mode) mkdir_with_mode(path, mode)
#endif

/* Platform-specific endian headers */
#if defined(HAVE_DARWIN_ENDIAN) && defined(HAVE_LIBKERN_OSBYTEORDER_H)
#include <libkern/OSByteOrder.h>
#endif

#if defined(HAVE_FREEBSD_ENDIAN) && defined(HAVE_SYS_ENDIAN_H)
#include <sys/endian.h>
#endif

#if defined(HAVE_OPENBSD_ENDIAN) && defined(HAVE_SYS_ENDIAN_H)
#include <sys/endian.h>
#endif

#if defined(HAVE_LINUX_ENDIAN) && defined(HAVE_ENDIAN_H)
#include <endian.h>
#include <byteswap.h>
#endif

#define MAGIC 0x267052A0B125277DULL
#define ARCHIVE_VERSION 1
#define HEADER_SIZE 16
#define ENTRY_SIZE 256
#define MAX_NAME_LEN 247

/* Option flags (matching ar behavior) */
#define OPT_C 0x01  /* Suppress "creating archive" message */
#define OPT_V 0x02  /* Verbose mode */

/* Old-school struct naming */
struct br_ar_header {
    uint32_t entries;
    uint32_t version;
};

struct br_ar_entry {
    char name[248];  /* 1 byte length + 247 bytes name */
    uint8_t name_len;
    uint32_t contents_offset;
    uint32_t contents_len;
};

struct file_list {
    char **paths;
    char **names;
    char **contents;
    size_t *sizes;
    size_t count;
    size_t capacity;
};

/* Endian conversion functions */
#if USE_PLATFORM_ENDIAN

#if defined(HAVE_DARWIN_ENDIAN)
/* Darwin/macOS endian operations */
static inline void write_u32_le(uint8_t *buf, uint32_t value) {
    uint32_t le = OSSwapHostToLittleInt32(value);
    memcpy(buf, &le, 4);
}

static inline void write_u64_le(uint8_t *buf, uint64_t value) {
    uint64_t le = OSSwapHostToLittleInt64(value);
    memcpy(buf, &le, 8);
}

static inline uint32_t read_u32_le(const uint8_t *buf) {
    uint32_t le;
    memcpy(&le, buf, 4);
    return OSSwapLittleToHostInt32(le);
}

static inline uint64_t read_u64_le(const uint8_t *buf) {
    uint64_t le;
    memcpy(&le, buf, 8);
    return OSSwapLittleToHostInt64(le);
}

#elif defined(HAVE_FREEBSD_ENDIAN) || defined(HAVE_OPENBSD_ENDIAN)
/* FreeBSD/OpenBSD endian operations */
static inline void write_u32_le(uint8_t *buf, uint32_t value) {
    uint32_t le = htole32(value);
    memcpy(buf, &le, 4);
}

static inline void write_u64_le(uint8_t *buf, uint64_t value) {
    uint64_t le = htole64(value);
    memcpy(buf, &le, 8);
}

static inline uint32_t read_u32_le(const uint8_t *buf) {
    uint32_t le;
    memcpy(&le, buf, 4);
    return le32toh(le);
}

static inline uint64_t read_u64_le(const uint8_t *buf) {
    uint64_t le;
    memcpy(&le, buf, 8);
    return le64toh(le);
}

#elif defined(HAVE_LINUX_ENDIAN)
/* Linux endian operations */
static inline void write_u32_le(uint8_t *buf, uint32_t value) {
    uint32_t le = htole32(value);
    memcpy(buf, &le, 4);
}

static inline void write_u64_le(uint8_t *buf, uint64_t value) {
    uint64_t le = htole64(value);
    memcpy(buf, &le, 8);
}

static inline uint32_t read_u32_le(const uint8_t *buf) {
    uint32_t le;
    memcpy(&le, buf, 4);
    return le32toh(le);
}

static inline uint64_t read_u64_le(const uint8_t *buf) {
    uint64_t le;
    memcpy(&le, buf, 8);
    return le64toh(le);
}

#else
/* Fallback to generic implementation */
#define USE_PLATFORM_ENDIAN 0
#endif

#endif /* USE_PLATFORM_ENDIAN */

#if !USE_PLATFORM_ENDIAN
/* Generic portable endian operations */
static void write_u32_le(uint8_t *buf, uint32_t value) {
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)((value >> 8) & 0xFF);
    buf[2] = (uint8_t)((value >> 16) & 0xFF);
    buf[3] = (uint8_t)((value >> 24) & 0xFF);
}

static void write_u64_le(uint8_t *buf, uint64_t value) {
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)((value >> 8) & 0xFF);
    buf[2] = (uint8_t)((value >> 16) & 0xFF);
    buf[3] = (uint8_t)((value >> 24) & 0xFF);
    buf[4] = (uint8_t)((value >> 32) & 0xFF);
    buf[5] = (uint8_t)((value >> 40) & 0xFF);
    buf[6] = (uint8_t)((value >> 48) & 0xFF);
    buf[7] = (uint8_t)((value >> 56) & 0xFF);
}

static uint32_t read_u32_le(const uint8_t *buf) {
    return (uint32_t)buf[0] |
           ((uint32_t)buf[1] << 8) |
           ((uint32_t)buf[2] << 16) |
           ((uint32_t)buf[3] << 24);
}

static uint64_t read_u64_le(const uint8_t *buf) {
    return (uint64_t)buf[0] |
           ((uint64_t)buf[1] << 8) |
           ((uint64_t)buf[2] << 16) |
           ((uint64_t)buf[3] << 24) |
           ((uint64_t)buf[4] << 32) |
           ((uint64_t)buf[5] << 40) |
           ((uint64_t)buf[6] << 48) |
           ((uint64_t)buf[7] << 56);
}
#endif

/* Read file contents into buffer */
static char *read_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(f);
        return NULL;
    }
    
    char *buf = malloc((size_t)file_size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    
    size_t read = fread(buf, 1, (size_t)file_size, f);
    fclose(f);
    
    if (read != (size_t)file_size) {
        free(buf);
        return NULL;
    }
    
    buf[file_size] = '\0';
    *size = (size_t)file_size;
    return buf;
}

/* Write buffer to file */
static bool write_file(const char *path, const void *data, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        return false;
    }
    
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    
    return written == size;
}

/* File list operations */
static void file_list_init(struct file_list *list) {
    list->capacity = 16;
    list->count = 0;
    list->paths = malloc(list->capacity * sizeof(char*));
    list->names = malloc(list->capacity * sizeof(char*));
    list->contents = malloc(list->capacity * sizeof(char*));
    list->sizes = malloc(list->capacity * sizeof(size_t));
}

static void file_list_free(struct file_list *list) {
    size_t i;
    for (i = 0; i < list->count; i++) {
        free(list->paths[i]);
        free(list->names[i]);
        free(list->contents[i]);
    }
    free(list->paths);
    free(list->names);
    free(list->contents);
    free(list->sizes);
}

static bool file_list_add(struct file_list *list, const char *path, const char *name) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->paths = realloc(list->paths, list->capacity * sizeof(char*));
        list->names = realloc(list->names, list->capacity * sizeof(char*));
        list->contents = realloc(list->contents, list->capacity * sizeof(char*));
        list->sizes = realloc(list->sizes, list->capacity * sizeof(size_t));
    }
    
#ifdef HAVE_STRDUP
    list->paths[list->count] = strdup(path);
    list->names[list->count] = strdup(name);
#else
    /* Fallback if strdup not available */
    size_t path_len = strlen(path) + 1;
    size_t name_len = strlen(name) + 1;
    list->paths[list->count] = malloc(path_len);
    list->names[list->count] = malloc(name_len);
    if (list->paths[list->count] && list->names[list->count]) {
        memcpy(list->paths[list->count], path, path_len);
        memcpy(list->names[list->count], name, name_len);
    } else {
        free(list->paths[list->count]);
        free(list->names[list->count]);
        return false;
    }
#endif
    
    size_t size;
    list->contents[list->count] = read_file(path, &size);
    if (!list->contents[list->count]) {
        free(list->paths[list->count]);
        free(list->names[list->count]);
        return false;
    }
    
    list->sizes[list->count] = size;
    list->count++;
    return true;
}

static void collect_files_recursive(const char *dir_path, const char *base_path, struct file_list *list) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) != 0) {
            continue;
        }
        
        if (S_ISREG(st.st_mode)) {
            char relative_name[PATH_MAX];
            if (base_path) {
                snprintf(relative_name, sizeof(relative_name), "%s/%s", base_path, entry->d_name);
            } else {
                strncpy(relative_name, entry->d_name, sizeof(relative_name) - 1);
                relative_name[sizeof(relative_name) - 1] = '\0';
            }
            
            if (strlen(relative_name) > MAX_NAME_LEN) {
                fprintf(stderr, "Warning: File name too long, skipping: %s\n", relative_name);
                continue;
            }
            
            file_list_add(list, full_path, relative_name);
        } else if (S_ISDIR(st.st_mode)) {
            char new_base[PATH_MAX];
            if (base_path) {
                snprintf(new_base, sizeof(new_base), "%s/%s", base_path, entry->d_name);
            } else {
                strncpy(new_base, entry->d_name, sizeof(new_base) - 1);
                new_base[sizeof(new_base) - 1] = '\0';
            }
            collect_files_recursive(full_path, new_base, list);
        }
    }
    
    closedir(dir);
}

/* Create archive from directory */
static bool create_archive(const char *archive_path, const char *dir_path, int options) {
    struct file_list files;
    file_list_init(&files);
    
    collect_files_recursive(dir_path, NULL, &files);
    
    if (files.count == 0) {
        fprintf(stderr, "No files found in directory: %s\n", dir_path);
        file_list_free(&files);
        return false;
    }
    
    /* Calculate total size needed */
    size_t header_and_entries_size = HEADER_SIZE + (ENTRY_SIZE * files.count);
    size_t data_offset = header_and_entries_size;
    size_t total_data_size = 0;
    size_t i;
    
    for (i = 0; i < files.count; i++) {
        total_data_size += files.sizes[i];
    }
    
    size_t total_size = header_and_entries_size + total_data_size;
    uint8_t *archive = malloc(total_size);
    if (!archive) {
        fprintf(stderr, "Memory allocation failed\n");
        file_list_free(&files);
        return false;
    }
    
    memset(archive, 0, total_size);
    
    /* Write header */
    write_u64_le(archive, MAGIC);
    write_u32_le(archive + 8, (uint32_t)files.count);
    write_u32_le(archive + 12, ARCHIVE_VERSION);
    
    /* Write entry descriptors and calculate offsets */
    size_t entry_offset = HEADER_SIZE;
    size_t data_pos = data_offset;
    
    for (i = 0; i < files.count; i++) {
        uint8_t name_len = (uint8_t)strlen(files.names[i]);
        archive[entry_offset] = name_len;
        
        memcpy(archive + entry_offset + 1, files.names[i], name_len);
        /* Rest is already zeroed */
        
        /* contents_offset is relative to data block start */
        write_u32_le(archive + entry_offset + 248, (uint32_t)(data_pos - data_offset));
        write_u32_le(archive + entry_offset + 252, (uint32_t)files.sizes[i]);
        
        data_pos += files.sizes[i];
        entry_offset += ENTRY_SIZE;
    }
    
    /* Write file contents */
    data_pos = data_offset;
    for (i = 0; i < files.count; i++) {
        memcpy(archive + data_pos, files.contents[i], files.sizes[i]);
        data_pos += files.sizes[i];
    }
    
    bool success = write_file(archive_path, archive, total_size);
    
    free(archive);
    file_list_free(&files);
    
    if (success) {
        if (!(options & OPT_C)) {
            printf("Created archive: %s (%zu files)\n", archive_path, files.count);
        }
    }
    
    return success;
}

/* Extract archive to directory (with optional file filter) */
static bool extract_archive(const char *archive_path, const char *dir_path, char **file_filter, int filter_count, int options) {
    size_t archive_size;
    uint8_t *archive_data = (uint8_t*)read_file(archive_path, &archive_size);
    if (!archive_data) {
        fprintf(stderr, "Failed to read archive: %s\n", archive_path);
        return false;
    }
    
    if (archive_size < HEADER_SIZE) {
        fprintf(stderr, "Archive too small\n");
        free(archive_data);
        return false;
    }
    
    /* Read header */
    uint64_t magic = read_u64_le(archive_data);
    if (magic != MAGIC) {
        fprintf(stderr, "Invalid magic number: 0x%016llx\n", (unsigned long long)magic);
        free(archive_data);
        return false;
    }
    
    uint32_t entries = read_u32_le(archive_data + 8);
    uint32_t version = read_u32_le(archive_data + 12);
    
    if (version != ARCHIVE_VERSION) {
        fprintf(stderr, "Unsupported version: %u\n", version);
        free(archive_data);
        return false;
    }
    
    /* Create output directory if specified */
    if (dir_path) {
        struct stat st;
        if (stat(dir_path, &st) != 0) {
            if (mkdir(dir_path, 0755) != 0) {
                fprintf(stderr, "Failed to create directory: %s\n", dir_path);
                free(archive_data);
                return false;
            }
        }
    }
    
    /* Read entries */
    size_t entry_offset = HEADER_SIZE;
    size_t data_block_start = HEADER_SIZE + (ENTRY_SIZE * entries);
    uint32_t i;
    
    for (i = 0; i < entries; i++) {
        if (entry_offset + ENTRY_SIZE > archive_size) {
            fprintf(stderr, "Archive corrupted: entry %u out of bounds\n", i);
            free(archive_data);
            return false;
        }
        
        uint8_t name_len = archive_data[entry_offset];
        if (name_len > MAX_NAME_LEN) {
            fprintf(stderr, "Invalid name length in entry %u\n", i);
            entry_offset += ENTRY_SIZE;
            continue;
        }
        
        char name[248];
        memcpy(name, archive_data + entry_offset + 1, name_len);
        name[name_len] = '\0';
        
        /* Check if this file should be extracted (if filter is specified) */
        bool should_extract = true;
        if (filter_count > 0) {
            should_extract = false;
            int j;
            for (j = 0; j < filter_count; j++) {
                /* Match by basename (like ar does) */
                const char *basename = strrchr(name, '/');
                const char *filter_basename = strrchr(file_filter[j], '/');
                const char *name_to_match = basename ? basename + 1 : name;
                const char *filter_to_match = filter_basename ? filter_basename + 1 : file_filter[j];
                if (strcmp(name_to_match, filter_to_match) == 0) {
                    should_extract = true;
                    break;
                }
            }
        }
        
        if (!should_extract) {
            entry_offset += ENTRY_SIZE;
            continue;
        }
        
        uint32_t contents_offset = read_u32_le(archive_data + entry_offset + 248);
        uint32_t contents_len = read_u32_le(archive_data + entry_offset + 252);
        
        /* contents_offset is relative to data block start */
        size_t actual_offset = data_block_start + contents_offset;
        if (actual_offset + contents_len > archive_size) {
            fprintf(stderr, "Archive corrupted: file %s out of bounds\n", name);
            entry_offset += ENTRY_SIZE;
            continue;
        }
        
        /* Create output file path */
        char output_path[PATH_MAX];
        if (dir_path) {
            snprintf(output_path, sizeof(output_path), "%s/%s", dir_path, name);
        } else {
            /* Extract to current directory (like ar -x) */
            snprintf(output_path, sizeof(output_path), "%s", name);
        }
        
        /* Create parent directories if needed */
        char *last_slash = strrchr(output_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            struct stat dir_st;
            if (stat(output_path, &dir_st) != 0) {
                /* Create parent directories recursively */
                char *path = output_path;
                char *p;
                for (p = path + 1; *p; p++) {
                    if (*p == '/') {
                        *p = '\0';
                        if (stat(path, &dir_st) != 0) {
                            mkdir(path, 0755);
                        }
                        *p = '/';
                    }
                }
                mkdir(path, 0755);
            }
            *last_slash = '/';
        }
        
        /* Write file (contents_offset is relative to data block start) */
        if (!write_file(output_path, archive_data + actual_offset, contents_len)) {
            fprintf(stderr, "Failed to write file: %s\n", output_path);
        } else {
            if (options & OPT_V) {
                printf("x - %s\n", name);
            }
        }
        
        entry_offset += ENTRY_SIZE;
    }
    
    free(archive_data);
    return true;
}

/* Print archive contents to stdout (with optional file filter) */
static bool print_archive(const char *archive_path, char **file_filter, int filter_count) {
    size_t archive_size;
    uint8_t *archive_data = (uint8_t*)read_file(archive_path, &archive_size);
    if (!archive_data) {
        fprintf(stderr, "Failed to read archive: %s\n", archive_path);
        return false;
    }
    
    if (archive_size < HEADER_SIZE) {
        fprintf(stderr, "Archive too small\n");
        free(archive_data);
        return false;
    }
    
    /* Read header */
    uint64_t magic = read_u64_le(archive_data);
    if (magic != MAGIC) {
        fprintf(stderr, "Invalid magic number: 0x%016llx\n", (unsigned long long)magic);
        free(archive_data);
        return false;
    }
    
    uint32_t entries = read_u32_le(archive_data + 8);
    
    /* Read entries */
    size_t entry_offset = HEADER_SIZE;
    size_t data_block_start = HEADER_SIZE + (ENTRY_SIZE * entries);
    uint32_t i;
    
    for (i = 0; i < entries; i++) {
        if (entry_offset + ENTRY_SIZE > archive_size) {
            fprintf(stderr, "Archive corrupted: entry %u out of bounds\n", i);
            break;
        }
        
        uint8_t name_len = archive_data[entry_offset];
        if (name_len > MAX_NAME_LEN) {
            fprintf(stderr, "Invalid name length in entry %u\n", i);
            entry_offset += ENTRY_SIZE;
            continue;
        }
        
        char name[248];
        memcpy(name, archive_data + entry_offset + 1, name_len);
        name[name_len] = '\0';
        
        /* Check if this file should be printed (if filter is specified) */
        bool should_print = true;
        if (filter_count > 0) {
            should_print = false;
            int j;
            for (j = 0; j < filter_count; j++) {
                /* Match by basename (like ar does) */
                const char *basename = strrchr(name, '/');
                const char *filter_basename = strrchr(file_filter[j], '/');
                const char *name_to_match = basename ? basename + 1 : name;
                const char *filter_to_match = filter_basename ? filter_basename + 1 : file_filter[j];
                if (strcmp(name_to_match, filter_to_match) == 0) {
                    should_print = true;
                    break;
                }
            }
        }
        
        if (should_print) {
            uint32_t contents_offset = read_u32_le(archive_data + entry_offset + 248);
            uint32_t contents_len = read_u32_le(archive_data + entry_offset + 252);
            size_t actual_offset = data_block_start + contents_offset;
            
            if (actual_offset + contents_len > archive_size) {
                fprintf(stderr, "Archive corrupted: file %s out of bounds\n", name);
                entry_offset += ENTRY_SIZE;
                continue;
            }
            
            /* Print file contents to stdout */
            fwrite(archive_data + actual_offset, 1, contents_len, stdout);
        }
        
        entry_offset += ENTRY_SIZE;
    }
    
    free(archive_data);
    return true;
}

/* List archive contents (with optional file filter) */
static bool list_archive(const char *archive_path, char **file_filter, int filter_count) {
    size_t archive_size;
    uint8_t *archive_data = (uint8_t*)read_file(archive_path, &archive_size);
    if (!archive_data) {
        fprintf(stderr, "Failed to read archive: %s\n", archive_path);
        return false;
    }
    
    if (archive_size < HEADER_SIZE) {
        fprintf(stderr, "Archive too small\n");
        free(archive_data);
        return false;
    }
    
    /* Read header */
    uint64_t magic = read_u64_le(archive_data);
    if (magic != MAGIC) {
        fprintf(stderr, "Invalid magic number: 0x%016llx\n", (unsigned long long)magic);
        free(archive_data);
        return false;
    }
    
    uint32_t entries = read_u32_le(archive_data + 8);
    
    /* Read entries */
    size_t entry_offset = HEADER_SIZE;
    uint32_t i;
    
    for (i = 0; i < entries; i++) {
        if (entry_offset + ENTRY_SIZE > archive_size) {
            fprintf(stderr, "Archive corrupted: entry %u out of bounds\n", i);
            break;
        }
        
        uint8_t name_len = archive_data[entry_offset];
        if (name_len > MAX_NAME_LEN) {
            fprintf(stderr, "Invalid name length in entry %u\n", i);
            entry_offset += ENTRY_SIZE;
            continue;
        }
        
        char name[248];
        memcpy(name, archive_data + entry_offset + 1, name_len);
        name[name_len] = '\0';
        
        /* Check if this file should be listed (if filter is specified) */
        bool should_list = true;
        if (filter_count > 0) {
            should_list = false;
            int j;
            for (j = 0; j < filter_count; j++) {
                /* Match by basename (like ar does) */
                const char *basename = strrchr(name, '/');
                const char *filter_basename = strrchr(file_filter[j], '/');
                const char *name_to_match = basename ? basename + 1 : name;
                const char *filter_to_match = filter_basename ? filter_basename + 1 : file_filter[j];
                if (strcmp(name_to_match, filter_to_match) == 0) {
                    should_list = true;
                    break;
                }
            }
        }
        
        if (should_list) {
            printf("%s\n", name);
        }
        
        entry_offset += ENTRY_SIZE;
    }
    
    free(archive_data);
    return true;
}

static void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s -r archive directory\n", prog_name);
    fprintf(stderr, "       %s -t archive [file ...]\n", prog_name);
    fprintf(stderr, "       %s -x archive [file ...]\n", prog_name);
    fprintf(stderr, "       %s -p archive [file ...]\n", prog_name);
    fprintf(stderr, "       %s -d archive file ...\n", prog_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Operations (one required):\n");
    fprintf(stderr, "  -r  Replace/add files to archive (creates if doesn't exist)\n");
    fprintf(stderr, "  -t  List archive contents\n");
    fprintf(stderr, "  -x  Extract files from archive to current directory\n");
    fprintf(stderr, "  -p  Print file contents to stdout\n");
    fprintf(stderr, "  -d  Delete files from archive\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -c  Suppress 'creating archive' message (silent mode)\n");
    fprintf(stderr, "  -v  Verbose mode (show extracted files)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Note: Options can be combined (e.g., -rc, -xv)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s -r pack.brarchive ./mydir\n", prog_name);
    fprintf(stderr, "  %s -rc pack.brarchive ./mydir         # Silent create\n", prog_name);
    fprintf(stderr, "  %s -t pack.brarchive\n", prog_name);
    fprintf(stderr, "  %s -x pack.brarchive\n", prog_name);
    fprintf(stderr, "  %s -d pack.brarchive file1.json\n", prog_name);
    fprintf(stderr, "  %s -xv pack.brarchive                  # Verbose extract\n", prog_name);
    fprintf(stderr, "  %s -p pack.brarchive file1.json\n", prog_name);
}

/* Delete files from archive */
static bool delete_from_archive(const char *archive_path, char **files_to_delete, int file_count, int options) {
    /* Read existing archive */
    size_t archive_size;
    uint8_t *archive_data = (uint8_t*)read_file(archive_path, &archive_size);
    if (!archive_data) {
        fprintf(stderr, "Failed to read archive: %s\n", archive_path);
        return false;
    }
    
    if (archive_size < HEADER_SIZE) {
        fprintf(stderr, "Archive too small\n");
        free(archive_data);
        return false;
    }
    
    uint64_t magic = read_u64_le(archive_data);
    if (magic != MAGIC) {
        fprintf(stderr, "Invalid magic number\n");
        free(archive_data);
        return false;
    }
    
    uint32_t entries = read_u32_le(archive_data + 8);
    uint32_t version = read_u32_le(archive_data + 12);
    
    if (version != ARCHIVE_VERSION) {
        fprintf(stderr, "Unsupported version: %u\n", version);
        free(archive_data);
        return false;
    }
    
    /* Collect entries to keep */
    struct file_list files;
    file_list_init(&files);
    
    size_t entry_offset = HEADER_SIZE;
    size_t data_block_start = HEADER_SIZE + (ENTRY_SIZE * entries);
    uint32_t i;
    int deleted_count = 0;
    
    for (i = 0; i < entries; i++) {
        uint8_t name_len = archive_data[entry_offset];
        if (name_len > MAX_NAME_LEN) {
            entry_offset += ENTRY_SIZE;
            continue;
        }
        
        char name[248];
        memcpy(name, archive_data + entry_offset + 1, name_len);
        name[name_len] = '\0';
        
        /* Check if this file should be deleted */
        /* Match by exact name (like ar command) */
        bool should_delete = false;
        int j;
        for (j = 0; j < file_count; j++) {
            if (strcmp(name, files_to_delete[j]) == 0) {
                should_delete = true;
                deleted_count++;
                if (options & OPT_V) {
                    printf("d - %s\n", name);
                }
                break;
            }
        }
        
        if (!should_delete) {
            /* Keep this file - read its contents */
            uint32_t contents_offset = read_u32_le(archive_data + entry_offset + 248);
            uint32_t contents_len = read_u32_le(archive_data + entry_offset + 252);
            size_t actual_offset = data_block_start + contents_offset;
            
            if (actual_offset + contents_len > archive_size) {
                fprintf(stderr, "Warning: Invalid entry, skipping: %s\n", name);
                entry_offset += ENTRY_SIZE;
                continue;
            }
            
            char *content = malloc(contents_len);
            if (content) {
                memcpy(content, archive_data + actual_offset, contents_len);
                
                files.paths[files.count] = NULL;
#ifdef HAVE_STRDUP
                files.names[files.count] = strdup(name);
#else
                size_t name_len_dup = strlen(name) + 1;
                files.names[files.count] = malloc(name_len_dup);
                if (files.names[files.count]) {
                    memcpy(files.names[files.count], name, name_len_dup);
                }
#endif
                files.contents[files.count] = content;
                files.sizes[files.count] = contents_len;
                files.count++;
            }
        }
        
        entry_offset += ENTRY_SIZE;
    }
    
    free(archive_data);
    
    if (deleted_count == 0) {
        fprintf(stderr, "No files deleted (files not found in archive)\n");
        file_list_free(&files);
        return false;
    }
    
    /* Recreate archive with remaining files */
    if (files.count == 0) {
        fprintf(stderr, "Warning: All files deleted, archive will be empty\n");
    }
    
    /* Calculate total size needed */
    size_t header_and_entries_size = HEADER_SIZE + (ENTRY_SIZE * files.count);
    size_t data_offset = header_and_entries_size;
    size_t total_data_size = 0;
    
    for (i = 0; i < files.count; i++) {
        total_data_size += files.sizes[i];
    }
    
    size_t total_size = header_and_entries_size + total_data_size;
    uint8_t *archive = malloc(total_size);
    if (!archive) {
        fprintf(stderr, "Memory allocation failed\n");
        file_list_free(&files);
        return false;
    }
    
    memset(archive, 0, total_size);
    
    /* Write header */
    write_u64_le(archive, MAGIC);
    write_u32_le(archive + 8, (uint32_t)files.count);
    write_u32_le(archive + 12, ARCHIVE_VERSION);
    
    /* Write entry descriptors and calculate offsets */
    entry_offset = HEADER_SIZE;
    size_t data_pos = data_offset;
    
    for (i = 0; i < files.count; i++) {
        uint8_t name_len_val = (uint8_t)strlen(files.names[i]);
        archive[entry_offset] = name_len_val;
        
        memcpy(archive + entry_offset + 1, files.names[i], name_len_val);
        /* Rest is already zeroed */
        
        /* contents_offset is relative to data block start */
        write_u32_le(archive + entry_offset + 248, (uint32_t)(data_pos - data_offset));
        write_u32_le(archive + entry_offset + 252, (uint32_t)files.sizes[i]);
        
        data_pos += files.sizes[i];
        entry_offset += ENTRY_SIZE;
    }
    
    /* Write file contents */
    data_pos = data_offset;
    for (i = 0; i < files.count; i++) {
        memcpy(archive + data_pos, files.contents[i], files.sizes[i]);
        data_pos += files.sizes[i];
    }
    
    bool success = write_file(archive_path, archive, total_size);
    
    free(archive);
    file_list_free(&files);
    
    return success;
}

int main(int argc, char *argv[]) {
    int c;
    int options = 0;
    int operation = 0;  /* 'r', 't', 'x', 'p', 'd' */
    char *p;
    char *progname = argv[0];
    
    if (argc < 3) {
        print_usage(progname);
        return 1;
    }
    
    /*
     * Historic versions didn't require a '-' in front of the options.
     * Fix it, if necessary (like ar does).
     */
    if (argv[1][0] != '-') {
        size_t len = strlen(argv[1]);
        if (!(p = malloc(len + 2))) {
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }
        *p = '-';
        memcpy(p + 1, argv[1], len + 1);
        argv[1] = p;
    }
    
    /* Parse options using getopt (handles combined flags like -rc automatically) */
    while ((c = getopt(argc, argv, "cdptvxr")) != -1) {
        switch (c) {
        case 'c':
            options |= OPT_C;
            break;
        case 'd':
            if (operation && operation != 'd') {
                fprintf(stderr, "Only one operation (-d, -p, -r, -t, -x) allowed\n");
                return 1;
            }
            operation = 'd';
            break;
        case 'p':
            if (operation && operation != 'p') {
                fprintf(stderr, "Only one operation (-d, -p, -r, -t, -x) allowed\n");
                return 1;
            }
            operation = 'p';
            break;
        case 'r':
            if (operation && operation != 'r') {
                fprintf(stderr, "Only one operation (-d, -p, -r, -t, -x) allowed\n");
                return 1;
            }
            operation = 'r';
            break;
        case 't':
            if (operation && operation != 't') {
                fprintf(stderr, "Only one operation (-d, -p, -r, -t, -x) allowed\n");
                return 1;
            }
            operation = 't';
            break;
        case 'v':
            options |= OPT_V;
            break;
        case 'x':
            if (operation && operation != 'x') {
                fprintf(stderr, "Only one operation (-d, -p, -r, -t, -x) allowed\n");
                return 1;
            }
            operation = 'x';
            break;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }
    
    if (!operation) {
        fprintf(stderr, "One of options -d, -p, -r, -t, -x is required\n");
        print_usage(argv[0]);
        return 1;
    }
    
    /* Get remaining arguments (archive and files) */
    argc -= optind;
    argv += optind;
    
    if (argc < 1) {
        fprintf(stderr, "No archive specified\n");
        return 1;
    }
    
    const char *archive_path = argv[0];
    argc--;
    argv++;
    
    /* Execute operation */
    if (operation == 'r') {
        /* Replace/add: brar -r archive directory */
        if (argc != 1) {
            fprintf(stderr, "Usage: %s -r archive directory\n", progname);
            return 1;
        }
        if (!create_archive(archive_path, argv[0], options)) {
            return 1;
        }
    } else if (operation == 't') {
        /* List: brar -t archive [file ...] */
        char **file_filter = (argc > 0) ? argv : NULL;
        int filter_count = argc;
        if (!list_archive(archive_path, file_filter, filter_count)) {
            return 1;
        }
    } else if (operation == 'x') {
        /* Extract: brar -x archive [file ...] */
        char **file_filter = (argc > 0) ? argv : NULL;
        int filter_count = argc;
        if (!extract_archive(archive_path, NULL, file_filter, filter_count, options)) {
            return 1;
        }
    } else if (operation == 'p') {
        /* Print: brar -p archive [file ...] */
        char **file_filter = (argc > 0) ? argv : NULL;
        int filter_count = argc;
        if (!print_archive(archive_path, file_filter, filter_count)) {
            return 1;
        }
    } else if (operation == 'd') {
        /* Delete: br-ar -d archive file ... */
        if (argc < 1) {
            fprintf(stderr, "Usage: %s -d archive file ...\n", progname);
            return 1;
        }
        char **files_to_delete = argv;
        int file_count = argc;
        if (!delete_from_archive(archive_path, files_to_delete, file_count, options)) {
            return 1;
        }
    }
    
    return 0;
}

