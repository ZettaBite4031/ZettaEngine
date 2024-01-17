using Editor.Components;
using Editor.DLLWrapper;
using Editor.GameDev;
using Editor.Utilities;
using EnvDTE;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;

namespace Editor.GameProject
{


    [DataContract(Name = "Game")]
    class Project : ViewModelBase
    {
        public static string Extension => ".zetta";

        [DataMember]
        public string Name { get; private set; } = "New Project";
        [DataMember]
        public string Path { get; private set; }
        public string FullPath => $@"{Path}{Name}\{Name}{Extension}";
        public string Solution => $@"{Path}{Name}\{Name}.sln";
        public string ContentPath => $@"{Path}{Name}\Content\";
        public string TempFolder => $@"{Path}{Name}\.zetta\Temp\";

        private int _BuildConfig;
        [DataMember]
        public int BuildConfig
        {
            get => _BuildConfig;
            set
            {
                if (_BuildConfig != value)
                {
                    _BuildConfig = value;
                    OnPropertyChanged(nameof(BuildConfig));
                }
            }
        }

        public BuildConfiguration StandAloneBuildConfiguration => BuildConfig == 0 ? BuildConfiguration.Debug : BuildConfiguration.Release;
        public BuildConfiguration DLLBuildConfiguration => BuildConfig == 0 ? BuildConfiguration.DebugEditor : BuildConfiguration.ReleaseEditor;

        private string[] _AvailableScripts;
        public string[] AvailableScripts
        {
            get => _AvailableScripts;
            set
            {
                if (_AvailableScripts != value)
                {
                    _AvailableScripts = value;
                    OnPropertyChanged(nameof(AvailableScripts));
                }
            }
        }

        [DataMember(Name = nameof(Scenes))]
        private ObservableCollection<Scene> _scenes = new();
        public ReadOnlyObservableCollection<Scene> Scenes { get; private set; }

        private Scene _ActiveScene;
        public Scene ActiveScene
        {
            get => _ActiveScene;
            set
            {
                if (_ActiveScene != value)
                {
                    _ActiveScene = value;
                    OnPropertyChanged(nameof(ActiveScene));
                }
            }
        }
        public static Project Current => Application.Current.MainWindow?.DataContext as Project;

        public static UndoRedo UndoRedo { get; } = new UndoRedo();

        public ICommand AddSceneCommand { get; private set; }
        public ICommand RemoveSceneCommand { get; private set; }
        public ICommand UndoCommand { get; private set; }
        public ICommand RedoCommand { get; private set; }
        public ICommand SaveCommand { get; private set; }
        public ICommand DebugStartCommand { get; private set; }
        public ICommand DebugStartWithoutDebuggingCommand { get; private set; }
        public ICommand DebugStopCommand { get; private set; }
        public ICommand BuildCommand { get; private set; }

        private void SetCommands()
        {
            AddSceneCommand = new CommandRelay<object>(x =>
            {
                AddScene($"New Scene {_scenes.Count}");
                var newScene = _scenes.Last();
                var sceneIdx = _scenes.Count - 1;
                UndoRedo.Add(new UndoRedoAction(
                    () => RemoveScene(newScene),
                    () => _scenes.Insert(sceneIdx, newScene),
                    $"Add {newScene.Name}"));
            });
            RemoveSceneCommand = new CommandRelay<Scene>(x =>
            {
                var sceneIdx = _scenes.IndexOf(x);
                RemoveScene(x);

                UndoRedo.Add(new UndoRedoAction(
                    () => _scenes.Insert(sceneIdx, x),
                    () => RemoveScene(x),
                    $"Remove {x.Name}"));
            }, x => !x.IsActive);
            UndoCommand = new CommandRelay<object>(x => UndoRedo.Undo(), x => UndoRedo.UndoList.Any());
            RedoCommand = new CommandRelay<object>(x => UndoRedo.Redo(), x => UndoRedo.UndoList.Any());
            SaveCommand = new CommandRelay<object>(x => Save(this));
            DebugStartCommand = new CommandRelay<object>(async x=> await RunGame(true), x => !VisualStudio.IsDebugging() && VisualStudio.BuildDone);
            DebugStartWithoutDebuggingCommand = new CommandRelay<object>(async x=> await RunGame(false), x => !VisualStudio.IsDebugging() && VisualStudio.BuildDone);
            DebugStopCommand = new CommandRelay<object>(async x => await StopGame(), x => VisualStudio.IsDebugging());
            BuildCommand = new CommandRelay<bool>(async x => await BuildGameDLL(x), x => !VisualStudio.IsDebugging() && VisualStudio.BuildDone);

            OnPropertyChanged(nameof(AddSceneCommand));
            OnPropertyChanged(nameof(RemoveSceneCommand));
            OnPropertyChanged(nameof(UndoCommand));
            OnPropertyChanged(nameof(RedoCommand));
            OnPropertyChanged(nameof(SaveCommand));
            OnPropertyChanged(nameof(DebugStartCommand));
            OnPropertyChanged(nameof(DebugStartWithoutDebuggingCommand));
            OnPropertyChanged(nameof(DebugStopCommand));
            OnPropertyChanged(nameof(BuildCommand));
        }


