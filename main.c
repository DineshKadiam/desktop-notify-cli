#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dbus-1.0/dbus/dbus.h>

#include "version.h"

#define IFACE "org.freedesktop.Notifications"

enum urgency {
    URGENCY_LOW,
    URGENCY_NORMAL,
    URGENCY_CRITICAL,
};

struct action {
    const char *name;
    const char *label;
};

struct hint {
    int type;  /* DBUS_TYPE_* */
    const char *type_as_string;

    const char *name;

    union {
        uint8_t byte;
        int32_t integer;
        uint32_t boolean;
        double floating;
        const char *string;
    } u;
};

static volatile sig_atomic_t got_sigint = 0;

static void
sigint_handler(int signo)
{
    assert(signo == SIGINT);
    got_sigint = 1;
}

static void
usage(const char *progname)
{
    printf(
        "Usage: %s [OPTIONS…] TITLE [MESSAGE]\n"
        "       %s --close=ID\n"
        "       %s --server-info\n"
        "       %s --server-capabilities\n\n"
        ""
        "Options:\n"
        "  -a,--app-name=NAME          app-name, typically used for the icon, unless\n"
        "                              --icon is used\n"
        "  -i,--icon=ICON              notification icon. Can be either a symbolic name,\n"
        "                              or a file (default: none)\n"
        "  -u,--urgency=URGENCY        notification urgency; low, normal or critical\n"
        "                              (default: normal)\n"
        "  -c,--category=CATEGORY      notification category (default: none)\n"
        "  -A,--action=NAME:LABEL      defines an action to display. Implies --wait. May\n"
        "                              be set multiple times. The name of the\n"
        "                              triggered action is output to stdout\n"
        "  -H,--hint=TYPE:NAME:VALUE   specifies additional hints. Valid types are\n"
        "                              boolean, byte, int, double and string\n"
        "  -r,--replaces=ID            update an existing notification\n"
        "  -t,--expire-time=TIME_MS    notification timeout, in milliseconds (default:\n"
        "                              server defined)\n"
        "     --transient              show a transient notification. Transient\n"
        "                              notifications by-pass the server's persistence\n"
        "                              capability, if any.\n"
        "  -C,--close=ID               close an existing notification\n"
        "  -p,--print-id               print the notification ID on stdout\n"
        "  -R,--print-reason           print the reason the notification was closed\n"
        "  -T,--print-token            print the activation token, if present\n"
        "  -w,--wait                   wait for the notification to be closed before\n"
        "                              exiting\n"
        "     --server-info            display server name and version\n"
        "     --server-capabilities    display server capabilities\n"
        "  -v,--version                show the version number and quit\n"
        , progname, progname, progname, progname);
}

static const char *
version_and_features(void)
{
    static char buf[256];
    snprintf(buf, sizeof(buf), "version: %s", FYI_VERSION);
    return buf;
}

static bool
send_close_notification(DBusConnection *conn, uint32_t notification_id)
{
    DBusMessage *msg = dbus_message_new_method_call(
        IFACE, "/org/freedesktop/Notifications",
        IFACE, "CloseNotification");

    /* Mimic notify-send: close the notification */
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, 'u', &notification_id);
    dbus_connection_send(conn, msg, 0);
    dbus_connection_flush(conn);
    dbus_message_unref(msg);
    return true;
}

static bool
send_server_information(DBusConnection *conn)
{
    DBusError err = DBUS_ERROR_INIT;
    bool ret = false;

    DBusMessage *msg = dbus_message_new_method_call(
        IFACE, "/org/freedesktop/Notifications",
        IFACE, "GetServerInformation");

    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        conn, msg, DBUS_TIMEOUT_USE_DEFAULT, &err);

    if (dbus_error_is_set(&err)) {
        fprintf(
            stderr,
            "error: failed to get server information: %s\n", err.message);
        goto out;
    }

    const char *name;
    const char *vendor;
    const char *version;
    const char *spec_version;

    if (!dbus_message_get_args(reply, &err,
                               DBUS_TYPE_STRING, &name,
                               DBUS_TYPE_STRING, &vendor,
                               DBUS_TYPE_STRING, &version,
                               DBUS_TYPE_STRING, &spec_version,
                               DBUS_TYPE_INVALID))
    {
        fprintf(stderr, "error: failed to parse reply: %s\n", err.message);
        goto out;
    }

    printf("name: %s\n", name);
    printf("vendor: %s\n", vendor);
    printf("version: %s\n", version);
    printf("spec-version: %s\n", spec_version);

    ret = true;

