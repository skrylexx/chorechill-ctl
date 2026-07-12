/*
 * chorechill-detect.c — DMI-based hardware detection utility
 *
 * Part of the chorechill-ctl project.
 *
 * This standalone utility reads SMBIOS/DMI identity data from the Linux sysfs
 * interface (/sys/class/dmi/id/) and prints the filename of the recommended
 * chorechill-ctl driver plugin to stdout.  The output is designed to be
 * captured by the daemon or an init script:
 *
 *   PLUGIN=$(chorechill-detect)
 *   chorechill-daemon --plugin "$PLUGIN"
 *
 * Diagnostic output (detected hardware strings, match result) is written to
 * stderr so that it does not pollute stdout.
 *
 * EXIT CODES
 *   0  — plugin filename successfully printed on stdout
 *   1  — DMI files are not readable (non-Linux kernel, missing debugfs/sysfs,
 *         or insufficient permissions)
 *
 * BUILD
 *   gcc -o chorechill-detect chorechill-detect.c
 *
 * INSTALL (example)
 *   install -m 755 chorechill-detect /usr/bin/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strncasecmp */
#include <ctype.h>     /* tolower     */
#include <errno.h>

/* ============================================================
 * DMI SYSFS PATHS
 * ============================================================ */
#define DMI_BOARD_VENDOR  "/sys/class/dmi/id/board_vendor"
#define DMI_BOARD_NAME    "/sys/class/dmi/id/board_name"
#define DMI_PRODUCT_NAME  "/sys/class/dmi/id/product_name"

/* Maximum number of characters read from any DMI file */
#define DMI_BUF_SIZE 256

/* ============================================================
 * BOARD-TO-PLUGIN LOOKUP TABLE
 *
 * Entries are matched against the board_name string using a
 * case-insensitive substring search.  The table is scanned in
 * order; the first match wins.
 *
 * To add a new board:
 *   1. Append a row before the NULL sentinel.
 *   2. Set board_substring to any unique part of the board_name
 *      string shown by:  cat /sys/class/dmi/id/board_name
 * ============================================================ */
typedef struct {
    const char *board_substring; /* Substring to search for (case-insensitive) */
    const char *plugin_filename; /* Recommended plugin .so filename             */
} board_entry_t;

static const board_entry_t board_table[] = {
    /* --------------------------------------------------------
     * MSI Modern EC — confirmed on GF63 Thin (MS-16R4, MS-16R5)
     * and similar 16" thin-and-light models with the modern EC layout.
     * -------------------------------------------------------- */
    { "ms-16r",  "plugin_msi_modern.so" },   /* GF63 Thin 10/11/12 gen */
    { "ms-16s",  "plugin_msi_modern.so" },   /* GF65 Thin              */
    { "ms-17e",  "plugin_msi_modern.so" },   /* GF75 Thin              */
    { "ms-1562", "plugin_msi_modern.so" },   /* GL62 (modern layout)   */
    { "ms-16w",  "plugin_msi_modern.so" },   /* Sword 15               */
    { "ms-158",  "plugin_msi_modern.so" },   /* Katana GF66            */

    /* Sentinel — must be last */
    { NULL, NULL }
};

/*
 * MSI FALLBACK — any MSI board_vendor that did not match the table above
 * is directed to the legacy driver until a specific entry is added.
 */
#define PLUGIN_MSI_LEGACY  "plugin_msi_legacy.so"

/*
 * UNKNOWN FALLBACK — non-MSI / unrecognised hardware.
 * The daemon will likely fail to load this, prompting the user to report
 * their board details.
 */
#define PLUGIN_UNKNOWN     "plugin_unknown.so"

/* ============================================================
 * HELPER: read_dmi_file()
 *
 * Read up to (buf_size - 1) bytes from 'path' into 'buf', strip
 * any trailing newline, and NUL-terminate.
 *
 * Returns  0 on success.
 * Returns -1 and prints an error to stderr on failure.
 * ============================================================ */
