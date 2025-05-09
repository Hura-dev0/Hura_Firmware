#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <gui/elements.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/dialog_ex.h>

#include <lib/toolbox/strint.h>

#include <notification/notification.h>
#include <notification/notification_messages.h>

#define TAG "UartEcho"

#define LINES_ON_SCREEN   6
#define COLUMNS_ON_SCREEN 21
#define DEFAULT_BAUD_RATE 230400
#define DEFAULT_DATA_BITS FuriHalSerialDataBits8
#define DEFAULT_PARITY    FuriHalSerialParityNone
#define DEFAULT_STOP_BITS FuriHalSerialStopBits1

typedef struct UartDumpModel UartDumpModel;

typedef struct {
    Gui* gui;
    NotificationApp* notification;
    ViewDispatcher* view_dispatcher;
    View* view;
    FuriThread* worker_thread;
    FuriStreamBuffer* rx_stream;
    FuriHalSerialHandle* serial_handle;
} UartEchoApp;

typedef struct {
    FuriString* text;
} ListElement;

struct UartDumpModel {
    ListElement* list[LINES_ON_SCREEN];
    uint8_t line;

    char last_char;
    bool escape;
};

typedef enum {
    WorkerEventReserved = (1 << 0), // Reserved for StreamBuffer internal event
    WorkerEventStop = (1 << 1),
    WorkerEventRxData = (1 << 2),
    WorkerEventRxIdle = (1 << 3),
    WorkerEventRxOverrunError = (1 << 4),
    WorkerEventRxFramingError = (1 << 5),
    WorkerEventRxNoiseError = (1 << 6),
    WorkerEventRxParityError = (1 << 7),
} WorkerEventFlags;

#define WORKER_EVENTS_MASK                                                                 \
    (WorkerEventStop | WorkerEventRxData | WorkerEventRxIdle | WorkerEventRxOverrunError | \
     WorkerEventRxFramingError | WorkerEventRxNoiseError | WorkerEventRxParityError)

const NotificationSequence sequence_notification = {
    &message_display_backlight_on,
    &message_green_255,
    &message_delay_10,
    NULL,
};

const NotificationSequence sequence_error = {
    &message_display_backlight_on,
    &message_red_255,
    &message_delay_10,
    NULL,
};

static void uart_echo_view_draw_callback(Canvas* canvas, void* _model) {
    UartDumpModel* model = _model;

    // Prepare canvas
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontKeyboard);

    for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
        canvas_draw_str(
            canvas,
            0,
            (i + 1) * (canvas_current_font_height(canvas) - 1),
            furi_string_get_cstr(model->list[i]->text));

        if(i == model->line) {
            uint8_t width =
                canvas_string_width(canvas, furi_string_get_cstr(model->list[i]->text));

            canvas_draw_box(
                canvas,
                width,
                (i) * (canvas_current_font_height(canvas) - 1) + 2,
                2,
                canvas_current_font_height(canvas) - 2);
        }
    }
}

static bool uart_echo_view_input_callback(InputEvent* event, void* context) {
    UNUSED(event);
    UNUSED(context);
    return false;
}

static uint32_t uart_echo_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static void
    uart_echo_on_irq_cb(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, void* context) {
    furi_assert(context);
    UNUSED(handle);
    UartEchoApp* app = context;
    volatile FuriHalSerialRxEvent event_copy = event;
    UNUSED(event_copy);

    WorkerEventFlags flag = 0;

    if(event & FuriHalSerialRxEventData) {
        uint8_t data = furi_hal_serial_async_rx(handle);
        furi_stream_buffer_send(app->rx_stream, &data, 1, 0);
        flag |= WorkerEventRxData;
    }

    if(event & FuriHalSerialRxEventIdle) {
        //idle line detected, packet transmission may have ended
        flag |= WorkerEventRxIdle;
    }

    //error detected
    if(event & FuriHalSerialRxEventFrameError) {
        flag |= WorkerEventRxFramingError;
    }
    if(event & FuriHalSerialRxEventNoiseError) {
        flag |= WorkerEventRxNoiseError;
    }
    if(event & FuriHalSerialRxEventOverrunError) {
        flag |= WorkerEventRxOverrunError;
    }
    if(event & FuriHalSerialRxEventParityError) {
        flag |= WorkerEventRxParityError;
    }

    furi_thread_flags_set(furi_thread_get_id(app->worker_thread), flag);
}

