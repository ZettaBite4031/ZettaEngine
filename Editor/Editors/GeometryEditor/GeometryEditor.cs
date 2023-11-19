using Editor.Content;
using System;
using System.CodeDom;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.IO.IsolatedStorage;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Media3D;

namespace Editor.Editors
{
	class MeshRendererVertexData : ViewModelBase
	{
		private Brush _Specular = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#ff111111"));
		public Brush Specular
		{
			get => _Specular;
			set
			{
				if (_Specular != value)
				{
					_Specular = value;
					OnPropertyChanged(nameof(Specular));
				}
			}
		}
		private Brush _Diffuse = Brushes.White;
		public Brush Diffuse
		{
			get => _Diffuse;
			set
			{
				if (_Diffuse != value)
				{
					_Diffuse = value;
					OnPropertyChanged(nameof(Diffuse));
				}
			}
		}
		public Point3DCollection Positions { get; } = new Point3DCollection();
		public Vector3DCollection Normals { get; } = new Vector3DCollection();
		public PointCollection UVs { get; } = new PointCollection();
		public Int32Collection Indices { get; } = new Int32Collection();
	}

	class MeshRenderer : ViewModelBase
	{
		public ObservableCollection<MeshRendererVertexData> Meshes { get; } = new ObservableCollection<MeshRendererVertexData> ();

		private Vector3D _CameraDirection = new Vector3D(0, 0, -10);
		public Vector3D CameraDirection
		{
			get => _CameraDirection;
			set
			{
				if (_CameraDirection != value)
				{
					_CameraDirection = value;
					OnPropertyChanged(nameof(CameraDirection));
				}
			}
		}

		private Point3D _CameraPosition = new Point3D(0, 0, 10);
		public Point3D CameraPosition
		{
			get => _CameraPosition;
			set
			{
				if (_CameraPosition != value)
				{
					_CameraPosition = value;
					CameraDirection = new Vector3D(-value.X, -value.Y, -value.Z);
					OnPropertyChanged(nameof(OffsetCameraPosition));
					OnPropertyChanged(nameof(CameraPosition));
				}
			}
		}

		private Point3D _CameraTarget = new Point3D(0, 0, 0);
		public Point3D CameraTarget
		{
			get => _CameraTarget;
			set
			{
				if (_CameraTarget != value)
				{
					_CameraTarget = value;
					OnPropertyChanged(nameof(OffsetCameraPosition));
					OnPropertyChanged(nameof(CameraTarget));
				}
			}
		}

		public Point3D OffsetCameraPosition =>
			new Point3D(CameraPosition.X + CameraTarget.X, CameraPosition.Y + CameraTarget.Y, CameraPosition.Z + CameraTarget.Z);

		private Color _KeyLight = (Color)ColorConverter.ConvertFromString("#ffaeaeae");
		public Color KeyLight
		{
			get => _KeyLight;
			set
			{
				if (_KeyLight != value)
				{
					_KeyLight = value;
					OnPropertyChanged(nameof(KeyLight));
				}
			}
		}

		private Color _SkyLight = (Color)ColorConverter.ConvertFromString("#ff111b30");
		public Color SkyLight
		{
			get => _SkyLight;
			set
			{
				if (_SkyLight != value)
				{
					_SkyLight = value;
					OnPropertyChanged(nameof(SkyLight));
				}
			}
		}

		private Color _GndLight = (Color)ColorConverter.ConvertFromString("#ff3f2f1e");
		public Color GndLight
		{
			get => _GndLight;
			set
			{
				if (_GndLight != value)
				{
					_GndLight = value;
					OnPropertyChanged(nameof(GndLight));
				}
			}
		}

		private Color _AmbientLight = (Color)ColorConverter.ConvertFromString("#ff3b3b3b");
		public Color AmbientLight
		{
			get => _AmbientLight;
			set
			{
				if (_AmbientLight != value)
				{
					_AmbientLight = value;
					OnPropertyChanged(nameof(AmbientLight));
				}
			}
		}

