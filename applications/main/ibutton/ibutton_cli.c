#include <furi.h>
#include <furi_hal.h>

#include <cli/cli_main_commands.h>
#include <toolbox/args.h>
#include <toolbox/pipe.h>

#include <ibutton/ibutton_key.h>
#include <ibutton/ibutton_worker.h>
#include <ibutton/ibutton_protocols.h>

static void ibutton_cli_print_usage(void) {
    printf("Usage:\r\n");
    printf("ikey read\r\n");
    printf("ikey emulate <key_type> <key_data>\r\n");
    printf("ikey write Dallas <key_data>\r\n");
    printf("\t<key_type> choose from:\r\n");
    printf("\tDallas (8 bytes key_data)\r\n");
    printf("\tCyfral (2 bytes key_data)\r\n");
    printf("\tMetakom (4 bytes key_data), must contain correct parity\r\n");
    printf("\t<key_data> are hex-formatted\r\n");
}

static bool ibutton_cli_parse_key(iButtonProtocols* protocols, iButtonKey* key, FuriString* args) {
    bool result = false;
    FuriString* name = furi_string_alloc();

    do {
        // Read protocol name
        if(!args_read_string_and_trim(args, name)) break;

        // Make the protocol name uppercase
        const char first = furi_string_get_char(name, 0);
        furi_string_set_char(name, 0, toupper((int)first));

        const iButtonProtocolId id =
            ibutton_protocols_get_id_by_name(protocols, furi_string_get_cstr(name));
        if(id == iButtonProtocolIdInvalid) break;

        ibutton_key_set_protocol_id(key, id);

        // Get the data pointer
        iButtonEditableData data;
        ibutton_protocols_get_editable_data(protocols, key, &data);

        // Read data
        if(!args_read_hex_bytes(args, data.ptr, data.size)) break;

        result = true;
    } while(false);

    furi_string_free(name);
    return result;
}

static void ibutton_cli_print_key(iButtonProtocols* protocols, iButtonKey* key) {
    const char* name = ibutton_protocols_get_name(protocols, ibutton_key_get_protocol_id(key));

    if(strncmp(name, "DS", 2) == 0) {
        name = "Dallas";
    }

    printf("%s ", name);

    iButtonEditableData data;
    ibutton_protocols_get_editable_data(protocols, key, &data);

    for(size_t i = 0; i < data.size; i++) {
        printf("%02X", data.ptr[i]);
    }

    printf("\r\n");
}

#define EVENT_FLAG_IBUTTON_COMPLETE (1 << 0)

static void ibutton_cli_worker_read_cb(void* context) {
    furi_assert(context);
    FuriEventFlag* event = context;
    furi_event_flag_set(event, EVENT_FLAG_IBUTTON_COMPLETE);
}

static void ibutton_cli_read(PipeSide* pipe) {
    iButtonProtocols* protocols = ibutton_protocols_alloc();
    iButtonWorker* worker = ibutton_worker_alloc(protocols);
    iButtonKey* key = ibutton_key_alloc(ibutton_protocols_get_max_data_size(protocols));
    FuriEventFlag* event = furi_event_flag_alloc();

    ibutton_worker_start_thread(worker);
    ibutton_worker_read_set_callback(worker, ibutton_cli_worker_read_cb, event);

    printf("Reading iButton...\r\nPress Ctrl+C to abort\r\n");
    ibutton_worker_read_start(worker, key);

    while(true) {
        uint32_t flags =
            furi_event_flag_wait(event, EVENT_FLAG_IBUTTON_COMPLETE, FuriFlagWaitAny, 100);

        if(flags & EVENT_FLAG_IBUTTON_COMPLETE) {
            ibutton_cli_print_key(protocols, key);
            break;
        }

        if(cli_is_pipe_broken_or_is_etx_next_char(pipe)) break;
    }

    ibutton_worker_stop(worker);
    ibutton_worker_stop_thread(worker);

    ibutton_key_free(key);
    ibutton_worker_free(worker);
    ibutton_protocols_free(protocols);

    furi_event_flag_free(event);
}

typedef struct {
    FuriEventFlag* event;
    iButtonWorkerWriteResult result;
} iButtonWriteContext;

