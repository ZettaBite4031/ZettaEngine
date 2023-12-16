using Editor.DLLWrapper;
using Editor.GameProject;
using Editor.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace Editor.Content
{
	enum PrimitiveMeshType 
	{
		Plane,
		Cube,
		UVSphere,
		Icosphere,
		Cylinder,
		Capsule
	}

	enum ElementType
	{
		Position = 0x00,
		Normals = 0x01,
		TSpace = 0x03,
		Joints = 0x04,
		Colors = 0x08
	}

	enum PrimitiveTopology
	{
		PointList = 1,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip
	}

	class Mesh : ViewModelBase
	{
		public static int PositionSize = sizeof(float) * 3;

		private int _ElementSize;
		public int ElementSize
		{
			get => _ElementSize;
			set
			{
				if (_ElementSize != value)
				{
					_ElementSize = value;
					OnPropertyChanged(nameof(ElementSize));
				}
			}
		}

		private int _VertexCount;
		public int VertexCount
		{
			get => _VertexCount;
			set
			{
				if (_VertexCount != value)
				{
					_VertexCount = value;
					OnPropertyChanged(nameof(VertexCount));
				}
			}
		}

		private int _IndexSize;
		public int IndexSize
		{
			get => _IndexSize;
			set
			{
				if (_IndexSize != value)
				{
					_IndexSize = value;
					OnPropertyChanged(nameof(IndexSize));
				}
			}
		}

		private int _IndexCount;
		public int IndexCount
		{
			get => _IndexCount;
			set
			{
				if (_IndexCount != value)
				{
					_IndexCount = value;
					OnPropertyChanged(nameof(IndexCount));
				}
			}
		}

		private string _Name;
		public string Name
		{
			get => _Name;
			set
			{
				if (_Name != value)
				{
					_Name = value;
					OnPropertyChanged(nameof(Name));
				}
			}
		}

		public ElementType ElementsType { get; set; }

		public PrimitiveTopology PrimitiveTopology { get; set; }

		public byte[] Positions { get; set; }
		public byte[] Elements { get; set; }
		public byte[] Indices { get; set; }
	}

	class MeshLOD : ViewModelBase
	{
		private string _Name;
		public string Name
		{
			get => _Name;
			set
			{
				if (_Name != value)
				{
					_Name = value;
					OnPropertyChanged(nameof(Name));
				}
			}
		}

		private float _LODThreshold;
		public float LODThreshold
		{
			get => _LODThreshold;
			set
			{
				if (_LODThreshold != value)
				{
					_LODThreshold = value;
					OnPropertyChanged(nameof(LODThreshold));
				}
			}
		}

		public ObservableCollection<Mesh> Meshes { get; } = new ObservableCollection<Mesh>();
	}

	class LODGroup : ViewModelBase
	{

		private string _Name;
		public string Name
		{
			get => _Name;
			set
			{
				if (_Name != value)
				{
					_Name = value;
					OnPropertyChanged(nameof(Name));
				}
			}
		}

		public ObservableCollection<MeshLOD> LODs { get; } = new ObservableCollection<MeshLOD>();
	}

	class GeometryImportSettings : ViewModelBase
	{
		private float _SmoothingAngle;
		public float SmoothingAngle
		{
			get => _SmoothingAngle;
			set
			{
				if (!_SmoothingAngle.IsEquals(value))
				{
					_SmoothingAngle = value;
					OnPropertyChanged(nameof(SmoothingAngle));
				}
			}
		}

		private bool _CalculateNormals;
		public bool CalculateNormals
		{
			get => _CalculateNormals;
			set
			{
				if (_CalculateNormals != value)
				{
					_CalculateNormals = value;
					OnPropertyChanged(nameof(CalculateNormals));
				}
			}
		}

		private bool _CalculateTangents;
		public bool CalculateTangents
		{
			get => _CalculateTangents;
			set
			{
				if (_CalculateTangents != value)
				{
					_CalculateTangents = value;
					OnPropertyChanged(nameof(CalculateTangents));
				}
			}
		}

		private bool _ReverseHandedness;
		public bool ReverseHandedness
		{
			get => _ReverseHandedness;
			set
			{
				if (_ReverseHandedness != value)
				{
					_ReverseHandedness = value;
					OnPropertyChanged(nameof(ReverseHandedness));
				}
			}
		}

		private bool _ImportEmbededTextures;
		public bool ImportEmbededTextures
		{
			get => _ImportEmbededTextures;
			set
			{
				if (_ImportEmbededTextures != value)
				{
					_ImportEmbededTextures = value;
					OnPropertyChanged(nameof(ImportEmbededTextures));
				}
			}
		}

		private bool _ImportAnimations;
		public bool ImportAnimations
		{
			get => _ImportAnimations;
			set
			{
				if (_ImportAnimations != value)
				{
					_ImportAnimations = value;
					OnPropertyChanged(nameof(ImportAnimations));
				}
			}
		}

		public GeometryImportSettings()
		{
			CalculateNormals = false;
			CalculateTangents = false;
			SmoothingAngle = 178f;
			ReverseHandedness = false;
			ImportEmbededTextures = true;
			ImportAnimations = true;
		}

		public void ToBinary(BinaryWriter writer)
		{
			writer.Write(CalculateNormals);
			writer.Write(CalculateTangents);
			writer.Write(SmoothingAngle);
			writer.Write(ReverseHandedness);
			writer.Write(ImportEmbededTextures);
			writer.Write(ImportAnimations);
		}

		public void FromBinary(BinaryReader reader)
		{
			CalculateNormals = reader.ReadBoolean();
			CalculateTangents = reader.ReadBoolean();
			SmoothingAngle = reader.ReadSingle();
			ReverseHandedness = reader.ReadBoolean();
			ImportEmbededTextures = reader.ReadBoolean();
			ImportAnimations = reader.ReadBoolean();
		}
	}

	class Geometry : Asset
	{
		private readonly object _lock = new object();
		private readonly List<LODGroup> _lodGroups = new List<LODGroup>();

		public GeometryImportSettings ImportSettings { get; } = new GeometryImportSettings();

		public LODGroup GetLODGroup(int lodGroup = 0)
		{
			Debug.Assert(lodGroup >= 0 && lodGroup < _lodGroups.Count);
			return (lodGroup < _lodGroups.Count) ? _lodGroups[lodGroup] : null;
		}

		public void FromRawData(byte[] data)
		{
			Debug.Assert(data?.Length > 0);
			_lodGroups.Clear();

			using var reader = new BinaryReader(new MemoryStream(data));
			// skip scene name string
			var s = reader.ReadInt32();
			reader.BaseStream.Position += s;
			// get number of LODs
			var numLODGroups = reader.ReadInt32();
			Debug.Assert(numLODGroups > 0);

			for (int i = 0; i < numLODGroups; ++i)
			{
				// get LOD group's name
				s = reader.ReadInt32();
				string lodGroupName;
				if (s > 0)
				{
					var nameBytes = reader.ReadBytes(s);
					lodGroupName = Encoding.UTF8.GetString(nameBytes);
				}
				else lodGroupName = $"lod_{ContentHelper.GetRandomString()}";

				// get number of meshes in this LOD group
				var numMeshes = reader.ReadInt32();
				Debug.Assert(numMeshes > 0);
				var lods = ReadMeshLODs(numMeshes, reader);

				var lodGroup = new LODGroup() { Name = lodGroupName };
				lods.ForEach(l => lodGroup.LODs.Add(l));

				_lodGroups.Add(lodGroup);
			}
		}

		private List<MeshLOD> ReadMeshLODs(int numMeshes, BinaryReader reader)
		{
			var lodIDs = new List<int>();
			var lodList = new List<MeshLOD>();
			for (int i = 0; i < numMeshes; i++) ReadMeshes(reader, lodIDs, lodList);
			return lodList;
		}

		private void ReadMeshes(BinaryReader reader, List<int> lodIDs, List<MeshLOD> lodList)
		{
			var s = reader.ReadInt32();
			string meshName;
			if (s > 0)
			{
				var nameBytes = reader.ReadBytes(s);
				meshName = Encoding.UTF8.GetString(nameBytes);
			}
			else meshName = $"mesh_{ContentHelper.GetRandomString()}";
			

			var mesh = new Mesh() { Name = meshName };

			var lodId = reader.ReadInt32();
			mesh.ElementSize = reader.ReadInt32();
			mesh.ElementsType = (ElementType)reader.ReadInt32();
			mesh.PrimitiveTopology = PrimitiveTopology.TriangleList;
			mesh.VertexCount = reader.ReadInt32();
			mesh.IndexSize = reader.ReadInt32();
			mesh.IndexCount = reader.ReadInt32();
			var lodThreshold = reader.ReadSingle();

			var elementBufferSize = mesh.ElementSize * mesh.VertexCount;
			var indexBufferSize = mesh.IndexSize * mesh.IndexCount;

			mesh.Positions = reader.ReadBytes(Mesh.PositionSize * mesh.VertexCount);
			mesh.Elements = reader.ReadBytes(elementBufferSize);
			mesh.Indices = reader.ReadBytes(indexBufferSize);

			MeshLOD lod;
			if (ID.IsValid(lodId) && lodIDs.Contains(lodId))
			{
				lod = lodList[lodIDs.IndexOf(lodId)];
				Debug.Assert(lod != null);
			}
			else
			{
				lodIDs.Add(lodId);
				lod = new MeshLOD() { Name = meshName, LODThreshold = lodThreshold };
				lodList.Add(lod);
			}

			lod.Meshes.Add(mesh);
		}

		public override void Import(string file)
		{
			Debug.Assert(File.Exists(file));
			Debug.Assert(!string.IsNullOrEmpty(FullPath));
			var ext = Path.GetExtension(file).ToLower();

			SourcePath = file;
			try
			{
				if (ext == ".fbx") ImportFBX(file);
			}
			catch (Exception ex)
			{
				Debug.WriteLine(ex.Message);
				var msg = $"Failed to read {file} for import.";
				Debug.WriteLine(msg);
				Logger.Log(MessageType.Error, msg);
			}
		}

		private void ImportFBX(string file)
		{
			Logger.Log(MessageType.Info, $"Importing FBX file {file}");
			var tempPath = Application.Current.Dispatcher.Invoke(() => Project.Current.TempFolder);
			if (string.IsNullOrEmpty(tempPath)) return;

			lock (_lock)
			{
				if (!Directory.Exists(tempPath)) Directory.CreateDirectory(tempPath);
			}

			var tempFile = $"{tempPath}{ContentHelper.GetRandomString()}.fbx";
			File.Copy(file, tempFile, true);
			ContentToolsAPI.ImportFBX(tempFile, this);
		}

		public override void Load(string file)
		{
			Debug.Assert(File.Exists(file));
			Debug.Assert(Path.GetExtension(file).ToLower() == AssetFileExtension);

			try
			{
				byte[] data = null;
				using (var reader = new BinaryReader(File.Open(file, FileMode.Open, FileAccess.Read)))
				{
					ReadAssetFileHeader(reader);
					ImportSettings.FromBinary(reader);
					int datalen = reader.ReadInt32();
					Debug.Assert(datalen > 0);
					data = reader.ReadBytes(datalen);
				}

				Debug.Assert(data.Length > 0);

				using (var reader = new BinaryReader(new MemoryStream(data)))
				{
					LODGroup lodGroup = new LODGroup();
					lodGroup.Name = reader.ReadString();
					var lodGroupCount = reader.ReadInt32();

					for (int i = 0; i < lodGroupCount; i++)
						lodGroup.LODs.Add(BinaryToLOD(reader));

					_lodGroups.Clear();
					_lodGroups.Add(lodGroup);

				}

				// For Testing. Remove Later!!1!11!111!!!
				PackForEngine();
				// For Testing. Remove Later!!1!11!111!!!
			}
			catch (Exception ex)
			{
				Debug.WriteLine(ex.Message);
				Logger.Log(MessageType.Error, $"Failed to load geometry asset from file: {file}");
			}
		}

		public override IEnumerable<string> Save(string file)
		{
			Debug.Assert(_lodGroups.Any());
			var savedFiles = new List<string>();
			if (!_lodGroups.Any()) return savedFiles;

			var path = Path.GetDirectoryName(file) + Path.DirectorySeparatorChar;
			var fileName = Path.GetFileNameWithoutExtension(file);

			try
			{
				foreach ( var group in _lodGroups )
				{
					Debug.Assert(group.LODs.Any());
					var meshFileName = ContentHelper.SanitizeFileName(
						 path + fileName + ((_lodGroups.Count > 1 ) ? "_" + ((group.LODs.Count > 1 ) ? group.LODs[0].Name : group.LODs[0].Name) : string.Empty)) + AssetFileExtension;
					Guid = TryGetAssetInfo(meshFileName) is AssetInfo info && info.Type == Type ? info.Guid : Guid.NewGuid();
					byte[] data = null;
					using (var writer = new BinaryWriter(new MemoryStream()))
					{
						writer.Write(group.Name);
						writer.Write(group.LODs.Count);
						var hashes = new List<byte>();
						foreach (var lod in group.LODs)
						{
							LODToBinary(lod, writer, out var hash);
							hashes.AddRange(hash);
						}

						Hash = ContentHelper.ComputeHash(hashes.ToArray());
						data = (writer.BaseStream as MemoryStream).ToArray();
						Icon = GenerateIcon(group.LODs[0]);
					}

					Debug.Assert(data?.Length > 0);

					using (var writer = new BinaryWriter(File.Open(meshFileName, FileMode.Create, FileAccess.Write)))
					{
						WriteAssetFileHeader(writer);
						ImportSettings.ToBinary(writer);
						writer.Write(data.Length);
						writer.Write(data);
					}

					Logger.Log(MessageType.Info, $"Saved geometry to {meshFileName}");
					savedFiles.Add(meshFileName);
				}
			}
			catch (Exception ex)
			{
				Debug.WriteLine(ex.Message);
				Logger.Log(MessageType.Error, $"Failed to save geometry to {file}");
				throw;
			}

			return savedFiles;
		}

		public override byte[] PackForEngine()
		{
			using var writer = new BinaryWriter(new MemoryStream());
			
			writer.Write(GetLODGroup().LODs.Count);
			foreach (var lod in GetLODGroup().LODs)
			{
				writer.Write(lod.LODThreshold);
				writer.Write(lod.Meshes.Count);
				var sizeofSubmeshesPosition = writer.BaseStream.Position;
				writer.Write(0);
				foreach (var mesh in lod.Meshes)
				{
					writer.Write(mesh.ElementSize);
					writer.Write(mesh.VertexCount);
					writer.Write(mesh.IndexCount);
					writer.Write((int)mesh.ElementsType);
					writer.Write((int)mesh.PrimitiveTopology);

					var alignedPositionBuffer = new byte[MathUtils.AlignSizeUp(mesh.Positions.Length, 4)];
					Array.Copy(mesh.Positions, alignedPositionBuffer, mesh.Positions.Length);
					var alignedElementBuffer = new byte[MathUtils.AlignSizeUp(mesh.Elements.Length, 4)];
					Array.Copy(mesh.Elements, alignedElementBuffer, mesh.Elements.Length);

					writer.Write(alignedPositionBuffer);
					writer.Write(alignedElementBuffer);
					writer.Write(mesh.Indices);
				}
				var endOfSubmeshes = writer.BaseStream.Position;
				var sizeofSubmeshes = (int)(endOfSubmeshes - sizeofSubmeshesPosition - sizeof(int));

				writer.BaseStream.Position = sizeofSubmeshesPosition;
				writer.Write(sizeofSubmeshes);
				writer.BaseStream.Position = endOfSubmeshes;
			}

			writer.Flush();
			var data = (writer.BaseStream as MemoryStream)?.ToArray();
			Debug.Assert(data?.Length > 0);

			// For Testing. Remove Later !!!!1111!!!1!!1!
			using (var fs = new FileStream(@"..\..\x64\model.model", FileMode.Create))
			{
				fs.Write(data, 0, data.Length);
			}
			// For Testing. Remove Later !!!!1111!!!1!!1!

				return data;
		}

		private void LODToBinary(MeshLOD lod, BinaryWriter writer, out byte[] hash)
		{
			writer.Write(lod.Name);
			writer.Write(lod.LODThreshold);
			writer.Write(lod.Meshes.Count);

			var meshDataBegin = writer.BaseStream.Position;

			foreach (var mesh in lod.Meshes)
			{
				writer.Write(mesh.Name);
				writer.Write(mesh.ElementSize);
				writer.Write((int)mesh.ElementsType);
				writer.Write((int)mesh.PrimitiveTopology);
				writer.Write(mesh.VertexCount);
				writer.Write(mesh.IndexSize);
				writer.Write(mesh.IndexCount);
				writer.Write(mesh.Positions);
				writer.Write(mesh.Elements);
				writer.Write(mesh.Indices);
			}

			var meshDataSize = writer.BaseStream.Position - meshDataBegin;
			Debug.Assert(meshDataSize > 0);
			var buffer = (writer.BaseStream as MemoryStream).ToArray();
			hash = ContentHelper.ComputeHash(buffer, (int)meshDataBegin, (int)meshDataSize);
		}

		private MeshLOD BinaryToLOD(BinaryReader reader)
		{
			var lod = new MeshLOD();
			lod.Name = reader.ReadString();
			lod.LODThreshold = reader.ReadSingle();
			var meshCount = reader.ReadInt32();

			for (int i = 0; i < meshCount; i++)
			{
				var mesh = new Mesh()
				{
					Name = reader.ReadString(),
					ElementSize = reader.ReadInt32(),
					ElementsType = (ElementType)reader.ReadInt32(),
                    PrimitiveTopology = (PrimitiveTopology)reader.ReadInt32(),
					VertexCount = reader.ReadInt32(),
					IndexSize = reader.ReadInt32(),
					IndexCount = reader.ReadInt32()
				};

				mesh.Positions = reader.ReadBytes(Mesh.PositionSize * mesh.VertexCount);
				mesh.Elements = reader.ReadBytes(mesh.ElementSize * mesh.VertexCount);
				mesh.Indices = reader.ReadBytes(mesh.IndexSize *  mesh.IndexCount);

				lod.Meshes.Add(mesh);
			}

			return lod;
		}

		private byte[] GenerateIcon(MeshLOD lod)
		{
			var width = ContentInfo.IconWidth * 4;

			using var memStream = new MemoryStream();
			BitmapSource bmp = null;

			Application.Current.Dispatcher.Invoke(() =>
			{
				bmp = Editors.GeometryView.RenderToBitmap(new Editors.MeshRenderer(lod, null), width, width);
				bmp = new TransformedBitmap(bmp, new ScaleTransform(0.25, 0.25, 0.5, 0.5));

				memStream.SetLength(0);

				var encoder = new PngBitmapEncoder();
				encoder.Frames.Add(BitmapFrame.Create(bmp));
				encoder.Save(memStream);
			});

			return memStream.ToArray();
		}

		public Geometry() : base(AssetType.Mesh) { }
	}
}