static void uart_echo_push_to_list(UartDumpModel* model, const char data) {
    if(model->escape) {
        // escape code end with letter
        if((data >= 'a' && data <= 'z') || (data >= 'A' && data <= 'Z')) {
            model->escape = false;
        }
    } else if(data == '[' && model->last_char == '\e') {
        // "Esc[" is a escape code
        model->escape = true;
    } else if((data >= ' ' && data <= '~') || (data == '\n' || data == '\r')) {
        bool new_string_needed = false;
        if(furi_string_size(model->list[model->line]->text) >= COLUMNS_ON_SCREEN) {
            new_string_needed = true;
        } else if(data == '\n' || data == '\r') {
            // pack line breaks
            if(model->last_char != '\n' && model->last_char != '\r') {
                new_string_needed = true;
            }
        }

        if(new_string_needed) {
            if((model->line + 1) < LINES_ON_SCREEN) {
                model->line += 1;
            } else {
                ListElement* first = model->list[0];

                for(size_t i = 1; i < LINES_ON_SCREEN; i++) {
                    model->list[i - 1] = model->list[i];
                }

                furi_string_reset(first->text);
                model->list[model->line] = first;
            }
        }

        if(data != '\n' && data != '\r') {
            furi_string_push_back(model->list[model->line]->text, data);
        }
    }
    model->last_char = data;
}

static int32_t uart_echo_worker(void* context) {
    furi_assert(context);
    UartEchoApp* app = context;

    while(1) {
        uint32_t events =
            furi_thread_flags_wait(WORKER_EVENTS_MASK, FuriFlagWaitAny, FuriWaitForever);
        furi_check((events & FuriFlagError) == 0);

        if(events & WorkerEventStop) break;
        if(events & WorkerEventRxData) {
            size_t length = 0;
            do {
                uint8_t data[64];
                length = furi_stream_buffer_receive(app->rx_stream, data, 64, 0);
                if(length > 0) {
                    furi_hal_serial_tx(app->serial_handle, data, length);
                    with_view_model(
                        app->view,
                        UartDumpModel * model,
                        {
                            for(size_t i = 0; i < length; i++) {
                                uart_echo_push_to_list(model, data[i]);
                            }
                        },
                        false);
                }
            } while(length > 0);

            notification_message(app->notification, &sequence_notification);
            with_view_model(app->view, UartDumpModel * model, { UNUSED(model); }, true);
        }

        if(events & WorkerEventRxIdle) {
            furi_hal_serial_tx(app->serial_handle, (uint8_t*)"\r\nDetect IDLE\r\n", 15);
        }

        if(events &
           (WorkerEventRxOverrunError | WorkerEventRxFramingError | WorkerEventRxNoiseError)) {
            if(events & WorkerEventRxOverrunError) {
                furi_hal_serial_tx(app->serial_handle, (uint8_t*)"\r\nDetect ORE\r\n", 14);
            }
            if(events & WorkerEventRxFramingError) {
                furi_hal_serial_tx(app->serial_handle, (uint8_t*)"\r\nDetect FE\r\n", 13);
            }
            if(events & WorkerEventRxNoiseError) {
                furi_hal_serial_tx(app->serial_handle, (uint8_t*)"\r\nDetect NE\r\n", 13);
            }
            if(events & WorkerEventRxParityError) {
                furi_hal_serial_tx(app->serial_handle, (uint8_t*)"\r\nDetect PE\r\n", 13);
            }
            notification_message(app->notification, &sequence_error);
        }
    }

    return 0;
}

static UartEchoApp* uart_echo_app_alloc(
    uint32_t baudrate,
    FuriHalSerialDataBits data_bits,
    FuriHalSerialParity parity,
    FuriHalSerialStopBits stop_bits) {
    UartEchoApp* app = malloc(sizeof(UartEchoApp));

    app->rx_stream = furi_stream_buffer_alloc(2048, 1);

    // Gui
    app->gui = furi_record_open(RECORD_GUI);
    app->notification = furi_record_open(RECORD_NOTIFICATION);

    // View dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Views
    app->view = view_alloc();
    view_set_draw_callback(app->view, uart_echo_view_draw_callback);
    view_set_input_callback(app->view, uart_echo_view_input_callback);
    view_allocate_model(app->view, ViewModelTypeLocking, sizeof(UartDumpModel));
    with_view_model(
        app->view,
        UartDumpModel * model,
        {
            for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
                model->line = 0;
                model->escape = false;
                model->list[i] = malloc(sizeof(ListElement));
                model->list[i]->text = furi_string_alloc();
            }
        },
        true);

    view_set_previous_callback(app->view, uart_echo_exit);
    view_dispatcher_add_view(app->view_dispatcher, 0, app->view);
    view_dispatcher_switch_to_view(app->view_dispatcher, 0);

    app->worker_thread = furi_thread_alloc_ex("UsbUartWorker", 1024, uart_echo_worker, app);
    furi_thread_start(app->worker_thread);

    // Enable uart listener
    app->serial_handle = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    furi_check(app->serial_handle);
    furi_hal_serial_init(app->serial_handle, baudrate);
    furi_hal_serial_configure_framing(app->serial_handle, data_bits, parity, stop_bits);

    furi_hal_serial_async_rx_start(app->serial_handle, uart_echo_on_irq_cb, app, true);

    return app;
}

