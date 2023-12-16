using Editor.GameProject;
using Editor.Utilities;
using EnvDTE;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Text;
using System.Threading;

namespace Editor.GameDev
{
    enum BuildConfiguration
    {
        Debug,
        DebugEditor,
        Release,
        ReleaseEditor,
    }

    static class VisualStudio
    {
        private static readonly ManualResetEventSlim _resetEvent = new ManualResetEventSlim(false);
        private static readonly string _progID = "VisualStudio.DTE.17.0";
        private static readonly object _lock = new object();
        private static readonly string[] _buildConfigurationNames = new string[] { "Debug", "DebugEditor", "Release", "ReleaseEditor" };
        private static EnvDTE80.DTE2 _vsInstance = null;

        public static bool BuildSuccess { get; private set; } = true;
        public static bool BuildDone { get; private set; } = true;

        public static string GetConfigurationName(BuildConfiguration config) => _buildConfigurationNames[(int)config];

        [DllImport("ole32.dll")]
        private static extern int GetRunningObjectTable(uint reserved, out IRunningObjectTable pprot);
        [DllImport("ole32.dll")]
        private static extern int CreateBindCtx(uint reserved, out IBindCtx pctx);

        private static void CallOnSTAThread(Action action)
        {
            var thread = new System.Threading.Thread(() =>
            {
                MessageFilter.Register();
                try { action(); }
                catch (Exception ex) { Logger.Log(MessageType.Warn, ex.Message); }
                finally { MessageFilter.Revoke(); }
            });

            thread.SetApartmentState(ApartmentState.STA);
            thread.Start();
            thread.Join();
        }

        private static void OpenVisualStudio_Internal(string sln)
        {
            IRunningObjectTable rot = null;
            IEnumMoniker monikerTable = null;
            IBindCtx bindCtx = null;
            try
            {
                if (_vsInstance == null)
                {
                    // Find and grab VS2022
                    var res = GetRunningObjectTable(0, out rot);
                    if (res < 0 || rot == null) throw new COMException($"GetRunningObjectTable() returned HRESULT: {res:X8}");

                    rot.EnumRunning(out monikerTable);
                    monikerTable.Reset();

                    res = CreateBindCtx(0, out bindCtx);
                    if (res < 0 || bindCtx == null) throw new COMException($"CreateBindCtx() returned HRESULT: {res:X8}");

                    IMoniker[] currentMoniker = new IMoniker[1];
                    while (monikerTable.Next(1, currentMoniker, IntPtr.Zero) == 0)
                    {
                        string name = string.Empty;
                        currentMoniker[0]?.GetDisplayName(bindCtx, null, out name);
                        if (name.Contains(_progID))
                        {
                            res = rot.GetObject(currentMoniker[0], out object obj);
                            if (res < 0 || obj == null) throw new COMException($"RunningObjectTable.GetObject() returned HRESULT: {res:X8}");
                            EnvDTE80.DTE2 dte = obj as EnvDTE80.DTE2;

                            var slnName = string.Empty;
                            CallOnSTAThread(() =>
                            {
                                var slnName = dte.Solution.FullName;
                            });
                            if (slnName == sln)
                            {
                                _vsInstance = dte;
                                break;
                            }
                        }
                    }

                    if (_vsInstance == null) // VS2022 is not open
                    {
                        // Open VS2022
                        Type visualStudioType = Type.GetTypeFromProgID(_progID, true);
                        _vsInstance = Activator.CreateInstance(visualStudioType) as EnvDTE80.DTE2;
                    }
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, "Failed to open Visual Studio");
            }
            finally
            {
                if (bindCtx != null) Marshal.ReleaseComObject(bindCtx);
                if (monikerTable != null) Marshal.ReleaseComObject(monikerTable);
                if (rot != null) Marshal.ReleaseComObject(rot);
            }
        }

        public static void OpenVisualStudio(string sln)
        {
            lock (_lock) { OpenVisualStudio(sln); }
        }

