using Editor.GameProject;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace Editor.Content
{
    abstract class AssetProxy : ViewModelBase
    {
        public FileInfo FileInfo { get; }

        private string _dstFolder;
        public string DstFolder
        {
            get => _dstFolder;
            set
            {
                if ((!Path.EndsInDirectorySeparator(value)))
                    value += Path.DirectorySeparatorChar;

                if (_dstFolder != value)
                {
                    _dstFolder = value;
                    OnPropertyChanged(nameof(DstFolder));
                }
            }
        }

        public abstract IAssetImportSettings ImportSettings { get; }
        public abstract void CopySettings(IAssetImportSettings settings);

        public AssetProxy(string file, string dst)
        {
            Debug.Assert(File.Exists(file));
            FileInfo = new FileInfo(file);
            DstFolder = dst;
        }
    }

    class GeometryProxy : AssetProxy
    {
        public override GeometryImportSettings ImportSettings { get; } = new();

        public override void CopySettings(IAssetImportSettings settings)
        {
            Debug.Assert(settings is GeometryImportSettings);
            if (settings is GeometryImportSettings geometryImportSettings)
            {
                IAssetImportSettings.CopyImportSettings(geometryImportSettings, ImportSettings);
            }
        }

        public GeometryProxy(string file, string dst) 
            : base(file, dst)
        {
        }
    }

    class TextureProxy : AssetProxy
    {
        public override TextureImportSettings ImportSettings { get; } = new();

        public override void CopySettings(IAssetImportSettings settings)
        {
            Debug.Assert(settings is TextureImportSettings);
            if (settings is TextureImportSettings textureImportSettings)
            {
                IAssetImportSettings.CopyImportSettings(textureImportSettings, ImportSettings);
            }
        }

        public TextureProxy(string file, string dst)
            : base(file, dst)
        {
        }
    }

    class AudioProxy : AssetProxy
    {
        public override IAssetImportSettings ImportSettings => throw new NotImplementedException();

        public override void CopySettings(IAssetImportSettings settings)
        {
            throw new NotImplementedException();
        }

        public AudioProxy(string file, string dst)
            : base(file, dst)
        {
        }
    }

    interface IImportSettingsConfigurator<T> where T : AssetProxy
    {
        void AddFiles(IEnumerable<string> files, string dst);
        void RemoveFile(T proxy);
        void Import();
    }

    class GeometryImportSettingsConfigurator : ViewModelBase, IImportSettingsConfigurator<GeometryProxy>
    {
        private readonly ObservableCollection<GeometryProxy> _geometryProxies = new();
        public ReadOnlyObservableCollection<GeometryProxy> GeometryProxies { get; }

        public void AddFiles(IEnumerable<string> files, string dst)
        {
            files.Except(_geometryProxies.Select(proxy => proxy.FileInfo.FullName))
                .ToList().ForEach(file => _geometryProxies.Add(new(file, dst)));
        }

        public void RemoveFile(GeometryProxy proxy) => _geometryProxies.Remove(proxy);

        public void Import()
        {
            if (!_geometryProxies.Any()) return;

            _ = ContentHelper.ImportFilesAsync(_geometryProxies);
            _geometryProxies.Clear();
        }

        public GeometryImportSettingsConfigurator()
        {
            GeometryProxies = new(_geometryProxies);
        }
    }

    class TextureImportSettingsConfigurator : ViewModelBase, IImportSettingsConfigurator<TextureProxy>
    {
        private readonly ObservableCollection<TextureProxy> _textureProxies = new();
        public ReadOnlyObservableCollection<TextureProxy> TextureProxies { get; }

        public void AddFiles(IEnumerable<string> files, string dst)
        {
            files.Except(_textureProxies.Select(proxy => proxy.FileInfo.FullName))
                .ToList().ForEach(file => _textureProxies.Add(new(file, dst)));
        }

        public void RemoveFile(TextureProxy proxy) => _textureProxies.Remove(proxy);

        public void Import()
        {
            if (!_textureProxies.Any()) return;

            _ = ContentHelper.ImportFilesAsync(_textureProxies);
            _textureProxies.Clear();
        }

        public TextureImportSettingsConfigurator()
        {
            TextureProxies = new(_textureProxies);
        }
    }

    class AudioImportSettingsConfigurator : ViewModelBase, IImportSettingsConfigurator<AudioProxy>
    {
        private readonly ObservableCollection<AudioProxy> _audioProxy = new();
        public ReadOnlyObservableCollection<AudioProxy> AudioProxies { get; }

        public void AddFiles(IEnumerable<string> files, string dst)
        {
            files.Except(_audioProxy.Select(proxy => proxy.FileInfo.FullName))
                .ToList().ForEach(file => _audioProxy.Add(new(file, dst)));
        }

        public void RemoveFile(AudioProxy proxy) => _audioProxy.Remove(proxy);

        public void Import()
        {
            if (!_audioProxy.Any()) return;

            _ = ContentHelper.ImportFilesAsync(_audioProxy);
            _audioProxy.Clear();
        }

        public AudioImportSettingsConfigurator()
        {
            AudioProxies = new(_audioProxy);
        }
    }

    class ConfigureImportSettings : ViewModelBase
    {
        public string LastDestinationFolder { get; private set; }

        public GeometryImportSettingsConfigurator GeometryImportSettingsConfigurator { get; } = new();
        public TextureImportSettingsConfigurator TextureImportSettingsConfigurator { get; } = new();
        public AudioImportSettingsConfigurator AudioImportSettingsConfigurator { get; } = new();

        public int FileCount =>
            GeometryImportSettingsConfigurator.GeometryProxies.Count +
            TextureImportSettingsConfigurator.TextureProxies.Count +
            AudioImportSettingsConfigurator.AudioProxies.Count;

        public void Import()
        {
            GeometryImportSettingsConfigurator.Import();
            TextureImportSettingsConfigurator.Import();
            AudioImportSettingsConfigurator.Import();
        }

        public void AddFiles(string[] files, string dst)
        {
            Debug.Assert(files != null);
            Debug.Assert(!string.IsNullOrEmpty(dst) && Directory.Exists(dst));
            if (!dst.EndsWith(Path.DirectorySeparatorChar)) dst += Path.DirectorySeparatorChar;
            Debug.Assert(Application.Current.Dispatcher.Invoke(() => dst.Contains(Project.Current.ContentPath)));
            LastDestinationFolder = dst;

            var meshes = files.Where(file => ContentHelper.MeshFileExtensions.Contains(Path.GetExtension(file).ToLower()));
            var images = files.Where(file => ContentHelper.ImageFileExtensions.Contains(Path.GetExtension(file).ToLower()));
            var sounds = files.Where(file => ContentHelper.AudioFileExtensions.Contains(Path.GetExtension(file).ToLower()));

            GeometryImportSettingsConfigurator.AddFiles(meshes, dst);
            TextureImportSettingsConfigurator.AddFiles(images, dst);
            AudioImportSettingsConfigurator.AddFiles(sounds, dst);
        }

        public ConfigureImportSettings(string[] files, string dst)
        {
            AddFiles(files, dst);
        }

        public ConfigureImportSettings(string dst)
        {
            Debug.Assert(!string.IsNullOrEmpty(dst) && Directory.Exists(dst));
            if (!dst.EndsWith(Path.DirectorySeparatorChar)) dst += Path.DirectorySeparatorChar;
            Debug.Assert(Application.Current.Dispatcher.Invoke(() => dst.Contains(Project.Current.ContentPath)));
            LastDestinationFolder = dst;
        }
    }
}