static void uart_echo_app_free(UartEchoApp* app) {
    furi_assert(app);

    furi_hal_serial_async_rx_stop(app->serial_handle);
    furi_hal_serial_deinit(app->serial_handle);
    furi_hal_serial_control_release(app->serial_handle);

    furi_thread_flags_set(furi_thread_get_id(app->worker_thread), WorkerEventStop);
    furi_thread_join(app->worker_thread);
    furi_thread_free(app->worker_thread);

    // Free views
    view_dispatcher_remove_view(app->view_dispatcher, 0);

    with_view_model(
        app->view,
        UartDumpModel * model,
        {
            for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
                furi_string_free(model->list[i]->text);
                free(model->list[i]);
            }
        },
        true);
    view_free(app->view);
    view_dispatcher_free(app->view_dispatcher);

    // Close gui record
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    app->gui = NULL;

    furi_stream_buffer_free(app->rx_stream);

    // Free rest
    free(app);
}

// silences "same-assignment" false positives in the arg parser below
// -V::1048

int32_t uart_echo_app(void* p) {
    uint32_t baudrate = DEFAULT_BAUD_RATE;
    FuriHalSerialDataBits data_bits = DEFAULT_DATA_BITS;
    FuriHalSerialParity parity = DEFAULT_PARITY;
    FuriHalSerialStopBits stop_bits = DEFAULT_STOP_BITS;

    if(p) {
        // parse argument
        char* parse_ptr = p;
        bool parse_success = false;

        do {
            if(strint_to_uint32(parse_ptr, &parse_ptr, &baudrate, 10) != StrintParseNoError) break;

            if(*(parse_ptr++) != '_') break;

            uint16_t data_bits_int;
            if(strint_to_uint16(parse_ptr, &parse_ptr, &data_bits_int, 10) != StrintParseNoError)
                break;
            if(data_bits_int == 6)
                data_bits = FuriHalSerialDataBits6;
            else if(data_bits_int == 7)
                data_bits = FuriHalSerialDataBits7;
            else if(data_bits_int == 8)
                data_bits = FuriHalSerialDataBits8;
            else if(data_bits_int == 9)
                data_bits = FuriHalSerialDataBits9;
            else
                break;

            char parity_char = *(parse_ptr++);
            if(parity_char == 'N')
                parity = FuriHalSerialParityNone;
            else if(parity_char == 'E')
                parity = FuriHalSerialParityEven;
            else if(parity_char == 'O')
                parity = FuriHalSerialParityOdd;
            else
                break;

            uint16_t stop_bits_int;
            if(strint_to_uint16(parse_ptr, &parse_ptr, &stop_bits_int, 10) != StrintParseNoError)
                break;
            if(stop_bits_int == 5)
                stop_bits = FuriHalSerialStopBits0_5;
            else if(stop_bits_int == 1)
                stop_bits = FuriHalSerialStopBits1;
            else if(stop_bits_int == 15)
                stop_bits = FuriHalSerialStopBits1_5;
            else if(stop_bits_int == 2)
                stop_bits = FuriHalSerialStopBits2;
            else
                break;

            parse_success = true;
        } while(0);

        if(!parse_success) {
            FURI_LOG_I(
                TAG,
                "Couldn't parse baud rate and framing (%s). Applying defaults (%d_8N1)",
                (const char*)p,
                DEFAULT_BAUD_RATE);
        }
    }

    UartEchoApp* app = uart_echo_app_alloc(baudrate, data_bits, parity, stop_bits);
    view_dispatcher_run(app->view_dispatcher);
    uart_echo_app_free(app);
    return 0;
}