        private static void CloseVisualStudio_Internal()
        {
            CallOnSTAThread(() =>
            {
                if (_vsInstance?.Solution.IsOpen == true)
                {
                    _vsInstance.ExecuteCommand("File.SaveAll");
                    _vsInstance.Solution.Close(true);
                }
                _vsInstance?.Quit();
                _vsInstance = null;
            });
        }

        public static void CloseVisualStudio()
        {
            lock (_lock) { CloseVisualStudio_Internal(); }
        }

        private static bool AddFilesToSolution_Internal(string sln, string projectName, string[] files)
        {
            Debug.Assert(files?.Length > 0);
            try
            {
                OpenVisualStudio_Internal(sln);
                if (_vsInstance != null)
                {
                    CallOnSTAThread(() =>
                    {
                        if (!_vsInstance.Solution.IsOpen) _vsInstance.Solution.Open(sln);
                        else _vsInstance.ExecuteCommand("File.SaveAll");

                        foreach (EnvDTE.Project project in _vsInstance.Solution.Projects)
                            if (project.UniqueName.Contains(projectName))
                                foreach (var file in files) project.ProjectItems.AddFromFile(file);

                        var source = files.FirstOrDefault(x => Path.GetExtension(x) == ".cpp");
                        if (!string.IsNullOrEmpty(source))
                            _vsInstance.ItemOperations.OpenFile(source, EnvDTE.Constants.vsViewKindTextView).Visible = true;
                        _vsInstance.MainWindow.Activate();
                        _vsInstance.MainWindow.Visible = true;
                    });
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Debug.WriteLine("Failed to add files to Visual Studio project");
                return false;
            }
            return true;
        }

        public static bool AddFilesToSolution(string sln, string projectName, string[] files)
        {
            lock (_lock) { return AddFilesToSolution_Internal(sln, projectName, files); }
        }

        private static bool IsDebugging_Internal()
        {
            bool res = false;
            CallOnSTAThread(() =>
            {
                res = _vsInstance != null &&
                    (_vsInstance.Debugger.CurrentProgram != null || _vsInstance.Debugger.CurrentMode == EnvDTE.dbgDebugMode.dbgRunMode);
            });
                
            return res;
        }

        public static bool IsDebugging()
        {
            lock (_lock) { return IsDebugging_Internal(); }
        }

        private static void BuildSolution_Internal(GameProject.Project project, BuildConfiguration buildConfig, bool showWindow = true)
        {
            if (IsDebugging_Internal())
            {
                Logger.Log(MessageType.Error, "Visual Studio Is debugging a process");
                return;
            }

            OpenVisualStudio_Internal(project.Solution);
            BuildSuccess = BuildDone = false;
            CallOnSTAThread(() =>
            {
                _vsInstance.MainWindow.Visible = showWindow;

                if (!_vsInstance.Solution.IsOpen) 
                    _vsInstance.Solution.Open(project.Solution);

                _vsInstance.Events.BuildEvents.OnBuildProjConfigBegin += OnBuildSolutionBegin;
                _vsInstance.Events.BuildEvents.OnBuildProjConfigDone += OnBuildSolutionDone;
            });

            var configName = GetConfigurationName(buildConfig);

            try
            {
                foreach (var pdb in Directory.GetFiles(Path.Combine($@"{project.Path}{project.Name}", $@"x64\{configName}"), "*.pdb"))
                    File.Delete(pdb);
            }
            catch (Exception ex) { Debug.WriteLine(ex.Message); }

            CallOnSTAThread(() =>
            {
                _vsInstance.Solution.SolutionBuild.SolutionConfigurations.Item(configName).Activate();
                _vsInstance.ExecuteCommand("Build.BuildSolution");
                _resetEvent.Wait();
                _resetEvent.Reset();
            });
        }
       
        public static void BuildSolution(GameProject.Project project, BuildConfiguration buildConfig, bool showWindow = true)
        {
            lock (_lock) { BuildSolution_Internal(project, buildConfig, showWindow); }
        }

        private static void Run_Internal(GameProject.Project project, BuildConfiguration buildConfig, bool debug)
        {
            CallOnSTAThread(() =>
            {
                if (_vsInstance != null && !IsDebugging_Internal() && BuildSuccess)
                    _vsInstance.ExecuteCommand(debug ? "Debug.Start" : "Debug.StartWithoutDebugging");
            });
        }

        public static void Run(GameProject.Project project, BuildConfiguration buildConfig, bool debug)
        {
            lock (_lock) { Run_Internal(project, buildConfig, debug); }
        }

        private static void Stop_Internal()
        {
            CallOnSTAThread(() => 
            {
                if (_vsInstance != null && IsDebugging_Internal())
                    _vsInstance.ExecuteCommand("Debug.StopDebugging");
            });
        }

        public static void Stop()
        {
            lock (_lock) { Stop_Internal(); }  
        }

        private static void OnBuildSolutionBegin(string project, string projectConfig, string platform, string solutionConfig)
        {
            if (BuildDone) return;
            Logger.Log(MessageType.Info, $"Building {project}, {projectConfig}|{platform}");
        }

        private static void OnBuildSolutionDone(string project, string projectConfig, string platform, string solutionConfig, bool success)
        {
            if (BuildDone) return;

            if (success) Logger.Log(MessageType.Info, $"Successfully built {project} {projectConfig}|{platform}");
            else Logger.Log(MessageType.Error, $"Failed to build {project} {projectConfig}|{platform}");
            BuildDone = true;
            BuildSuccess = success;
            _resetEvent.Set();
        }

    }

