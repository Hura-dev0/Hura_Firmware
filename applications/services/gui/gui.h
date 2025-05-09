/**
 * @file gui.h
 * GUI: main API
 */

#pragma once

#include "view_port.h"
#include "canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Gui layers */
typedef enum {
    GuiLayerDesktop, /**< Desktop layer for internal use. Like fullscreen but with status bar */

    GuiLayerWindow, /**< Window layer, status bar is shown */

    GuiLayerStatusBarLeft, /**< Status bar left-side layer, auto-layout */
    GuiLayerStatusBarRight, /**< Status bar right-side layer, auto-layout */

    GuiLayerFullscreen, /**< Fullscreen layer, no status bar */

    GuiLayerMAX /**< Don't use or move, special value */
} GuiLayer;

/** Gui Canvas Commit Callback */
typedef void (*GuiCanvasCommitCallback)(
    uint8_t* data,
    size_t size,
    CanvasOrientation orientation,
    void* context);

#define RECORD_GUI "gui"

typedef struct Gui Gui;

/** Add view_port to view_port tree
 *
 * @remark     thread safe
 *
 * @param      gui        Gui instance
 * @param      view_port  ViewPort instance
 * @param[in]  layer      GuiLayer where to place view_port
 */
void gui_add_view_port(Gui* gui, ViewPort* view_port, GuiLayer layer);

/** Remove view_port from rendering tree
 *
 * @remark     thread safe
 *
 * @param      gui        Gui instance
 * @param      view_port  ViewPort instance
 */
void gui_remove_view_port(Gui* gui, ViewPort* view_port);

/** Send ViewPort to the front
 *
 * Places selected ViewPort to the top of the drawing stack
 *
 * @param      gui        Gui instance
 * @param      view_port  ViewPort instance
 */
void gui_view_port_send_to_front(Gui* gui, ViewPort* view_port);

/** Send ViewPort to the back
 *
 * Places selected ViewPort to the bottom of the drawing stack
 *
 * @param      gui        Gui instance
 * @param      view_port  ViewPort instance
 */
void gui_view_port_send_to_back(Gui* gui, ViewPort* view_port);

/** Add gui canvas commit callback
 *
 * This callback will be called upon Canvas commit Callback dispatched from GUI
 * thread and is time critical
 *
 * @param      gui       Gui instance
 * @param      callback  GuiCanvasCommitCallback
 * @param      context   GuiCanvasCommitCallback context
 */
void gui_add_framebuffer_callback(Gui* gui, GuiCanvasCommitCallback callback, void* context);

/** Remove gui canvas commit callback
 *
 * @param      gui       Gui instance
 * @param      callback  GuiCanvasCommitCallback
 * @param      context   GuiCanvasCommitCallback context
 */
void gui_remove_framebuffer_callback(Gui* gui, GuiCanvasCommitCallback callback, void* context);

/** Get gui canvas frame buffer size
 * *
 * @param      gui       Gui instance
 * @return     size_t    size of frame buffer in bytes
 */
size_t gui_get_framebuffer_size(const Gui* gui);

/** Set lockdown mode
 *
 * When lockdown mode is enabled, only GuiLayerDesktop is shown.
 * This feature prevents services from showing sensitive information when flipper is locked.
 *
 * @param      gui       Gui instance
 * @param      lockdown  bool, true if enabled
 */
void gui_set_lockdown(Gui* gui, bool lockdown);

/** Inhibit lockdown mode
 * 
 * Lockdown mode can be inhibited by calling this function with inhibit set to true.
 * This is used to show information even when flipper is locked.
 * 
 * @param      gui       Gui instance
 * @param      inhibit   true to inhibit lockdown mode
 */
void gui_set_lockdown_inhibit(Gui* gui, bool inhibit);

/** Check if Gui is in lockdown mode
 * 
 * @param      gui       Gui instance
 * @return     bool      true if Gui is in lockdown mode
 */
bool gui_is_lockdown(const Gui* gui);

/** Acquire Direct Draw lock and get Canvas instance
 *
 * This method return Canvas instance for use in monopoly mode. Direct draw lock
 * disables input and draw call dispatch functions in GUI service. No other
 * applications or services will be able to draw until gui_direct_draw_release
 * call.
 *
 * @param      gui   The graphical user interface
 *
 * @return     Canvas instance
 */
Canvas* gui_direct_draw_acquire(Gui* gui);

/** Release Direct Draw Lock
 *
 * Release Direct Draw Lock, enables Input and Draw call processing. Canvas
 * acquired in gui_direct_draw_acquire will become invalid after this call.
 *
 * @param      gui   Gui instance
 */
void gui_direct_draw_release(Gui* gui);

#ifdef __cplusplus
}
#endif