		public MeshRenderer(MeshLOD lod, MeshRenderer old)
		{
			Debug.Assert(lod?.Meshes.Any() == true);
			// Calculate vertex size minus the position and normal vectors
			var offset = lod.Meshes[0].VertexSize - 3 * sizeof(float) - sizeof(int) - 2 * sizeof(short);

			double minX, minY, minZ; minX = minY = minZ = double.MaxValue;
			double maxX, maxY, maxZ; maxX = maxY = maxZ = double.MinValue;
			Vector3D avgNormal = new Vector3D();

			var intervals = 2.0f / ((1 << 16) - 1);
			foreach (var mesh in lod.Meshes)
			{
				var vertexData = new MeshRendererVertexData();

				using (var reader = new BinaryReader(new MemoryStream(mesh.Vertices))) 
					for(int i = 0; i < mesh.VertexCount; i++)
					{
						var posX = reader.ReadSingle();
						var posY = reader.ReadSingle();
						var posZ = reader.ReadSingle();
						var signs = (reader.ReadUInt32() >> 24) & 0x000000ff;
						vertexData.Positions.Add(new Point3D(posX, posY, posZ));

						minX = Math.Min(minX, posX); maxX = Math.Max(maxX, posX);
						minY = Math.Min(minY, posY); maxY = Math.Max(maxY, posY);
						minZ = Math.Min(minZ, posZ); maxZ = Math.Max(maxZ, posZ);

						var nX = reader.ReadUInt16() * intervals - 1.0f;
						var nY = reader.ReadUInt16() * intervals - 1.0f;
						var nZ = Math.Sqrt(Math.Clamp(1f - (nX * nX + nY * nY), 0f, 1f)) * ((signs & 0x2) - 1f);
						var normal = new Vector3D(nX, nY, nZ);
						normal.Normalize();
						vertexData.Normals.Add(normal);
						avgNormal += normal;

						reader.BaseStream.Position += (offset - sizeof(float) * 2);
						var u = reader.ReadSingle();
						var v = reader.ReadSingle();
						vertexData.UVs.Add(new Point(u, v));
					}

				using (var reader = new BinaryReader(new MemoryStream(mesh.Indices)))
					if (mesh.IndexSize == sizeof(short))
						for (int i = 0; i < mesh.IndexCount; i++) vertexData.Indices.Add(reader.ReadInt16());
					else
						for (int i = 0; i < mesh.IndexCount; i++) vertexData.Indices.Add(reader.ReadInt32());

				vertexData.Positions.Freeze();
				vertexData.Normals.Freeze();
				vertexData.UVs.Freeze();
				vertexData.Indices.Freeze();
				Meshes.Add(vertexData);
			}

			if (old != null)
			{
				CameraTarget = old.CameraTarget;
				CameraPosition = old.CameraPosition;
			}
			else
			{
				var width = maxX - minX;
				var height = maxY - minY;
				var depth = maxZ - minZ;
				var radius = new Vector3D(height, width, depth).Length * 1.2;
				if (avgNormal.Length > 0.8)
				{
					avgNormal.Normalize();
					avgNormal *= radius;
					CameraPosition = new Point3D(avgNormal.X, avgNormal.Y, avgNormal.Z);
				}
				else CameraPosition = new Point3D(width, height * 0.5, radius);

				CameraTarget = new Point3D(minX + width * 0.5, minY + height * 0.5, minZ + depth * 0.5);
			}
		}
	}

	class GeometryEditor : ViewModelBase, IAssetEditor
	{
		Asset IAssetEditor.Asset => _Geometry;

		private Content.Geometry _Geometry;
		public Content.Geometry Geometry
		{
			get => _Geometry;
			set
			{
				if (_Geometry != value)
				{
					_Geometry = value;
					OnPropertyChanged(nameof(Geometry));
				}
			}
		}

		private MeshRenderer _MeshRenderer;
		public MeshRenderer MeshRenderer
		{
			get => _MeshRenderer;
			set
			{
				if (_MeshRenderer != value)
				{
					_MeshRenderer = value;
					OnPropertyChanged(nameof(MeshRenderer));
					var LODs = Geometry.GetLODGroup().LODs;
					MaxLODIndex = (LODs.Count > 0) ? LODs.Count - 1 : 0;
					OnPropertyChanged(nameof(MaxLODIndex));
					if (LODs.Count > 1)
					{
						MeshRenderer.PropertyChanged += (s, e) =>
						{
							if (e.PropertyName == nameof(MeshRenderer.OffsetCameraPosition) && AutoLOD) ComputeLODs(LODs);
						};
						ComputeLODs(LODs);
					}
				}
			}
		}

		private bool _AutoLOD = true;
		public bool AutoLOD
		{
			get => _AutoLOD;
			set
			{
				if (_AutoLOD != value)
				{
					_AutoLOD = value;
					OnPropertyChanged(nameof(AutoLOD));
				}
			}
		}

		public int MaxLODIndex { get; private set; }

		private int _LodIndex;
		public int LodIndex
		{
			get => _LodIndex;
			set
			{
				var lods = Geometry.GetLODGroup().LODs;
				value = Math.Clamp(value, 0, lods.Count - 1);
				if (_LodIndex != value)
				{
					_LodIndex = value;
					OnPropertyChanged(nameof(LodIndex));
					MeshRenderer = new MeshRenderer(lods[value], MeshRenderer);
				}
			}
		}

		private void ComputeLODs(ObservableCollection<MeshLOD> LODs)
		{
			if (!AutoLOD) return;

			var p = MeshRenderer.OffsetCameraPosition;
			var d = new Vector3D(p.X, p.Y, p.Z).Length;
			for (int i = MaxLODIndex; i >= 0; i--)
			{
				if (LODs[i].LODThreshold < d)
				{
					LodIndex = i;
					break;
				}
			}
		}

		public void SetAsset(Content.Asset asset)
		{
			Debug.Assert(asset is Content.Geometry);
			if (asset is Content.Geometry geometry)
			{
				Geometry = geometry;
				var numLODs = Geometry.GetLODGroup().LODs.Count;
				if (LodIndex >= numLODs)
				{
					LodIndex = numLODs - 1;
				}
				else MeshRenderer = new MeshRenderer(Geometry.GetLODGroup().LODs[0], MeshRenderer);
			}
		}

		public async void SetAsset(AssetInfo info)
		{
			try
			{
				Debug.Assert(info != null && File.Exists(info.FullPath));
				var geometry = new Content.Geometry();
				await Task.Run(() =>
				{
					geometry.Load(info.FullPath);
				});

				SetAsset(geometry);
			}
			catch (Exception ex)
			{
				Debug.WriteLine(ex.Message);
			}
		}
	}
}