static int read_dmi_file(const char *path, char *buf, size_t buf_size)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr,
                "[chorechill-detect] ERROR: Cannot open '%s': %s\n",
                path, strerror(errno));
        return -1;
    }

    if (!fgets(buf, (int)buf_size, fp)) {
        fprintf(stderr,
                "[chorechill-detect] ERROR: Failed to read '%s': %s\n",
                path, strerror(errno));
        fclose(fp);
        return -1;
    }
    fclose(fp);

    /* Strip trailing newline (and any other trailing whitespace) */
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r' ||
                       buf[len - 1] == ' '  || buf[len - 1] == '\t')) {
        buf[--len] = '\0';
    }

    return 0;
}

/* ============================================================
 * HELPER: str_contains_ci()
 *
 * Case-insensitive substring search:
 * returns 1 if 'needle' is found anywhere inside 'haystack', 0 otherwise.
 * ============================================================ */
static int str_contains_ci(const char *haystack, const char *needle)
{
    if (!haystack || !needle) return 0;

    size_t needle_len = strlen(needle);
    size_t hay_len    = strlen(haystack);

    if (needle_len == 0) return 1;
    if (needle_len > hay_len) return 0;

    for (size_t i = 0; i <= hay_len - needle_len; i++) {
        if (strncasecmp(haystack + i, needle, needle_len) == 0)
            return 1;
    }
    return 0;
}

/* ============================================================
 * main()
 * ============================================================ */
int main(void)
{
    char board_vendor[DMI_BUF_SIZE] = {0};
    char board_name[DMI_BUF_SIZE]   = {0};
    char product_name[DMI_BUF_SIZE] = {0};

    /* --- Read DMI files --------------------------------------------------- */

    /* board_vendor and product_name are informational; board_name drives
     * the primary lookup.  Fail hard only if board_name is unreadable. */
    int vendor_ok  = read_dmi_file(DMI_BOARD_VENDOR,  board_vendor,  sizeof(board_vendor));
    int board_ok   = read_dmi_file(DMI_BOARD_NAME,    board_name,    sizeof(board_name));
    int product_ok = read_dmi_file(DMI_PRODUCT_NAME,  product_name,  sizeof(product_name));

    if (board_ok == -1) {
        /* board_name is the primary key — without it we cannot match anything */
        fprintf(stderr,
                "[chorechill-detect] FATAL: Could not read board name from DMI sysfs.\n"
                "                   Make sure sysfs is mounted and you are running Linux.\n");
        return 1;
    }

    /* --- Log detected hardware to stderr (visible in daemon logs) --------- */
    fprintf(stderr, "[chorechill-detect] Detected hardware:\n");
    fprintf(stderr, "  board_vendor : %s\n",
            vendor_ok  == 0 ? board_vendor  : "(unreadable)");
    fprintf(stderr, "  board_name   : %s\n",  board_name);
    fprintf(stderr, "  product_name : %s\n",
            product_ok == 0 ? product_name : "(unreadable)");

    /* --- Lookup: exact board_name substring match -------------------------- */
    const char *chosen_plugin = NULL;

    for (const board_entry_t *e = board_table; e->board_substring != NULL; e++) {
        if (str_contains_ci(board_name, e->board_substring)) {
            chosen_plugin = e->plugin_filename;
            fprintf(stderr,
                    "[chorechill-detect] Matched board substring '%s' → %s\n",
                    e->board_substring, chosen_plugin);
            break;
        }
    }

    /* --- Fallback: MSI vendor catch-all ------------------------------------ */
    if (!chosen_plugin) {
        if (str_contains_ci(board_vendor, "msi") ||
            str_contains_ci(board_vendor, "micro-star")) {
            chosen_plugin = PLUGIN_MSI_LEGACY;
            fprintf(stderr,
                    "[chorechill-detect] No specific board match; "
                    "MSI vendor detected → falling back to %s\n",
                    chosen_plugin);
        }
    }

    /* --- Final fallback: completely unknown hardware ----------------------- */
    if (!chosen_plugin) {
        chosen_plugin = PLUGIN_UNKNOWN;
        fprintf(stderr,
                "[chorechill-detect] WARNING: Unrecognised hardware. "
                "No plugin match found.\n"
                "                   Please report your board_vendor/board_name "
                "on the project issue tracker.\n"
                "                   Falling back to: %s\n",
                chosen_plugin);
    }

    /* --- Print ONLY the plugin filename to stdout -------------------------- */
    printf("%s\n", chosen_plugin);

    return 0;
}
