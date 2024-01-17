using Editor.DLLWrapper;
using Editor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Editor.Content
{
    enum DXGI_FORMAT
    {
        DXGI_FORMAT_UNKNOWN = 0,
        DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
        DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
        DXGI_FORMAT_R32G32B32A32_UINT = 3,
        DXGI_FORMAT_R32G32B32A32_SINT = 4,
        DXGI_FORMAT_R32G32B32_TYPELESS = 5,
        DXGI_FORMAT_R32G32B32_FLOAT = 6,
        DXGI_FORMAT_R32G32B32_UINT = 7,
        DXGI_FORMAT_R32G32B32_SINT = 8,
        DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
        DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
        DXGI_FORMAT_R16G16B16A16_UNORM = 11,
        DXGI_FORMAT_R16G16B16A16_UINT = 12,
        DXGI_FORMAT_R16G16B16A16_SNORM = 13,
        DXGI_FORMAT_R16G16B16A16_SINT = 14,
        DXGI_FORMAT_R32G32_TYPELESS = 15,
        DXGI_FORMAT_R32G32_FLOAT = 16,
        DXGI_FORMAT_R32G32_UINT = 17,
        DXGI_FORMAT_R32G32_SINT = 18,
        DXGI_FORMAT_R32G8X24_TYPELESS = 19,
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
        DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
        DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
        DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
        DXGI_FORMAT_R10G10B10A2_UNORM = 24,
        DXGI_FORMAT_R10G10B10A2_UINT = 25,
        DXGI_FORMAT_R11G11B10_FLOAT = 26,
        DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
        DXGI_FORMAT_R8G8B8A8_UNORM = 28,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
        DXGI_FORMAT_R8G8B8A8_UINT = 30,
        DXGI_FORMAT_R8G8B8A8_SNORM = 31,
        DXGI_FORMAT_R8G8B8A8_SINT = 32,
        DXGI_FORMAT_R16G16_TYPELESS = 33,
        DXGI_FORMAT_R16G16_FLOAT = 34,
        DXGI_FORMAT_R16G16_UNORM = 35,
        DXGI_FORMAT_R16G16_UINT = 36,
        DXGI_FORMAT_R16G16_SNORM = 37,
        DXGI_FORMAT_R16G16_SINT = 38,
        DXGI_FORMAT_R32_TYPELESS = 39,
        DXGI_FORMAT_D32_FLOAT = 40,
        DXGI_FORMAT_R32_FLOAT = 41,
        DXGI_FORMAT_R32_UINT = 42,
        DXGI_FORMAT_R32_SINT = 43,
        DXGI_FORMAT_R24G8_TYPELESS = 44,
        DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
        DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
        DXGI_FORMAT_R8G8_TYPELESS = 48,
        DXGI_FORMAT_R8G8_UNORM = 49,
        DXGI_FORMAT_R8G8_UINT = 50,
        DXGI_FORMAT_R8G8_SNORM = 51,
        DXGI_FORMAT_R8G8_SINT = 52,
        DXGI_FORMAT_R16_TYPELESS = 53,
        DXGI_FORMAT_R16_FLOAT = 54,
        DXGI_FORMAT_D16_UNORM = 55,
        DXGI_FORMAT_R16_UNORM = 56,
        DXGI_FORMAT_R16_UINT = 57,
        DXGI_FORMAT_R16_SNORM = 58,
        DXGI_FORMAT_R16_SINT = 59,
        DXGI_FORMAT_R8_TYPELESS = 60,
        DXGI_FORMAT_R8_UNORM = 61,
        DXGI_FORMAT_R8_UINT = 62,
        DXGI_FORMAT_R8_SNORM = 63,
        DXGI_FORMAT_R8_SINT = 64,
        DXGI_FORMAT_A8_UNORM = 65,
        DXGI_FORMAT_R1_UNORM = 66,
        DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
        DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
        DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
        DXGI_FORMAT_BC1_TYPELESS = 70,
        DXGI_FORMAT_BC1_UNORM = 71,
        DXGI_FORMAT_BC1_UNORM_SRGB = 72,
        DXGI_FORMAT_BC2_TYPELESS = 73,
        DXGI_FORMAT_BC2_UNORM = 74,
        DXGI_FORMAT_BC2_UNORM_SRGB = 75,
        DXGI_FORMAT_BC3_TYPELESS = 76,
        DXGI_FORMAT_BC3_UNORM = 77,
        DXGI_FORMAT_BC3_UNORM_SRGB = 78,
        DXGI_FORMAT_BC4_TYPELESS = 79,
        DXGI_FORMAT_BC4_UNORM = 80,
        DXGI_FORMAT_BC4_SNORM = 81,
        DXGI_FORMAT_BC5_TYPELESS = 82,
        DXGI_FORMAT_BC5_UNORM = 83,
        DXGI_FORMAT_BC5_SNORM = 84,
        DXGI_FORMAT_B5G6R5_UNORM = 85,
        DXGI_FORMAT_B5G5R5A1_UNORM = 86,
        DXGI_FORMAT_B8G8R8A8_UNORM = 87,
        DXGI_FORMAT_B8G8R8X8_UNORM = 88,
        DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
        DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
        DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
        DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
        DXGI_FORMAT_BC6H_TYPELESS = 94,
        DXGI_FORMAT_BC6H_UF16 = 95,
        DXGI_FORMAT_BC6H_SF16 = 96,
        DXGI_FORMAT_BC7_TYPELESS = 97,
        DXGI_FORMAT_BC7_UNORM = 98,
        DXGI_FORMAT_BC7_UNORM_SRGB = 99,
        DXGI_FORMAT_AYUV = 100,
        DXGI_FORMAT_Y410 = 101,
        DXGI_FORMAT_Y416 = 102,
        DXGI_FORMAT_NV12 = 103,
        DXGI_FORMAT_P010 = 104,
        DXGI_FORMAT_P016 = 105,
        DXGI_FORMAT_420_OPAQUE = 106,
        DXGI_FORMAT_YUY2 = 107,
        DXGI_FORMAT_Y210 = 108,
        DXGI_FORMAT_Y216 = 109,
        DXGI_FORMAT_NV11 = 110,
        DXGI_FORMAT_AI44 = 111,
        DXGI_FORMAT_IA44 = 112,
        DXGI_FORMAT_P8 = 113,
        DXGI_FORMAT_A8P8 = 114,
        DXGI_FORMAT_B4G4R4A4_UNORM = 115,

        DXGI_FORMAT_P208 = 130,
        DXGI_FORMAT_V208 = 131,
        DXGI_FORMAT_V408 = 132,


        DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE = 189,
        DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,


        DXGI_FORMAT_FORCE_UINT = -1
    };

    enum BC_FORMAT : int
    {
        [Description("Pick best fit")]
        DXGI_FORMAT_UNKNOWN  = 0,
        [Description("BC1 (RGBA) Low Quality Alpha")]
        DXGI_FORMAT_BC1_UNORM = 77,
        [Description("BC3 (RGBA) Medium Quality")]
        DXGI_FORMAT_BC3_UNORM = 77,
        [Description("BC4 (R8) Single-Channel Greyscale")]
        DXGI_FORMAT_BC4_UNORM = 80,
        [Description("BC5 (R8G8) Dual-Channel Greyscale")]
        DXGI_FORMAT_BC5_UNORM = 83,
        [Description("BC6 (UF16) HDR")]
        DXGI_FORMAT_BC6H_UF16 = 95,
        [Description("BC7 (RGBA) High Quality")]
        DXGI_FORMAT_BC7_UNORM = 98,
    }

    enum TextureDimensions : int
    {
        [Description("1D Texture")] 
        Texture1D,
        [Description("2D Texture")] 
        Texture2D,
        [Description("3D Texture")] 
        Texture3D,
        [Description("Texture Cube")] 
        TextureCube,
    }

    // NOTE: Is the same as Zetta::Content::TextureFlags::Flags enumeration in ContentToEngine.h
    enum TextureFlags : int
    {
        IsHDR = 0x01,
        HasAlpha = 0x02,
        IsPremultipliedAlpha = 0x04,
        IsImportedAsNormalMap = 0x08,
        IsCubeMap = 0x10,
        IsVolumeMap = 0x20,
    }

    class TextureImportSettings : ViewModelBase, IAssetImportSettings
    {
        public ObservableCollection<string> Sources { get; } = new();

        private TextureDimensions _arr;
        public TextureDimensions arr
        {
            get => _arr;
            set
            {
                if (_arr != value)
                {
                    _arr = value;
                    OnPropertyChanged(nameof(arr));
                }
            }
        }

        private int _MipLevels;
        public int MipLevels
        {
            get => _MipLevels;
            set
            {
                if (_MipLevels != value)
                {
                    _MipLevels = value;
                    OnPropertyChanged(nameof(MipLevels));
                }
            }
        }

        private float _AlphaThreshold;
        public float AlphaThreshold
        {
            get => _AlphaThreshold;
            set
            {
                value = Math.Clamp(value, 0.0f, 1.0f);  
                if (_AlphaThreshold != value)
                {
                    _AlphaThreshold = value;
                    OnPropertyChanged(nameof(AlphaThreshold));
                }
            }
        }

        private bool _PreferBC7;
        public bool PreferBC7
        {
            get => _PreferBC7;
            set
            {
                if (_PreferBC7 != value)
                {
                    _PreferBC7 = value;
                    OnPropertyChanged(nameof(PreferBC7));
                }
            }
        }

        private TextureDimensions _Dimension = TextureDimensions.Texture2D;
        public TextureDimensions Dimension
        {
            get => _Dimension;
            set
            {
                if (_Dimension != value)
                {
                    _Dimension = value;
                    OnPropertyChanged(nameof(Dimension));
                }
            }
        }

        private int _FormatIndex;
        public int FormatIndex
        {
            get => _FormatIndex;
            set
            {
                value = Math.Clamp(value, 0, Enum.GetValues<BC_FORMAT>().Length);
                if (_FormatIndex != value)
                {
                    _FormatIndex = value;
                    OnPropertyChanged(nameof(FormatIndex));
                    OnPropertyChanged(nameof(OutputFormat));
                }
            }
        }
        public DXGI_FORMAT OutputFormat => Compress ? (DXGI_FORMAT)Enum.GetValues<BC_FORMAT>()[FormatIndex] : DXGI_FORMAT.DXGI_FORMAT_UNKNOWN;

        private bool _Compress;
        public bool Compress
        {
            get => _Compress;
            set
            {
                if (_Compress != value)
                {
                    _Compress = value;
                    OnPropertyChanged(nameof(Compress));
                }
            }
        }

        public void ToBinary(BinaryWriter writer)
        {
            writer.Write(string.Join(";", Sources.ToArray()));
            writer.Write((int)Dimension);
            writer.Write(MipLevels);
            writer.Write(AlphaThreshold);
            writer.Write(PreferBC7);
            writer.Write(FormatIndex);
            writer.Write(Compress);
        }

        public void FromBinary(BinaryReader reader)
        {
            Sources.Clear();
            reader.ReadString().Split(";").ToList().ForEach(x => Sources.Add(x));
            Dimension = (TextureDimensions)reader.ReadInt32();
            MipLevels = reader.ReadInt32();
            AlphaThreshold = reader.ReadInt32();
            PreferBC7 = reader.ReadBoolean();
            FormatIndex = reader.ReadInt32();
            Compress = reader.ReadBoolean();
        }

        public TextureImportSettings()
        {
            MipLevels = 0;
            AlphaThreshold = 0.5f;
            PreferBC7 = true;
            FormatIndex = 0;
            Compress = false;
        }
    }

    class Slice
    {
        public int Width { get; set; }
        public int Height { get; set; }
        public int RowPitch { get; set; }
        public int SlicePitch { get; set; }
        public byte[] RawData { get; set; } 
    }

    class Texture : Asset
    {
        public static int MaxMipLevels => 14;

        public TextureImportSettings ImportSettings { get; } = new();

        // array [ mip [ subresource [ slice ] ] ]
        private List<List<List<Slice>>> _Slices;
        public List<List<List<Slice>>> Slices
        {
            get => _Slices;
            private set
            {
                if (_Slices != value)
                {
                    _Slices = value;
                    OnPropertyChanged(nameof(Slices));
                }
            }
        }

        private int  _Width;
        public int  Width
        {
            get => _Width;
            set
            {
                if (_Width != value)
                {
                    _Width = value;
                    OnPropertyChanged(nameof(Width));
                }
            }
        }

        private int _Height;
        public int Height
        {
            get => _Height;
            set
            {
                if (_Height != value)
                {
                    _Height = value;
                    OnPropertyChanged(nameof(Height));
                }
            }
        }

        private int _ArraySize;
        public int ArraySize
        {
            get => _ArraySize;
            set
            {
                if (_ArraySize != value)
                {
                    Debug.Assert(!(IsCubeMap && (value % 6) != 0));
                    _ArraySize = value;
                    OnPropertyChanged(nameof(ArraySize));
                }
            }
        }

        private TextureFlags _Flags;
        public TextureFlags Flags
        {
            get => _Flags;
            set
            {
                if (_Flags != value)
                {
                    _Flags = value;
                    OnPropertyChanged(nameof(IsHDR));
                    OnPropertyChanged(nameof(HasAlpha));
                    OnPropertyChanged(nameof(IsPremultipliedAlpha));
                    OnPropertyChanged(nameof(IsNormalMap));
                    OnPropertyChanged(nameof(IsCubeMap));
                    OnPropertyChanged(nameof(IsVolumeMap));
                }
            }
        }

        public bool IsHDR => Flags.HasFlag(TextureFlags.IsHDR);
        public bool HasAlpha => Flags.HasFlag(TextureFlags.HasAlpha);
        public bool IsPremultipliedAlpha => Flags.HasFlag(TextureFlags.IsPremultipliedAlpha);
        public bool IsNormalMap => Flags.HasFlag(TextureFlags.IsImportedAsNormalMap);
        public bool IsCubeMap => Flags.HasFlag(TextureFlags.IsCubeMap);
        public bool IsVolumeMap => Flags.HasFlag(TextureFlags.IsVolumeMap);

        private int _MipLevels;
        public int MipLevels
        {
            get => _MipLevels;
            set
            {
                if (_MipLevels != value)
                {
                    _MipLevels = value;
                    OnPropertyChanged(nameof(MipLevels));
                }
            }
        }

        private DXGI_FORMAT _Format;
        public DXGI_FORMAT Format
        {
            get => _Format;
            set
            {
                if (_Format != value)
                {
                    _Format = value;
                    OnPropertyChanged(nameof(Format));
                    OnPropertyChanged(nameof(FormatName));
                }
            }
        }

        public string FormatName => (ImportSettings.Compress) ? ((BC_FORMAT)Format).GetDescription() : Format.GetDescription();

        private static bool HasValidDimensions(int width, int height, string file)
        {
            bool res = true;

            if (width % 4 != 0 || height % 4 != 0)
            {
                Logger.Log(MessageType.Warn, $"Image Dimensions must be a multiple of 4. (file: {file})");
                res = false;
            }
            if (width != height)
            {
                Logger.Log(MessageType.Warn, $"Image is not a square (width != height). (file: {file})");
                res = false;
            }
            if (!MathUtils.IsPow2(width) || !MathUtils.IsPow2(height))
            {
                Logger.Log(MessageType.Warn, $"Image dimensions are not powers of 2. (file: {file})");
                res = false;
            }


            return res;
        }

        public override bool Import(string file)
        {
            Debug.Assert(File.Exists(file));
            try
            {
                Logger.Log(MessageType.Info, $"Importing image file {file}");
                ImportSettings.Sources.Add(file);
                (var slices, var icon) = ContentToolsAPI.Import(this);

                Debug.Assert(slices.Any() && slices.First().Any() && slices.First().First().Any());

                if (slices.Any() && slices.First().Any() && slices.First().First().Any())
                    Slices = slices;
                else return false;

                var first_mip = Slices[0][0][0];
                HasValidDimensions(first_mip.Width, first_mip.Height, file);

                if (icon == null)
                {
                    Debug.Assert(!ImportSettings.Compress);
                    icon = first_mip;
                }

                Icon = BitmapHelper.CreateThumbnail(BitmapHelper.ImageFromSlice(icon, IsNormalMap), ContentInfo.IconWidth, ContentInfo.IconWidth);
                
                return true;
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                var msg = $"Failed to read {file} for import";
                Debug.WriteLine(msg);
                Logger.Log(MessageType.Error, msg);
            }

            return false;
        }

        public override bool Load(string file)
        {
            return true;
        }

        public override byte[] PackForEngine()
        {
            throw new NotImplementedException();
        }

        public override IEnumerable<string> Save(string file)
        {
            try
            {
                if (TryGetAssetInfo(file) is AssetInfo info && info.Type == Type)
                    Guid = info.Guid;

                var compressed = CompressContent();
                Debug.Assert(compressed?.Length > 0);
                Hash = ContentHelper.ComputeHash(compressed);

                using var writer = new BinaryWriter(File.Open(file, FileMode.Create, FileAccess.Write));

                WriteAssetFileHeader(writer);
                ImportSettings.ToBinary(writer);

                writer.Write(Width);
                writer.Write(Height);
                writer.Write(ArraySize);
                writer.Write((int)Flags);
                writer.Write(MipLevels);
                writer.Write((int)Format);
                writer.Write(compressed.Length);
                writer.Write(compressed);

                FullPath = file;
                Logger.Log(MessageType.Info, $"Saved texture to {file}");

                var savedFiles = new List<string>() { file };
                return savedFiles;
            }
            catch(Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Log(MessageType.Error, $"Failed to save texture to {file}");
            }

            return new List<string>();
        }

        private byte[] CompressContent()
        {
            Debug.Assert(Slices.First().Any() && Slices.First().Count == MipLevels);
            var data = ContentToolsAPI.SlicesToBinary(Slices);
            return CompressionHelper.Compress(data);    
        }

        private void Decompress(byte[] compressed)
        {
            var decompressed = CompressionHelper.Decompress(compressed);
            Slices = ContentToolsAPI.SlicesFromBinary(decompressed, ArraySize, MipLevels, IsVolumeMap);
        }
        
        public Texture() : base(AssetType.Texture) { }
    }
}
