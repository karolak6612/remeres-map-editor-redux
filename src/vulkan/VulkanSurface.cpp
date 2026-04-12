#include "VulkanSurface.h"

#ifdef __WXMSW__
#include <wx/msw/private.h>
#include <vulkan/vulkan_win32.h>
#elif defined(__WXGTK__)
#include <wx/gtk/window.h>
#include <gdk/gdkx.h>
#include <vulkan/vulkan_xlib.h>
// Wayland could also be supported here
#elif defined(__WXOSX__)
// macOS MoltenVK surface creation
#endif

VkSurfaceKHR CreateVulkanSurface(VkInstance instance, wxWindow* window) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;

#ifdef __WXMSW__
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = (HWND)window->GetHWND();
    createInfo.hinstance = GetModuleHandle(nullptr);
    vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
#elif defined(__WXGTK__)
    // Assuming X11 for now
    GtkWidget* widget = window->GetHandle();
    GdkWindow* gdkWindow = gtk_widget_get_window(widget);
    Display* display = gdk_x11_display_get_xdisplay(gdk_window_get_display(gdkWindow));
    Window xid = gdk_x11_window_get_xid(gdkWindow);

    VkXlibSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    createInfo.dpy = display;
    createInfo.window = xid;
    vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface);
#elif defined(__WXOSX__)
    // TODO MoltenVK
#endif

    return surface;
}