        private void AddScene(string sceneName)
        {
            Debug.Assert(!string.IsNullOrEmpty(sceneName.Trim()));
            _scenes.Add(new Scene(this, sceneName));
        }

        private void RemoveScene(Scene scene)
        {
            Debug.Assert(_scenes.Contains(scene));
            _scenes.Remove(scene);
        }

        public static Project Load(string file)
        {
            Debug.Assert(File.Exists(file));
            return Serializer.FromFile<Project>(file);
        }

        public void Unload()
        {
            UnloadGameDLL();
            VisualStudio.CloseVisualStudio();
            UndoRedo.Reset();
            Logger.Clear();
            DeleteTempFolder();
        }

        private void DeleteTempFolder()
        {
            if(Directory.Exists(TempFolder)) Directory.Delete(TempFolder, true);
        }

        public static void Save(Project project)
        {
            Serializer.ToFile(project, project.FullPath);
            Logger.Log(MessageType.Info, $"Project saved to {project.FullPath}");
        }

        private void SaveToBinary()
        {
            var configName = VisualStudio.GetConfigurationName(StandAloneBuildConfiguration);
            var bin = $@"{Path}{Name}\x64\{configName}\game.bin";

            using (var bw = new BinaryWriter(File.Open(bin, FileMode.Create, FileAccess.Write)))
            {
                bw.Write(ActiveScene.GameEntities.Count);
                foreach (var entity in ActiveScene.GameEntities)
                {
                    bw.Write(0); // entity type (reserved for later)
                    bw.Write(entity.Components.Count);
                    foreach(var component in entity.Components)
                    {
                        bw.Write((int)component.ToEnumType());
                        component.WriteToBinary(bw);
                    }
                }
            }
        }

        private async Task RunGame(bool debug)
        {
            await Task.Run(async () => {
                VisualStudio.BuildSolution(this, StandAloneBuildConfiguration, debug);
                for (int i = 0; i < 3; i++)
                {
                    System.Threading.Thread.Sleep(1000);
                    if (VisualStudio.BuildSuccess)
                    {
                        SaveToBinary();
                        await Task.Run(()=>VisualStudio.Run(this, StandAloneBuildConfiguration, debug));
                        break;
                    }
                }
            });
        }

        private async Task StopGame() => await Task.Run(() => VisualStudio.Stop());

        private async Task BuildGameDLL(bool showWindow = true)
        {
            try
            {
                UnloadGameDLL();
                await Task.Run(() => VisualStudio.BuildSolution(this, DLLBuildConfiguration, showWindow));
                if (VisualStudio.BuildSuccess) LoadGameDLL();
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
                throw;
            }
        }

        private void LoadGameDLL()
        {
            var configName = VisualStudio.GetConfigurationName(DLLBuildConfiguration);
            var dll = $@"{Path}{Name}\x64\{configName}\{Name}.dll";
            AvailableScripts = null;
            if (File.Exists(dll) && EngineAPI.LoadGameDLL(dll) != 0)
            {
                AvailableScripts = EngineAPI.GetScriptNames();
                Logger.Log(MessageType.Info, "Game DLL loaded successfully");
            }
            else Logger.Log(MessageType.Warn, "Game DLL failed to load");
        }

        private void UnloadGameDLL()
        {
            ActiveScene.GameEntities.Where(x => x.GetComponent<Script>() != null).ToList().ForEach(x => x.IsActive = false);
            if (EngineAPI.UnloadGameDLL() != 0)
            {
                Logger.Log(MessageType.Info, "Game DLL unloaded");
                AvailableScripts = null;
            }
        }

        [OnDeserialized]
        private async void OnDeserialized(StreamingContext context)
        {
            if (_scenes != null)
            {
                Scenes = new ReadOnlyObservableCollection<Scene>(_scenes);
                OnPropertyChanged(nameof(Scenes));
            }
            ActiveScene = _scenes.FirstOrDefault(x => x.IsActive);
            Debug.Assert(ActiveScene != null);

            await BuildGameDLL(false);

            SetCommands();   
        }

        public Project(string name, string path)
        {
            Name = name;
            Path = path;

            Debug.Assert(File.Exists((Path + Name + Extension).ToLower()));
            OnDeserialized(new StreamingContext());
        }
    }
}