out:
    dbus_message_unref(msg);
    dbus_message_unref(reply);
    dbus_error_free(&err);
    return ret;
}

static bool
send_server_capabilities(DBusConnection *conn)
{
    DBusError err = DBUS_ERROR_INIT;
    bool ret = false;

    DBusMessage *msg = dbus_message_new_method_call(
        IFACE, "/org/freedesktop/Notifications",
        IFACE, "GetCapabilities");

    DBusMessage *reply = dbus_connection_send_with_reply_and_block(
        conn, msg, DBUS_TIMEOUT_USE_DEFAULT, &err);

    if (dbus_error_is_set(&err)) {
        fprintf(
            stderr,
            "error: failed to get server capabilities: %s\n", err.message);

        goto out;
    }

    DBusMessageIter vals;
    dbus_message_iter_init(reply, &vals);
    if (dbus_message_iter_get_arg_type(&vals) != DBUS_TYPE_ARRAY) {
        fprintf(stderr, "error: expected an array of strings\n");
        goto out;
    }

    DBusMessageIter caps;
    dbus_message_iter_recurse(&vals, &caps);

    while (dbus_message_iter_get_arg_type(&caps) != DBUS_TYPE_INVALID) {
        if (dbus_message_iter_get_arg_type(&caps) != DBUS_TYPE_STRING) {
            fprintf(stderr, "error: non-string capability\n");
            goto out;
        }

        const char *cap;
        dbus_message_iter_get_basic(&caps, &cap);
        printf("%s\n", cap);

        dbus_message_iter_next(&caps);
    }

    dbus_message_iter_next(&vals);
    assert(dbus_message_iter_get_arg_type(&vals) == DBUS_TYPE_INVALID);

    ret = true;

out:
    dbus_message_unref(reply);
    dbus_message_unref(msg);
    dbus_error_free(&err);

    return ret;
}

static bool
notification_closed_handler(DBusMessage *signal, uint32_t *id, uint32_t *reason)
{
    DBusError err = DBUS_ERROR_INIT;
    bool ret = false;

    if (!dbus_message_get_args(signal, &err,
                               DBUS_TYPE_UINT32, id,
                               DBUS_TYPE_UINT32, reason,
                               DBUS_TYPE_INVALID))
    {
        fprintf(
            stderr,
            "error: NotificationClosed: failed to parse signal arguments: %s\n",
            err.message);

        goto out;
    }

    ret = true;

out:
    dbus_error_free(&err);
    dbus_message_unref(signal);
    return ret;
}

static bool
action_invoked_handler(DBusMessage *signal, uint32_t *id, char **action_name)
{
    DBusError err = DBUS_ERROR_INIT;
    bool ret = false;

    if (action_name != NULL)
        *action_name = NULL;

    const char *action;
    if (!dbus_message_get_args(signal, &err,
                               DBUS_TYPE_UINT32, id,
                               DBUS_TYPE_STRING, &action,
                               DBUS_TYPE_INVALID))
    {
        fprintf(
            stderr,
            "error: ActionInvoked: failed to parse signal arguments: %s\n",
            err.message);

        goto out;
    }

    if (action_name != NULL)
        *action_name = strdup(action);

    ret = true;

out:
    dbus_error_free(&err);
    dbus_message_unref(signal);
    return ret;
}

