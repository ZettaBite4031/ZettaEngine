using Editor.Content;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Windows;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media.Imaging;
using Editor.DLLWrapper;
using System.ComponentModel.DataAnnotations;
using System.Windows.Input;
using System.Windows.Media;

namespace Editor.Editors
{
    class TextureEditor : ViewModelBase, IAssetEditor
    {
        private readonly List<List<List<BitmapSource>>> _sliceBitmaps = new();
        private List<List<List<Slice>>> _slices;

        public ICommand SetAllChannelsCommand { get; init; }
        public ICommand SetChannelCommand { get; init; }
        public ICommand RegenerateBitmapsCommand { get; init; }

        private AssetEditorState _State;
        public AssetEditorState State
        {
            get => _State;
            private set
            {
                if (_State != value)
                {
                    _State = value;
                    OnPropertyChanged(nameof(State));
                }
            }
        }

        public Guid AssetGuid { get; private set; }

        private bool _RedChannelSelected = true;
        public bool RedChannelSelected
        {
            get => _RedChannelSelected;
            set
            {
                if (_RedChannelSelected != value)
                {
                    _RedChannelSelected = value;
                    OnPropertyChanged(nameof(RedChannelSelected));
                    SetImageChannels();
                }
            }
        }

        private bool _GreenChannelSelected = true;
        public bool GreenChannelSelected
        {
            get => _GreenChannelSelected;
            set
            {
                if (_GreenChannelSelected != value)
                {
                    _GreenChannelSelected = value;
                    OnPropertyChanged(nameof(GreenChannelSelected));
                    SetImageChannels();
                }
            }
        }

        private bool _BlueChannelSelected = true;
        public bool BlueChannelSelected
        {
            get => _BlueChannelSelected;
            set
            {
                if (_BlueChannelSelected != value)
                {
                    _BlueChannelSelected = value;
                    OnPropertyChanged(nameof(BlueChannelSelected));
                    SetImageChannels();
                }
            }
        }

        private bool _AlphaChannelSelected = true;
        public bool AlphaChannelSelected
        {
            get => _AlphaChannelSelected;
            set
            {
                if (_AlphaChannelSelected != value)
                {
                    _AlphaChannelSelected = value;
                    OnPropertyChanged(nameof(AlphaChannelSelected));
                    SetImageChannels();
                }
            }
        }

        public Color Channels => new()
        {
            ScR = RedChannelSelected ? 1.0f : 0.0f,
            ScG = GreenChannelSelected ? 1.0f : 0.0f,
            ScB = BlueChannelSelected ? 1.0f : 0.0f,
            ScA = AlphaChannelSelected ? 1.0f : 0.0f
        };

        public float Stride => (float?)SelectedSliceBitmap?.Format.BitsPerPixel / 8 ?? 1.0f;

        Asset IAssetEditor.Asset => Texture;

        private Texture _Texture;
        public Texture Texture
        {
            get => _Texture;
            private set
            {
                if (_Texture != value)
                {
                    _Texture = value;
                    OnPropertyChanged(nameof(Texture));
                    SetSelectedBitmap();
                    SetImageChannels();
                }
            }
        }

        public int MaxMipIndex => _sliceBitmaps.Any() && _sliceBitmaps.First().Any() ? _sliceBitmaps.First().Count() - 1 : 0;
        public int MaxArrayIndex => _sliceBitmaps.Any() ? _sliceBitmaps.Count - 1 : 0;
        public int MaxDepthIndex => _sliceBitmaps.Any() && _sliceBitmaps.First().Any() && _sliceBitmaps.First().First().Any() ?
            _sliceBitmaps.ElementAtOrDefault(ArrayIndex).ElementAtOrDefault(MipIndex).Count - 1 : 0;


        private int _ArrayIndex;
        public int ArrayIndex
        {
            get => Math.Min(_ArrayIndex, MaxArrayIndex);
            set
            {
                value = Math.Min(value, MaxArrayIndex);
                if (_ArrayIndex != value)
                {
                    _ArrayIndex = value;
                    OnPropertyChanged(nameof(ArrayIndex));
                    SetSelectedBitmap();
                    SetImageChannels();
                }
            }
        }

        private int _MipIndex;
        public int MipIndex
        {
            get => Math.Min(_MipIndex, MaxMipIndex);
            set
            {
                value = Math.Min(value, MaxMipIndex);
                if (_MipIndex != value)
                {
                    _MipIndex = value;
                    DepthIndex = _DepthIndex;
                    OnPropertyChanged(nameof(MipIndex));
                    OnPropertyChanged(nameof(MaxDepthIndex));
                    SetSelectedBitmap();
                    SetImageChannels();
                }
            }
        }

        private int _DepthIndex;
        public int DepthIndex
        {
            get => Math.Min(_DepthIndex, MaxDepthIndex);
            set
            {
                value = Math.Min(value, MaxDepthIndex);
                if (_DepthIndex != value)
                {
                    _DepthIndex = value;
                    OnPropertyChanged(nameof(DepthIndex));
                    SetSelectedBitmap();
                    SetImageChannels();
                }
            }
        }

