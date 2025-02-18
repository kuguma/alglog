using UnityEngine;
using System.Runtime.InteropServices;

public static class AlgLogBridge
{
    private const string PluginName = "AlgLogUnity";

    [DllImport(PluginName)]
    private static extern System.IntPtr CreateLogger(bool async_mode);

    [DllImport(PluginName)]
    private static extern void DestroyLogger(System.IntPtr logger);

    [DllImport(PluginName)]
    private static extern void LogMessage(System.IntPtr logger, int level, string message);

    [DllImport(PluginName)]
    private static extern void FlushLogger(System.IntPtr logger);

    public class Logger
    {
        private System.IntPtr _nativeLogger;

        public Logger(bool async_mode = false)
        {
            _nativeLogger = CreateLogger(async_mode);
        }

        ~Logger()
        {
            if (_nativeLogger != System.IntPtr.Zero)
            {
                DestroyLogger(_nativeLogger);
                _nativeLogger = System.IntPtr.Zero;
            }
        }

        public void Error(string message) => LogMessage(_nativeLogger, 0, message);
        public void Alert(string message) => LogMessage(_nativeLogger, 1, message);
        public void Info(string message) => LogMessage(_nativeLogger, 2, message);
        public void Critical(string message) => LogMessage(_nativeLogger, 3, message);
        public void Warn(string message) => LogMessage(_nativeLogger, 4, message);
        public void Debug(string message) => LogMessage(_nativeLogger, 5, message);
        public void Trace(string message) => LogMessage(_nativeLogger, 6, message);

        public void Flush() => FlushLogger(_nativeLogger);
    }
}