static bool
activation_token_handler(DBusMessage *signal, uint32_t *id, char **token)
{
    DBusError err = DBUS_ERROR_INIT;
    bool ret = false;

    if (token != NULL)
        *token = NULL;

    const char *tok;
    if (!dbus_message_get_args(signal, &err,
                               DBUS_TYPE_UINT32, id,
                               DBUS_TYPE_STRING, &tok,
                               DBUS_TYPE_INVALID))
    {
        fprintf(
            stderr,
            "error: ActivationToken: failed to parse signal arguments: %s\n",
            err.message);

        goto out;
    }

    if (token != NULL)
        *token = strdup(tok);

    ret = true;

out:
    dbus_error_free(&err);
    dbus_message_unref(signal);
    return ret;
}

int
main(int argc, char *const *argv)
{
    int ret = EXIT_FAILURE;

    /* Disable buffering. This ensures e.g. the ID is flushed immediately */
    setbuf(stdout, NULL);

#define OPT_SERVER_INFO         256
#define OPT_SERVER_CAPABILITIES 257
#define OPT_TRANSIENT           258

    const struct option longopts[] = {
        {"app-name",            required_argument, NULL, 'a'},
        {"icon",                required_argument, NULL, 'i'},
        {"urgency",             required_argument, NULL, 'u'},
        {"category",            required_argument, NULL, 'c'},
        {"action",              required_argument, NULL, 'A'},
        {"hint",                required_argument, NULL, 'H'},
        {"replaces",            required_argument, NULL, 'r'},
        {"expire-time",         required_argument, NULL, 't'},
        {"close",               required_argument, NULL, 'C'},
        {"transient",           no_argument,       NULL, OPT_TRANSIENT},
        {"print-id",            no_argument,       NULL, 'p'},
        {"print-reason",        no_argument,       NULL, 'R'},
        {"print-token",         no_argument,       NULL, 'T'},
        {"wait",                no_argument,       NULL, 'w'},
        {"server-info",         no_argument,       NULL, OPT_SERVER_INFO},
        {"server-capabilities", no_argument,       NULL, OPT_SERVER_CAPABILITIES},
        {"version",             no_argument,       NULL, 'v'},
        {"help",                no_argument,       NULL, 'h'},

        {NULL,                  no_argument,       NULL, 0},
    };

    const char *progname = argv[0];
    const char *app_id = program_invocation_name;
    const char *icon = NULL;
    const char *category = NULL;

    uint32_t replaces_id = 0;
    uint32_t close_id = 0;
    int32_t expire_time = -1;
    enum urgency urgency = URGENCY_NORMAL;

    struct action *actions = NULL;
    size_t action_count = 0;

    struct hint *hints = NULL;
    size_t hint_count = 0;

    bool print_id = false;
    bool print_reason = false;
    bool print_token = false;
    bool wait = false;
    bool transient = false;
    bool server_info = false;
    bool server_capabilities = false;

    char *body = NULL;
    char *icon_uri = NULL;

    DBusError err = DBUS_ERROR_INIT;
    DBusMessage *msg = NULL;
    DBusMessage *reply = NULL;
    DBusConnection *conn = NULL;

    while (true) {
        int c = getopt_long(argc, argv, "+a:i:u:c:A:H:r:t:C:pRTwhv", longopts, NULL);

        if (c < 0)
            break;

        switch (c) {
        case 'a': app_id = optarg; break;
        case 'i': icon = optarg; break;
        case 'c': category = optarg; break;
        case 'p': print_id = true; break;
        case 'R': print_reason = true; break;
        case 'T': print_token = true; break;
        case 'w': wait = true; break;
        case OPT_TRANSIENT: transient = true; break;

        case 'A': {
            char *split = strchr(optarg, ':');
            if (split == NULL) {
                fprintf(stderr, "error: invalid action: %s\n", optarg);
                goto out;
            }

            *split = '\0';

            const char *name = optarg;
            const char *label = split + 1;
            actions = realloc(actions, (action_count + 1) * sizeof(actions[0]));
            actions[action_count++] = (struct action){name, label};
            break;
        }

        case 'H': {
            const char *type_str = optarg;
            const char *name = NULL;
            const char *value = NULL;

            char *split = strchr(type_str, ':');
            if (split != NULL) {
                *split = '\0';
                name = split + 1;

                split = strchr(name, ':');
                if (split != NULL) {
                    *split = '\0';
                    value = split + 1;
                }
            }

            if (name == NULL || value == NULL) {
                fprintf(stderr, "error: invalid hint: %s\n", optarg);
                goto out;
            }

            struct hint hint = {.name = name};

            errno = 0;
            char *end = NULL;

            if (strcmp(type_str, "boolean") == 0) {
                hint.type = DBUS_TYPE_BOOLEAN;
                hint.type_as_string = DBUS_TYPE_BOOLEAN_AS_STRING;
                hint.u.boolean =
                    strcmp(value, "1") == 0 ||
                    strcasecmp(value, "on") == 0 ||
                    strcasecmp(value, "true") == 0;
            } else if (strcmp(type_str, "byte") == 0) {
                hint.type = DBUS_TYPE_BYTE;
                hint.type_as_string = DBUS_TYPE_BYTE_AS_STRING;
                hint.u.byte = strtoul(value, &end, 10);
            } else if (strcmp(type_str, "int") == 0) {
                hint.type = DBUS_TYPE_INT32;
                hint.type_as_string = DBUS_TYPE_INT32_AS_STRING;
                hint.u.integer = strtol(value, &end, 10);
            } else if (strcmp(type_str, "double") == 0) {
                hint.type = DBUS_TYPE_DOUBLE;
                hint.type_as_string = DBUS_TYPE_DOUBLE_AS_STRING;
                hint.u.floating = strtod(value, &end);
            } else if (strcmp(type_str, "string") == 0) {
                hint.type = DBUS_TYPE_STRING;
                hint.type_as_string = DBUS_TYPE_STRING_AS_STRING;
                hint.u.string = value;
            } else {
                fprintf(stderr, "error: invalid hint type: %s\n", type_str);
                goto out;
            }

            if ((hint.type == DBUS_TYPE_BYTE ||
                 hint.type == DBUS_TYPE_INT32 ||
                 hint.type == DBUS_TYPE_DOUBLE) &&
                (errno != 0 || end == NULL || *end != '\0'))
            {
                fprintf(stderr, "error: invalid hint value: %s\n", value);
                goto out;
            }

            hints = realloc(hints, (hint_count + 1) * sizeof(hints[0]));
            hints[hint_count++] = hint;
            break;
        }

        case 'r': {
            errno = 0;
            char *end = NULL;
            unsigned long id = strtoul(optarg, &end, 10);
            if (errno == 0 && end != NULL && *end == '\0')
                replaces_id = (uint32_t)id;
            else {
                fprintf(stderr, "error: %s: invalid ID\n", optarg);
                goto out;
            }
            break;
        }

        case 't': {
            errno = 0;
            char *end = NULL;
            long timeout = strtol(optarg, &end, 10);
            if (errno == 0 && end != NULL && *end == '\0')
                expire_time = (int32_t)timeout;
            else {
                fprintf(stderr, "error: %s: invalid expire time\n", optarg);
                goto out;
            }
            break;
        }

        case 'u':
            if (strcmp(optarg, "low") == 0)
                urgency = URGENCY_LOW;
            else if (strcmp(optarg, "normal") == 0)
                urgency = URGENCY_NORMAL;
            else if (strcmp(optarg, "critical") == 0)
                urgency = URGENCY_CRITICAL;
            else {
                fprintf(stderr, "error: %s: invalid urgency level\n", optarg);
                goto out;
            }
            break;

        case 'C': {
            errno = 0;
            char *end = NULL;
            unsigned long id = strtoul(optarg, &end, 10);
            if (errno == 0 && end != NULL && *end == '\0')
                close_id = (uint32_t)id;
            else {
                fprintf(stderr, "error: %s: invalid ID\n", optarg);
                goto out;
            }
            break;
        }

        case OPT_SERVER_INFO:
            server_info = true;
            break;

        case OPT_SERVER_CAPABILITIES:
            server_capabilities = true;
            break;

        case 'v': printf("fyi %s\n", version_and_features()); ret = EXIT_SUCCESS; goto out;
        case 'h': usage(progname); ret = EXIT_SUCCESS; goto out;
        case '?': ret = EXIT_FAILURE; goto out;
        }
    }

    if (argc > 0) {
        argc -= optind;
        argv += optind;
    }

    if (argc == 0 && close_id == 0 && !server_info && !server_capabilities) {
        usage(progname);
        goto out;
    }

    const char *title = argv[0];
    uint32_t transient_value = transient; /* D-Bus "boolean" */
    uint8_t urgency_value = urgency;      /* D-Bus "byte" */

    for (size_t i = 1, len = 0; i < argc; i++) {
        const char *word = argv[i];
        size_t word_length = strlen(word);

        body = realloc(body, len + 1 + word_length + 1);
        body[len] = ' ';
        memcpy(&body[len + 1], word, word_length);

        len += 1 + word_length;
        body[len] = '\0';
    }

    if (body == NULL)
        body = strdup("");

    if (action_count > 0 || print_reason || print_token) {
        /* Actions implies wait */
        wait = true;
    }

    /* If 'icon' exists on disk, treat it as a filename, otherwise as
       a symbolic icon name */
    if (icon != NULL) {
        char *path = realpath(icon, NULL);
        if (path != NULL) {
            icon_uri = malloc(strlen("file://") + strlen(path) + 1);
            strcpy(icon_uri, "file://");
            strcat(icon_uri, path);
            free(path);
        }
    }

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "error: failed to connect: %s\n", err.message);
        goto out;
    }

    if (server_info) {
        ret = send_server_information(conn) ? EXIT_SUCCESS : EXIT_FAILURE;
        goto out;
    }

    if (server_capabilities) {
        ret = send_server_capabilities(conn) ? EXIT_SUCCESS : EXIT_FAILURE;
        goto out;
    }

    if (close_id > 0) {
        ret = send_close_notification(conn, close_id) ? EXIT_SUCCESS : EXIT_FAILURE;
        goto out;
    }

    msg = dbus_message_new_method_call(
        IFACE, "/org/freedesktop/Notifications",
        IFACE, "Notify");

    const char *icon_arg = icon != NULL ? icon : "";
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, 's', &app_id);
    dbus_message_iter_append_basic(&args, 'u', &replaces_id);
    dbus_message_iter_append_basic(&args, 's', &icon_arg);
    dbus_message_iter_append_basic(&args, 's', &title);
    dbus_message_iter_append_basic(&args, 's', &body);

    /* Actions: array of strings. Every even item is the action name,
       every odd item is the action label */
    DBusMessageIter args_actions;
    dbus_message_iter_open_container(&args, 'a', "s", &args_actions);
    for (size_t i = 0; i < action_count; i++) {
        const struct action *a = &actions[i];
        dbus_message_iter_append_basic(&args_actions, 's', &a->name);
        dbus_message_iter_append_basic(&args_actions, 's', &a->label);
    }
    dbus_message_iter_close_container(&args, &args_actions);

    /* Hints: dictionary of "sv" (string + variant) */
    DBusMessageIter args_hints;
    dbus_message_iter_open_container(&args, 'a', "{sv}", &args_hints);
    {
        DBusMessageIter args_hints_hint;
        DBusMessageIter args_hints_value;

        dbus_message_iter_open_container(&args_hints, 'e', 0, &args_hints_hint);
        {
            static const char *hint_name = "urgency";
            dbus_message_iter_append_basic(&args_hints_hint, 's', &hint_name);
            dbus_message_iter_open_container(&args_hints_hint, 'v', "y", &args_hints_value);
            {
                dbus_message_iter_append_basic(&args_hints_value, 'y', &urgency_value);
            }
            dbus_message_iter_close_container(&args_hints_hint, &args_hints_value);
        }
        dbus_message_iter_close_container(&args_hints, &args_hints_hint);

        if (icon != NULL) {
            dbus_message_iter_open_container(&args_hints, 'e', 0, &args_hints_hint);
            {
                static const char *hint_name = "image-path";
                dbus_message_iter_append_basic(&args_hints_hint, 's', &hint_name);
                dbus_message_iter_open_container(&args_hints_hint, 'v', "s", &args_hints_value);
                {
                    if (icon_uri != NULL)
                        dbus_message_iter_append_basic(&args_hints_value, 's', &icon_uri);
                    else
                    dbus_message_iter_append_basic(&args_hints_value, 's', &icon);
                }
                dbus_message_iter_close_container(&args_hints_hint, &args_hints_value);
            }
            dbus_message_iter_close_container(&args_hints, &args_hints_hint);
        }

        if (category != NULL) {
            dbus_message_iter_open_container(&args_hints, 'e', 0, &args_hints_hint);
            {
                static const char *hint_name = "category";
                dbus_message_iter_append_basic(&args_hints_hint, 's', &hint_name);
                dbus_message_iter_open_container(&args_hints_hint, 'v', "s", &args_hints_value);
                {
                    dbus_message_iter_append_basic(&args_hints_value, 's', &category);
                }
                dbus_message_iter_close_container(&args_hints_hint, &args_hints_value);
            }
            dbus_message_iter_close_container(&args_hints, &args_hints_hint);
        }

        if (transient) {
            dbus_message_iter_open_container(&args_hints, 'e', 0, &args_hints_hint);
            {
                static const char *hint_name = "transient";
                dbus_message_iter_append_basic(&args_hints_hint, 's', &hint_name);
                dbus_message_iter_open_container(&args_hints_hint, 'v', "b", &args_hints_value);
                {
                    dbus_message_iter_append_basic(&args_hints_value, 'b', &transient_value);
                }
                dbus_message_iter_close_container(&args_hints_hint, &args_hints_value);
            }
            dbus_message_iter_close_container(&args_hints, &args_hints_hint);
        }

        /* User specified hints */
        for (size_t i = 0; i < hint_count; i++) {
            const struct hint *h = &hints[i];
            dbus_message_iter_open_container(&args_hints, 'e', 0, &args_hints_hint);
            {
                dbus_message_iter_append_basic(&args_hints_hint, 's', &h->name);
                dbus_message_iter_open_container(&args_hints_hint, 'v', h->type_as_string, &args_hints_value);
                {
                    dbus_message_iter_append_basic(&args_hints_value, h->type, &h->u);
                }
                dbus_message_iter_close_container(&args_hints_hint, &args_hints_value);
            }
            dbus_message_iter_close_container(&args_hints, &args_hints_hint);
        }
    }
    dbus_message_iter_close_container(&args, &args_hints);

    /* Expire timeout */
    dbus_message_iter_append_basic(&args, 'i', &expire_time);

    /* Sign up for signals *before* we send the notification, to avoid race */
    if (wait) {
        dbus_bus_add_match(conn, "type='signal',interface='" IFACE "'", &err);
        if (dbus_error_is_set(&err)) {
            fprintf(stderr, "error: failed to register for notification signals: %s", err.message);
            goto out;
        }
    }

    /* Send notification, and wait for reply */
    reply = dbus_connection_send_with_reply_and_block(
        conn, msg, DBUS_TIMEOUT_USE_DEFAULT, &err);

    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "error: failed to send notification: %s\n", err.message);
        goto out;
    }

    dbus_message_unref(msg);
    msg = NULL;

    uint32_t notification_id;
    if (!dbus_message_get_args(reply, &err,
                               DBUS_TYPE_UINT32, &notification_id,
                               DBUS_TYPE_INVALID))
    {
        fprintf(stderr, "error: failed to parse reply: %s\n", err.message);
        goto out;
    }

    dbus_message_unref(reply);
    reply = NULL;

    if (print_id)
        printf("id=%u\n", notification_id);

    if (!wait) {
        ret = EXIT_SUCCESS;
    } else {
        bool connected = true;

        struct sigaction act = {.sa_handler = &sigint_handler};
        sigemptyset(&act.sa_mask);

        sigaction(SIGINT, &act, NULL);

        int dbus_fd = -1;
        if (!dbus_connection_get_unix_fd(conn, &dbus_fd)) {
            fprintf(stderr, "error: failed to get dbus socket\n");
            goto out;
        }

        while (true) {
            DBusMessage *signal = dbus_connection_pop_message(conn);

            if (signal == NULL) {
                /*
                 * No more queued up messages. Need to first wait for
                 * the connection to become readable, then pull in
                 * more messages into the queue.
                 */

                if (!connected)
                    break;

                /* Wait for D-Bus connection to become readable again */
                while (true) {
                    struct pollfd fds[] = {{.fd = dbus_fd, .events = POLLIN}};
                    int poll_ret = poll(fds, 1, -1);

                    if (poll_ret < 0) {
                        if (errno == EINTR) {
                            if (got_sigint) {
                                send_close_notification(conn, notification_id);

                                /* Continue processing messages.
                                 *
                                 * We'll exit when we get the
                                 * NotificationClosed signal.
                                 */
                            }

                            /* Poll again */
                            continue;
                        } else {
                            fprintf(stderr, "error: failed to poll: %s\n",
                                    strerror(errno));
                            goto out;
                        }
                    }

                    if (fds[0].revents & POLLHUP) {
                        fprintf(stderr, "error: disconnected\n");
                        connected = false;
                    }

                    /* Messages to read - exit the poll loop */
                    break;
                }

                if (connected && !dbus_connection_read_write(conn, 0)) {
                    fprintf(stderr, "error: disconnected\n");
                    connected = false;
                    /* Continue popping messages until local queue is empty */
                }

                continue;
            }

            if (dbus_message_is_signal(signal, IFACE, "NotificationClosed")) {
                uint32_t id;
                uint32_t reason;

                if (!notification_closed_handler(signal, &id, &reason))
                    break;

                if (id == notification_id) {
                    if (print_reason) {
                        switch (reason) {
                        case 1: printf("reason: expired\n"); break;
                        case 2: printf("reason: dismissed\n"); break;
                        case 3: printf("reason: force-closed\n"); break;
                        default: printf("reason: unknown\n"); break;
                        }
                    }
                    ret = EXIT_SUCCESS;
                    break;
                }
            }

            else if (dbus_message_is_signal(signal, IFACE, "ActionInvoked")) {
                uint32_t id;
                char *action_name = NULL;

                if (!action_invoked_handler(signal, &id, &action_name))
                    break;

                if (id == notification_id) {
                    printf("action=%s\n", action_name);
                    send_close_notification(conn, notification_id);
                }

                free(action_name);
            }

            else if (dbus_message_is_signal(signal, IFACE, "ActivationToken")) {
                uint32_t id;
                char *token = NULL;

                if (!activation_token_handler(signal, &id, &token))
                    break;

                if (id == notification_id && print_token)
                    printf("xdgtoken=%s\n", token);

                free(token);
            } else
                dbus_message_unref(signal);
        }
    }

out:
    if (reply != NULL)
        dbus_message_unref(reply);
    if (msg != NULL)
        dbus_message_unref(msg);
    if (conn != NULL)
        dbus_connection_unref(conn);

    dbus_error_free(&err);
    free(icon_uri);
    free(body);
    free(hints);
    free(actions);

    return ret;
}