static void ibutton_cli_worker_write_cb(void* context, iButtonWorkerWriteResult result) {
    furi_assert(context);
    iButtonWriteContext* write_context = (iButtonWriteContext*)context;
    write_context->result = result;
    furi_event_flag_set(write_context->event, EVENT_FLAG_IBUTTON_COMPLETE);
}

void ibutton_cli_write(PipeSide* pipe, FuriString* args) {
    iButtonProtocols* protocols = ibutton_protocols_alloc();
    iButtonWorker* worker = ibutton_worker_alloc(protocols);
    iButtonKey* key = ibutton_key_alloc(ibutton_protocols_get_max_data_size(protocols));

    iButtonWriteContext write_context;
    write_context.event = furi_event_flag_alloc();

    ibutton_worker_start_thread(worker);
    ibutton_worker_write_set_callback(worker, ibutton_cli_worker_write_cb, &write_context);

    do {
        if(!ibutton_cli_parse_key(protocols, key, args)) {
            ibutton_cli_print_usage();
            break;
        }

        if(!(ibutton_protocols_get_features(protocols, ibutton_key_get_protocol_id(key)) &
             iButtonProtocolFeatureWriteId)) {
            ibutton_cli_print_usage();
            break;
        }

        printf("Writing key ");
        ibutton_cli_print_key(protocols, key);
        printf("Press Ctrl+C to abort\r\n");

        ibutton_worker_write_id_start(worker, key);
        while(true) {
            uint32_t flags = furi_event_flag_wait(
                write_context.event, EVENT_FLAG_IBUTTON_COMPLETE, FuriFlagWaitAny, 100);

            if(flags & EVENT_FLAG_IBUTTON_COMPLETE) {
                if(write_context.result == iButtonWorkerWriteSameKey ||
                   write_context.result == iButtonWorkerWriteOK) {
                    printf("Write success\r\n");
                    break;
                } else if(write_context.result == iButtonWorkerWriteCannotWrite) {
                    printf("Write fail\r\n");
                    break;
                }
            }

            if(cli_is_pipe_broken_or_is_etx_next_char(pipe)) break;
        }
    } while(false);

    ibutton_worker_stop(worker);
    ibutton_worker_stop_thread(worker);

    ibutton_key_free(key);
    ibutton_worker_free(worker);
    ibutton_protocols_free(protocols);

    furi_event_flag_free(write_context.event);
}

void ibutton_cli_emulate(PipeSide* pipe, FuriString* args) {
    iButtonProtocols* protocols = ibutton_protocols_alloc();
    iButtonWorker* worker = ibutton_worker_alloc(protocols);
    iButtonKey* key = ibutton_key_alloc(ibutton_protocols_get_max_data_size(protocols));

    ibutton_worker_start_thread(worker);

    do {
        if(!ibutton_cli_parse_key(protocols, key, args)) {
            ibutton_cli_print_usage();
            break;
        }

        printf("Emulating key ");
        ibutton_cli_print_key(protocols, key);
        printf("Press Ctrl+C to abort\r\n");

        ibutton_worker_emulate_start(worker, key);

        while(!cli_is_pipe_broken_or_is_etx_next_char(pipe)) {
            furi_delay_ms(100);
        };

    } while(false);

    ibutton_worker_stop(worker);
    ibutton_worker_stop_thread(worker);

    ibutton_key_free(key);
    ibutton_worker_free(worker);
    ibutton_protocols_free(protocols);
}

static void execute(PipeSide* pipe, FuriString* args, void* context) {
    UNUSED(context);
    FuriString* cmd;
    cmd = furi_string_alloc();

    if(!args_read_string_and_trim(args, cmd)) {
        furi_string_free(cmd);
        ibutton_cli_print_usage();
        return;
    }

    if(furi_string_cmp_str(cmd, "read") == 0) {
        ibutton_cli_read(pipe);
    } else if(furi_string_cmp_str(cmd, "write") == 0) {
        ibutton_cli_write(pipe, args);
    } else if(furi_string_cmp_str(cmd, "emulate") == 0) {
        ibutton_cli_emulate(pipe, args);
    } else {
        ibutton_cli_print_usage();
    }

    furi_string_free(cmd);
}

CLI_COMMAND_INTERFACE(ikey, execute, CliCommandFlagDefault, 1024, CLI_APPID);
