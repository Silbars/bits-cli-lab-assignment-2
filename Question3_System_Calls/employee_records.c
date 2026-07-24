/*
 * employee_records.c
 *
 * Secure fixed-length record file utility built on raw Linux system calls
 * (open, read, write, lseek, close) instead of stdio.
 *
 *   1. Creates a file.
 *   2. Writes employee records.
 *   3. Updates a specific record in place (no full-file rewrite).
 *   4. Retrieves any record directly by index (random access).
 *
 * Fixed-length records make byte offsets predictable:
 *     offset(record N) = N * sizeof(struct Employee)
 * which is what lets lseek() jump straight to a record.
 *
 * Compile: gcc -o employee_records employee_records.c
 * Run:     ./employee_records
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DB "employees.dat"

struct Employee {
    int  id;
    char name[32];
    double salary;
};

/* Write a record at slot `index` without touching the rest of the file. */
static int put_record(int fd, int index, const struct Employee *e)
{
    off_t off = (off_t)index * sizeof(struct Employee);
    if (lseek(fd, off, SEEK_SET) == (off_t)-1) { perror("lseek"); return -1; }
    if (write(fd, e, sizeof(*e)) != (ssize_t)sizeof(*e)) { perror("write"); return -1; }
    return 0;
}

/* Read the record at slot `index` directly (random access). */
static int get_record(int fd, int index, struct Employee *e)
{
    off_t off = (off_t)index * sizeof(struct Employee);
    if (lseek(fd, off, SEEK_SET) == (off_t)-1) { perror("lseek"); return -1; }
    ssize_t n = read(fd, e, sizeof(*e));
    if (n != (ssize_t)sizeof(*e)) return -1;   /* not found / short read */
    return 0;
}

int main(void)
{
    /* 0600 = owner read/write only -> "secure" file permissions. */
    int fd = open(DB, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) { perror("open"); exit(EXIT_FAILURE); }

    struct Employee staff[3] = {
        {101, "Asha",  55000.0},
        {102, "Ben",   61000.0},
        {103, "Chitra",72000.0}
    };

    for (int i = 0; i < 3; i++) {
        if (put_record(fd, i, &staff[i]) == 0)
            printf("wrote record %d: id=%d name=%s salary=%.0f\n",
                   i, staff[i].id, staff[i].name, staff[i].salary);
    }

    /* Update ONLY record index 1 (Ben's raise) in place. */
    struct Employee ben;
    if (get_record(fd, 1, &ben) == 0) {
        ben.salary = 68000.0;
        put_record(fd, 1, &ben);
        printf("updated record 1: %s new salary=%.0f\n", ben.name, ben.salary);
    }

    /* Random-access retrieval of record index 2 without reading 0 and 1. */
    struct Employee r;
    if (get_record(fd, 2, &r) == 0)
        printf("read record 2 directly: id=%d name=%s salary=%.0f\n",
               r.id, r.name, r.salary);

    /* Confirm the in-place update persisted. */
    if (get_record(fd, 1, &r) == 0)
        printf("re-read record 1: id=%d name=%s salary=%.0f\n",
               r.id, r.name, r.salary);

    if (close(fd) == 0)
        printf("file closed cleanly.\n");
    return 0;
}
