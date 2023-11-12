using Editor.DLLWrapper;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows.Interop;

namespace Editor.Utilities
{
    class RenderSurfaceHost : HwndHost
    {
        private readonly int _width = 800;
        private readonly int _height = 600;
        public int SurfaceID { get; private set; } = ID.INVALID_ID;
        private DelayEventTimer _resizeTimer;

        private IntPtr _renderWindowHandle = IntPtr.Zero;

        public void Resize()
        {
            _resizeTimer.Trigger();
        }

        private void Resize(object s, DelayEventTimerArgs e)
        {
            e.RepeatEvent = Mouse.LeftButton == MouseButtonState.Pressed;
            if (!e.RepeatEvent)
            {
                EngineAPI.ResizeRenderSurface(SurfaceID);
            }
        }

        public RenderSurfaceHost(double width, double height)
        {
            _width = (int)width;
            _height = (int)height;
            _resizeTimer = new DelayEventTimer(TimeSpan.FromMilliseconds(250));
            _resizeTimer.Triggered += Resize;
        }

        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
            SurfaceID = EngineAPI.CreateRenderSurface(hwndParent.Handle, _width, _height);
            Debug.Assert(ID.IsValid(SurfaceID));
            _renderWindowHandle = EngineAPI.GetWindowHandle(SurfaceID);
            Debug.Assert(_renderWindowHandle != IntPtr.Zero);

            return new HandleRef(this, _renderWindowHandle);
        }

        protected override void DestroyWindowCore(HandleRef hwnd)
        {
            EngineAPI.RemoveRenderSurface(SurfaceID);
            SurfaceID = ID.INVALID_ID;
            _renderWindowHandle = IntPtr.Zero;
        }
    }
}
