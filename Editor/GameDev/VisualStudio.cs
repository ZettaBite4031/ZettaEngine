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
using System.Threading.Tasks;

namespace Editor.GameDev
{
    static class VisualStudio
    {
        private static EnvDTE80.DTE2 _vsInstance = null;
        private static readonly string _progID = "VisualStudio.DTE.17.0";

        public static bool BuildSuccess { get; private set; } = true;
        public static bool BuildDone { get; private set; } = true;

        [DllImport("ole32.dll")]
        private static extern int GetRunningObjectTable(uint reserved, out IRunningObjectTable pprot);
        [DllImport("ole32.dll")]
        private static extern int CreateBindCtx(uint reserved, out IBindCtx pctx);

        public static void OpenVisualStudio(string sln)
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
                            var slnName = dte.Solution.FullName;
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
            } catch (Exception ex)
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

        public static void CloseVisualStudio()
        {
            if (_vsInstance?.Solution.IsOpen == true)
            {
                _vsInstance.ExecuteCommand("File.SaveAll");
                _vsInstance.Solution.Close(true);
            }
            _vsInstance?.Quit();
        }

        internal static bool AddFilesToSolution(string sln, string projectName, string[] files)
        {
            Debug.Assert(files?.Length > 0);
            try
            {
                OpenVisualStudio(sln);
                if (_vsInstance != null)
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

        public static bool IsDebugging()
        {
            bool res = false;
            for (int i =0; i < 3; i++)
            {
                try
                {
                    res = _vsInstance != null &&
                        (_vsInstance.Debugger.CurrentProgram != null || _vsInstance.Debugger.CurrentMode == EnvDTE.dbgDebugMode.dbgRunMode);
                    break;
                }
                catch (Exception e)
                {
                    Debug.WriteLine(e.Message);
                    if (!res) System.Threading.Thread.Sleep(1000);
                }
            }
            return res;
        }

        internal static void BuildSolution(GameProject.Project project, string buildConfig, bool showWindow = true)
        {
            if (IsDebugging())
            {
                Logger.Log(MessageType.Error, "Visual Studio Is debugging a process");
                return;
            }

            OpenVisualStudio(project.Solution);
            BuildSuccess = BuildDone = false;
            for (int i = 0; i < 3 && !BuildDone; i++)
            {
                try
                {
                    if (!_vsInstance.Solution.IsOpen) _vsInstance.Solution.Open(project.Solution);
                    _vsInstance.MainWindow.Visible = showWindow;

                    _vsInstance.Events.BuildEvents.OnBuildProjConfigBegin += OnBuildSolutionBegin;
                    _vsInstance.Events.BuildEvents.OnBuildProjConfigDone += OnBuildSolutionDone;

                    try
                    {
                        foreach (var pdb in Directory.GetFiles(Path.Combine($"{project.Path}", $@"x64\{buildConfig}"), "*.pdb"))
                            File.Delete(pdb);
                    }
                    catch (Exception ex) { Debug.WriteLine(ex.Message); }

                    _vsInstance.Solution.SolutionBuild.SolutionConfigurations.Item(buildConfig).Activate();
                    _vsInstance.ExecuteCommand("Build.BuildSolution");
                    break;
                }
                catch (Exception e)
                {
                    Debug.WriteLine(e.Message);
                    Debug.WriteLine($"Failed to build Visual Studio Solution '{project.Solution}'");
                    System.Threading.Thread.Sleep(1000);
                }
            }
        }

        private static void OnBuildSolutionBegin(string project, string projectConfig, string platform, string solutionConfig)
        {
            Logger.Log(MessageType.Info, $"Building {project}, {projectConfig}|{platform}");
        }

        private static void OnBuildSolutionDone(string project, string projectConfig, string platform, string solutionConfig, bool success)
        {
            if (BuildDone) return;

            if (success) Logger.Log(MessageType.Info, $"Successfully built {project} {projectConfig}|{platform}");
            else Logger.Log(MessageType.Error, $"Failed to build {project} {projectConfig}|{platform}");
            BuildDone = true;
            BuildSuccess = success;
        }

        public static void Run(GameProject.Project project, string config, bool debug)
        {
            if (_vsInstance != null && !IsDebugging() && BuildDone && BuildSuccess)
                _vsInstance.ExecuteCommand(debug ? "Debug.Start" : "Debug.StartWithoutDebugging");
        }

        public static void Stop()
        {
            if (_vsInstance != null && IsDebugging())
                _vsInstance.ExecuteCommand("Debug.StopDebugging");
        }
    }
}
