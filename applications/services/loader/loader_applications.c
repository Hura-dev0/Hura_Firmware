#include "loader.h"
#include "loader_applications.h"
#include <dialogs/dialogs.h>
#include <flipper_application/flipper_application.h>
#include <assets_icons.h>
#include <gui/gui.h>
#include <gui/view_holder.h>
#include <gui/modules/loading.h>
#include <dolphin/dolphin.h>
#include <lib/toolbox/path.h>

#define TAG "LoaderApplications"

#define JS_RUNNER_APP "JS Runner"

struct LoaderApplications {
    FuriThread* thread;
    void (*closed_cb)(void*);
    void* context;
};

static int32_t loader_applications_thread(void* p);

LoaderApplications* loader_applications_alloc(void (*closed_cb)(void*), void* context) {
    LoaderApplications* loader_applications = malloc(sizeof(LoaderApplications));
    loader_applications->thread =
        furi_thread_alloc_ex(TAG, 768, loader_applications_thread, (void*)loader_applications);
    loader_applications->closed_cb = closed_cb;
    loader_applications->context = context;
    furi_thread_start(loader_applications->thread);
    return loader_applications;
}

void loader_applications_free(LoaderApplications* loader_applications) {
    furi_assert(loader_applications);
    furi_thread_join(loader_applications->thread);
    furi_thread_free(loader_applications->thread);
    free(loader_applications);
}

typedef struct {
    FuriString* file_path;
    DialogsApp* dialogs;
    Storage* storage;
    Loader* loader;

    Gui* gui;
    ViewHolder* view_holder;
    Loading* loading;
} LoaderApplicationsApp;

static LoaderApplicationsApp* loader_applications_app_alloc(void) {
    LoaderApplicationsApp* app = malloc(sizeof(LoaderApplicationsApp)); //-V799
    app->file_path = furi_string_alloc_set(EXT_PATH("apps"));
    app->dialogs = furi_record_open(RECORD_DIALOGS);
    app->storage = furi_record_open(RECORD_STORAGE);
    app->loader = furi_record_open(RECORD_LOADER);

    app->gui = furi_record_open(RECORD_GUI);
    app->view_holder = view_holder_alloc();
    app->loading = loading_alloc();

    view_holder_attach_to_gui(app->view_holder, app->gui);

    return app;
} //-V773

static void loader_applications_app_free(LoaderApplicationsApp* app) {
    furi_assert(app);

    view_holder_free(app->view_holder);
    loading_free(app->loading);
    furi_record_close(RECORD_GUI);

    furi_record_close(RECORD_LOADER);
    furi_record_close(RECORD_DIALOGS);
    furi_record_close(RECORD_STORAGE);
    furi_string_free(app->file_path);
    free(app);
}

static bool loader_applications_item_callback(
    FuriString* path,
    void* context,
    uint8_t** icon_ptr,
    FuriString* item_name) {
    LoaderApplicationsApp* loader_applications_app = context;
    furi_assert(loader_applications_app);
    if(furi_string_end_with(path, ".fap")) {
        return flipper_application_load_name_and_icon(
            path, loader_applications_app->storage, icon_ptr, item_name);
    } else {
        path_extract_filename(path, item_name, false);
        memcpy(*icon_ptr, icon_get_frame_data(&I_js_script_10px, 0), FAP_MANIFEST_MAX_ICON_SIZE);
        return true;
    }
}

static bool loader_applications_select_app(LoaderApplicationsApp* loader_applications_app) {
    const DialogsFileBrowserOptions browser_options = {
        .extension = ".fap|.js",
        .skip_assets = true,
        .icon = &I_unknown_10px,
        .hide_ext = true,
        .item_loader_callback = loader_applications_item_callback,
        .item_loader_context = loader_applications_app,
        .base_path = EXT_PATH("apps"),
    };

    return dialog_file_browser_show(
        loader_applications_app->dialogs,
        loader_applications_app->file_path,
        loader_applications_app->file_path,
        &browser_options);
}

#define APPLICATION_STOP_EVENT 1

static void loader_pubsub_callback(const void* message, void* context) {
    const LoaderEvent* event = message;
    const FuriThreadId thread_id = (FuriThreadId)context;

    if(event->type == LoaderEventTypeNoMoreAppsInQueue) {
        furi_thread_flags_set(thread_id, APPLICATION_STOP_EVENT);
    }
}

static void
    loader_applications_start_app(LoaderApplicationsApp* app, const char* name, const char* args) {
    dolphin_deed(DolphinDeedPluginStart);

    // load app
    FuriThreadId thread_id = furi_thread_get_current_id();
    FuriPubSubSubscription* subscription =
        furi_pubsub_subscribe(loader_get_pubsub(app->loader), loader_pubsub_callback, thread_id);

    LoaderStatus status = loader_start_with_gui_error(app->loader, name, args);

    if(status == LoaderStatusOk) {
        furi_thread_flags_wait(APPLICATION_STOP_EVENT, FuriFlagWaitAny, FuriWaitForever);
    }

    furi_pubsub_unsubscribe(loader_get_pubsub(app->loader), subscription);
}

static int32_t loader_applications_thread(void* p) {
    LoaderApplications* loader_applications = p;
    LoaderApplicationsApp* app = loader_applications_app_alloc();

    // start loading animation
    view_holder_set_view(app->view_holder, loading_get_view(app->loading));

    while(loader_applications_select_app(app)) {
        if(!furi_string_end_with(app->file_path, ".js")) {
            loader_applications_start_app(app, furi_string_get_cstr(app->file_path), NULL);
        } else {
            loader_applications_start_app(
                app, JS_RUNNER_APP, furi_string_get_cstr(app->file_path));
        }
    }

    // stop loading animation
    view_holder_set_view(app->view_holder, NULL);

    loader_applications_app_free(app);

    if(loader_applications->closed_cb) {
        loader_applications->closed_cb(loader_applications->context);
    }

    return 0;
}
