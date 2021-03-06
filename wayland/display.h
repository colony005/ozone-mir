// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OZONE_WAYLAND_DISPLAY_H_
#define OZONE_WAYLAND_DISPLAY_H_

#if !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#endif

#include <wayland-client.h>
#include <list>
#include <map>

#include "base/basictypes.h"
#include "ozone/ui/events/window_state_change_handler.h"
#include "ozone/ui/egl/display.h"

namespace ozonewayland {

class WaylandDisplayPollThread;
class WaylandInputDevice;
class WaylandScreen;
class WaylandWindow;

// WaylandDisplay is a wrapper around wl_display. Once we get a valid
// wl_display, the Wayland server will send different events to register
// the Wayland compositor, shell, screens, input devices, ...
class WaylandDisplay : public ozoneui::Display, public ozoneui::WindowStateChangeHandler {
 public:
  enum RegistrationType {
    RegisterAsNeeded,  // Handles all the required registrations.
    RegisterOutputOnly  // Only screen registration.
  };

  explicit WaylandDisplay(RegistrationType type);
  virtual ~WaylandDisplay();
  
  bool Initialized() const { return display_ != 0; }
  intptr_t GetNativeDisplay() { return reinterpret_cast<intptr_t>(display_); }
  
  const int32* GetEGLSurfaceProperties(const int32* desired_list);

  // Ownership is not passed to the caller.
  static WaylandDisplay* GetInstance() { return instance_; }
  // Returns a pointer to wl_display.
  wl_display* display() const { return display_; }

  wl_registry* registry() const { return registry_; }

  WaylandInputDevice* PrimaryInput() const { return primary_input_; }

  // Returns a list of the registered screens.
  const std::list<WaylandScreen*>& GetScreenList() const;
  WaylandScreen* PrimaryScreen() const { return primary_screen_ ; }

  wl_shell* shell() const { return shell_; }

  wl_shm* shm() const { return shm_; }
  wl_compositor* GetCompositor() const { return compositor_; }
  int GetDisplayFd() const { return wl_display_get_fd(display_); }
  unsigned GetSerial() const { return serial_; }
  void SetSerial(unsigned serial) { serial_ = serial; }

  const ozoneui::WindowMap& GetWindowList() const { return widget_map_; }

  // Creates a WaylandWindow backed by EGL Window and maps it to w. This can be
  // useful for callers to track a particular surface. By default the type of
  // surface(i.e. toplevel, menu) is none. One needs to explicitly call
  // WaylandWindow::SetShellAttributes to set this. The ownership of
  // WaylandWindow is not passed to the caller.
  WaylandWindow* CreateAcceleratedSurface(unsigned w);

  // Destroys WaylandWindow whose handle is w.
  void DestroyWindow(unsigned w);

  // Starts polling on display fd. This should be used when one needs to
  // continuously read pending events coming from Wayland compositor and
  // dispatch them. The polling is done completely on a separate thread and
  // doesn't block the thread from which this is called.
  void StartProcessingEvents();
  // Stops polling on display fd.
  void StopProcessingEvents();
  // Flush Display.
  void FlushDisplay();
  // Does a round trip to Wayland server. This call blocks the current thread
  // until all pending request are processed by the server.
  void SyncDisplay();
  // WindowStateChangeHandler implementation:
  virtual void SetWidgetState(unsigned widget,
                              ozoneui::WidgetState state,
                              unsigned width = 0,
                              unsigned height = 0) OVERRIDE;
  virtual void SetWidgetTitle(unsigned w,
                              const base::string16& title) OVERRIDE;
  virtual void SetWidgetAttributes(unsigned widget,
                                   unsigned parent,
                                   unsigned x,
                                   unsigned y,
                                   ozoneui::WidgetType type) OVERRIDE;

 private:
  void terminate();
  WaylandWindow* GetWidget(unsigned w);
  // This handler resolves all server events used in initialization. It also
  // handles input device registration, screen registration.
  static void DisplayHandleGlobal(
      void *data,
      struct wl_registry *registry,
      uint32_t name,
      const char *interface,
      uint32_t version);
  // This handler resolves only screen registration. In general you don't want
  // to use this but the one below.
  static void DisplayHandleOutputOnly(
      void *data,
      struct wl_registry *registry,
      uint32_t name,
      const char *interface,
      uint32_t version);

  // WaylandDisplay manages the memory of all these pointers.
  wl_display* display_;
  wl_registry* registry_;
  wl_compositor* compositor_;
  wl_shell* shell_;
  wl_shm* shm_;
  WaylandScreen* primary_screen_;
  WaylandInputDevice* primary_input_;
  WaylandDisplayPollThread* display_poll_thread_;

  std::list<WaylandScreen*> screen_list_;
  std::list<WaylandInputDevice*> input_list_;
  ozoneui::WindowMap widget_map_;
  unsigned serial_;
  static WaylandDisplay* instance_;
  DISALLOW_COPY_AND_ASSIGN(WaylandDisplay);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_DISPLAY_H_