        public BitmapSource SelectedSliceBitmap => _sliceBitmaps?.ElementAtOrDefault(ArrayIndex)?.ElementAtOrDefault(MipIndex)?.ElementAtOrDefault(DepthIndex);
        public Slice SelectedSlice => Texture?.Slices?.ElementAtOrDefault(ArrayIndex)?.ElementAtOrDefault(MipIndex)?.ElementAtOrDefault(DepthIndex);
        public long DataSize => Texture?.Slices?.Sum(x => x.Sum(y => y.Sum(z => z.RawData.LongLength))) ?? 0;

        private void SetSelectedBitmap()
        {
            OnPropertyChanged(nameof(SelectedSliceBitmap));
            OnPropertyChanged(nameof(SelectedSlice));
            OnPropertyChanged(nameof(DataSize));
        }

        private void SetImageChannels()
        {
            OnPropertyChanged(nameof(Channels));
            OnPropertyChanged(nameof(Stride));
        }

        private void OnSetAllChannelsCommand(object obj)
        {
            _RedChannelSelected = true;
            _GreenChannelSelected = true;
            _BlueChannelSelected = true;
            _AlphaChannelSelected = true;
            OnPropertyChanged(nameof(RedChannelSelected));
            OnPropertyChanged(nameof(GreenChannelSelected));
            OnPropertyChanged(nameof(BlueChannelSelected));
            OnPropertyChanged(nameof(AlphaChannelSelected));
            SetImageChannels();
        }

        private void OnSetChannelCommand(string param)
        {
            if (!Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
            {
                _RedChannelSelected = false;
                _GreenChannelSelected = false;
                _BlueChannelSelected = false;
                _AlphaChannelSelected = false;
                OnPropertyChanged(nameof(RedChannelSelected));
                OnPropertyChanged(nameof(GreenChannelSelected));
                OnPropertyChanged(nameof(BlueChannelSelected));
                OnPropertyChanged(nameof(AlphaChannelSelected));
            }
            switch (param)
            {
                case "0": RedChannelSelected = !RedChannelSelected; break;
                case "1": GreenChannelSelected = !GreenChannelSelected; break;
                case "2": BlueChannelSelected = !BlueChannelSelected; break;
                case "3": AlphaChannelSelected = !AlphaChannelSelected; break;
            }
        }

        private void OnRegenerateBitmapsCommand(bool isNM)
        {
            GenerateSliceBitmaps(isNM);
            OnPropertyChanged(nameof(SelectedSliceBitmap));
            SetImageChannels();
        }

        public async void SetAsset(AssetInfo info)
        {
            try
            {
                AssetGuid = info.Guid;
                Texture = null;
                Debug.Assert(info != null && File.Exists(info.FullPath));
                var texture = new Texture();
                State = AssetEditorState.Loading;

                await Task.Run(() =>
                {
                    texture.Load(info.FullPath);
                });

                await SetMipmaps(texture);
                Texture = texture;
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Debug.WriteLine($"Failed to set texture for use in texture editor. File: {info.FullPath}");
                Texture = new();
            }
            finally
            {
                State = AssetEditorState.Done;
            }
        }

        private async Task SetMipmaps(Texture texture)
        {
            try
            {
                await Task.Run(() => _slices = texture.ImportSettings.Compress ? ContentToolsAPI.Decompress(texture) : texture.Slices);
                Debug.Assert(_slices?.Any() == true && _slices.First()?.Any() == true);
                GenerateSliceBitmaps(texture.IsNormalMap);
                OnPropertyChanged(nameof(Texture));
                OnPropertyChanged(nameof(DataSize));
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Debug.WriteLine($"Failed to load mipmaps from {texture.FileName}");
            }
        }

        private void GenerateSliceBitmaps(bool isNormalMap)
        {
            _sliceBitmaps.Clear();
            foreach(var arraySlice in _slices)
            {
                List<List<BitmapSource>> mipmapBitmaps = new();
                foreach(var mipSlice in arraySlice)
                {
                    List<BitmapSource> sliceBitmaps = new();
                    foreach (var slice in mipSlice)
                    {
                        var image = BitmapHelper.ImageFromSlice(slice, isNormalMap);
                        Debug.Assert(image != null);
                        sliceBitmaps.Add(image);
                    }
                    mipmapBitmaps.Add(sliceBitmaps);
                }
                _sliceBitmaps.Add(mipmapBitmaps);
            }
            OnPropertyChanged(nameof(MaxMipIndex));
            OnPropertyChanged(nameof(MaxArrayIndex));
            OnPropertyChanged(nameof(MaxDepthIndex));
        }

        public TextureEditor()
        {
            SetAllChannelsCommand = new CommandRelay<string>(OnSetAllChannelsCommand);
            SetChannelCommand = new CommandRelay<string>(OnSetChannelCommand);
            RegenerateBitmapsCommand = new CommandRelay<bool>(OnRegenerateBitmapsCommand);
        }     
    }
}