    public class MessageFilter : IOleMessageFilter
    {
        private static int SERVERCALL_ISHANDLED = 0;
        private static int PENDINGMSG_WAITDEFPROCESS = 2;
        private static int SERVERCALL_RETRYLATER = 2;

        [DllImport("ole32.dll")]
        private static extern int CoRegisterMessageFilter(IOleMessageFilter newFilter, out IOleMessageFilter oldFilter);

        public static void Register()
        {
            IOleMessageFilter newFilter = new MessageFilter();
            int hr = CoRegisterMessageFilter(newFilter, out var oldFilter);
            Debug.Assert(hr >= 0, "Registering COM IMessageFilter failed.");
        }

        public static void Revoke()
        {
            int hr = CoRegisterMessageFilter(null, out var oldFilter);
            Debug.Assert(hr >= 0, "Unregistering COM IMessageFilter failed.");
        }

        int IOleMessageFilter.HandleInComingCall(int dwCallType, System.IntPtr hTaskCaller, int dwTickCount, System.IntPtr lpInterfaceInfo)
        {
            return SERVERCALL_ISHANDLED;
        }

        int IOleMessageFilter.RetryRejectedCall(System.IntPtr hTaskCallee, int dwTickCount, int dwRejectType)
        {
            if (dwRejectType == SERVERCALL_RETRYLATER)
            {
                Debug.WriteLine("COM Server busy. Retrying call...");
                return 500;
            }
            return -1;
        }

        int IOleMessageFilter.MessagePending(System.IntPtr hTaskCallee, int dwTickCount, int dwPendingType)
        {
            return PENDINGMSG_WAITDEFPROCESS;
        }
    }

    [ComImport(), Guid("00000016-0000-0000-C000-000000000046"), 
        InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IOleMessageFilter
    {
        [PreserveSig]
        int HandleInComingCall(int dwCallType, IntPtr hTaskCaller, int dwTickCount, IntPtr lpInterfaceInfo);

        [PreserveSig]
        int RetryRejectedCall(IntPtr hTaskCallee, int dwTickCount, int dwRejectType);

        [PreserveSig]
        int MessagePending(IntPtr hTaskCallee, int dwTickCount, int dwPendingType);
    }
}
